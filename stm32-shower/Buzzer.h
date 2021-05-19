#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define Buzzer_TIM          (TIM2)
#define GPIO_Buzzer         (GPIOA)
#define GPIO_Buzzer_Pin     (GPIO_Pin_0)

struct BeepSound
{
	uint16_t Frequency;
	uint16_t Duration;
	
	BeepSound(uint16_t duration)
	{
		Frequency = 0;
		Duration = duration;
	}
	
	BeepSound(uint16_t freq, uint16_t duration)
	{
		Frequency = freq;
		Duration = duration;
	}
};

class Buzzer
{
	SemaphoreHandle_t xSemaphore;
	StaticSemaphore_t xSemaphoreBuffer;
	
	SemaphoreHandle_t xSemaphoreHighPrio;
	StaticSemaphore_t xSemaphoreHighPrioBuffer;
	
	SemaphoreHandle_t xSemaphorePause;
	StaticSemaphore_t xSemaphorePauseBuffer;
	
	void Freq(uint16_t freq)
	{
		if (freq == 0)
		{
			DisableGPIO();
		}
		else
		{
			uint16_t period = 100000 / freq;     // compute period as function of 60KHz ticks
			Buzzer_TIM->ARR = period - 1;
			Buzzer_TIM->CCR1 = period / 2;
			
			EnableGPIO();
		}
	}
	
	void EnableGPIO()
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Pin = GPIO_Buzzer_Pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(GPIO_Buzzer, &GPIO_InitStructure);
	}
	
	void DisableGPIO()
	{
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Pin = GPIO_Buzzer_Pin;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(GPIO_Buzzer, &GPIO_InitStructure);
	}
	
	void BeepInternal(const BeepSound* samples, uint8_t length)
	{
		for (uint8_t i = 0; i < length; i++)
		{
			Freq(samples->Frequency);
			
			//vTaskDelay(samples->Duration / portTICK_PERIOD_MS);
			bool interrupted = (xSemaphoreTake(xSemaphorePause, samples->Duration / portTICK_PERIOD_MS) == pdTRUE);
			if (interrupted)
			{
				break;
			}
			samples++;
		}
		Freq(0);
	}
	
public:
	
    // Propertyes еще не инициализирован
	void Init()
	{	
		TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
		TIM_OCInitTypeDef TIM_OCInitStruct;
		GPIO_InitTypeDef GPIO_InitStructure;
		
		DisableGPIO();
		
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
		
		TIM_TimeBaseInitStruct.TIM_Prescaler = SystemCoreClock / 100000 - 1;     // 100 KHz timebase
		TIM_TimeBaseInitStruct.TIM_Period = SystemCoreClock / 10000 - 1;     // Arbitary placeholder 100 Hz
		TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;     // без  делителя
		TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseInit(Buzzer_TIM, &TIM_TimeBaseInitStruct);

		TIM_OCStructInit(&TIM_OCInitStruct);
		TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStruct.TIM_Pulse = (TIM_TimeBaseInitStruct.TIM_Period + 1) / 2;     // 50% Duty
		TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OC1Init(Buzzer_TIM, &TIM_OCInitStruct);
		TIM_OC1PreloadConfig(Buzzer_TIM, TIM_OCPreload_Enable);
		TIM_ARRPreloadConfig(Buzzer_TIM, ENABLE);
		
		const uint16_t period = 100000 / 1000;  // 1000 гц
		Buzzer_TIM->ARR = period - 1;
		Buzzer_TIM->CCR1 = period / 2;
		
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(GPIO_Buzzer, &GPIO_InitStructure);
		
		TIM_Cmd(Buzzer_TIM, ENABLE);
		
		xSemaphore = xSemaphoreCreateBinaryStatic(&xSemaphoreBuffer);
		xSemaphoreGive(xSemaphore);

		xSemaphoreHighPrio = xSemaphoreCreateBinaryStatic(&xSemaphoreHighPrioBuffer);
		xSemaphoreGive(xSemaphoreHighPrio);
		
		xSemaphorePause = xSemaphoreCreateBinaryStatic(&xSemaphorePauseBuffer);
	}
	
	void BeepHighPrio(const BeepSound* samples, uint8_t length)
	{
		xSemaphoreTake(xSemaphoreHighPrio, portMAX_DELAY);
		{
			bool busy = uxSemaphoreGetCount(xSemaphore) == 0;

			// Пищалка занята
			if (busy)
			{
				// Прервать
				xSemaphoreGive(xSemaphorePause);
			}
			
			xSemaphoreTake(xSemaphore, portMAX_DELAY);
			
			if (busy)
			{
				// Другой поток не успел захватить мьютекс
				if (uxSemaphoreGetCount(xSemaphorePause))
				{
					xSemaphoreTake(xSemaphorePause, portMAX_DELAY);
				}
			}
			
			if (busy)
				vTaskDelay(70);
				
			BeepInternal(samples, length);
			xSemaphoreGive(xSemaphore);
			
			xSemaphoreGive(xSemaphoreHighPrio);
		}
		
		// если был прерван чей-то звук, сделать дополнительную паузу в начале
	}
	
	void Beep(const BeepSound* samples, uint8_t length)
	{
		xSemaphoreTake(xSemaphore, portMAX_DELAY);
		{
			if (uxSemaphoreGetCount(xSemaphoreHighPrio))
			{
				BeepInternal(samples, length);
			}
			xSemaphoreGive(xSemaphore);
		}
	}
};

extern Buzzer buzzer;

