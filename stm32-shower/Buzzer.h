#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

struct BeepSound final
{
	const uint16_t Frequency;
	const uint16_t Duration;
	
	// Без звука.
	BeepSound(uint16_t duration)
		: Frequency(0)
		, Duration(duration)
	{
	
	}
	
	BeepSound(uint16_t frequency, uint16_t duration)
		: Frequency(frequency)
		, Duration(duration)
	{
		
	}
};

class Buzzer final
{
public:
	
	// Properties еще не инициализирован
	void Init()
	{	
		DisableGPIO();
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
		
		TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = 
		{
			.TIM_Prescaler = (uint16_t)(SystemCoreClock / 100000 - 1),  // 100 KHz timebase
			.TIM_CounterMode = TIM_CounterMode_Up,
			.TIM_Period = (uint16_t)(SystemCoreClock / 10000 - 1),      // Arbitary placeholder 100 Hz
			.TIM_ClockDivision = TIM_CKD_DIV1,       // без  делителя.
			.TIM_RepetitionCounter = 0
		};
		TIM_TimeBaseInit(Buzzer_TIM, &TIM_TimeBaseInitStruct);

		TIM_OCInitTypeDef TIM_OCInitStruct = 
		{
			.TIM_OCMode = TIM_OCMode_PWM1,
			.TIM_OutputState = TIM_OutputState_Enable,
			.TIM_OutputNState = TIM_OutputNState_Disable,
			.TIM_Pulse = (uint16_t)((TIM_TimeBaseInitStruct.TIM_Period + 1) / 2),       // 50% Duty.
			.TIM_OCPolarity = TIM_OCPolarity_High,
			.TIM_OCNPolarity = TIM_OCPolarity_High,
			.TIM_OCIdleState = TIM_OCIdleState_Reset,
			.TIM_OCNIdleState = TIM_OCNIdleState_Reset
		};
		TIM_OC1Init(Buzzer_TIM, &TIM_OCInitStruct);
		
		TIM_OC1PreloadConfig(Buzzer_TIM, TIM_OCPreload_Enable);
		TIM_ARRPreloadConfig(Buzzer_TIM, ENABLE);
		
		const uint16_t period = 100000 / 1000;   // 1000 гц.
		Buzzer_TIM->ARR = period - 1;
		Buzzer_TIM->CCR1 = period / 2;
		
		DisableGPIO();
		TIM_Cmd(Buzzer_TIM, ENABLE);
		
		m_xSemaphore = xSemaphoreCreateBinaryStatic(&m_xSemaphoreBuffer);
		xSemaphoreGive(m_xSemaphore);

		m_xSemaphoreHighPrio = xSemaphoreCreateBinaryStatic(&m_xSemaphoreHighPrioBuffer);
		xSemaphoreGive(m_xSemaphoreHighPrio);
		
		m_xSemaphorePause = xSemaphoreCreateBinaryStatic(&m_xSemaphorePauseBuffer);
	}
	
	void BeepHighPrio(const BeepSound* samples, uint8_t length)
	{
		xSemaphoreTake(m_xSemaphoreHighPrio, portMAX_DELAY);
		{
			bool busy = uxSemaphoreGetCount(m_xSemaphore) == 0;

			// Пищалка занята
			if(busy)
			{
				// Прервать
				xSemaphoreGive(m_xSemaphorePause);
			}
			
			xSemaphoreTake(m_xSemaphore, portMAX_DELAY);
			
			if (busy)
			{
				// Другой поток не успел захватить мьютекс
				if(uxSemaphoreGetCount(m_xSemaphorePause))
				{
					xSemaphoreTake(m_xSemaphorePause, portMAX_DELAY);
				}
			}
			
			if (busy)
			{
				vTaskDelay(70);
			}
				
			InnerPlaySound(samples, length);
			xSemaphoreGive(m_xSemaphore);
			
			xSemaphoreGive(m_xSemaphoreHighPrio);
		}
		
		// если был прерван чей-то звук, сделать дополнительную паузу в начале
	}
	
	// Воспроизводит звуковые семплы блокируя поток.
	void PlaySound(const BeepSound* samples, uint8_t length)
	{
		xSemaphoreTake(m_xSemaphore, portMAX_DELAY);
		{
			if (uxSemaphoreGetCount(m_xSemaphoreHighPrio))
			{
				InnerPlaySound(samples, length);
			}
			xSemaphoreGive(m_xSemaphore);
		}
	}
	
private:

	SemaphoreHandle_t m_xSemaphore;
	StaticSemaphore_t m_xSemaphoreBuffer;
	
	SemaphoreHandle_t m_xSemaphoreHighPrio;
	StaticSemaphore_t m_xSemaphoreHighPrioBuffer;
	
	SemaphoreHandle_t m_xSemaphorePause;
	StaticSemaphore_t m_xSemaphorePauseBuffer;
	
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
		GPIO_InitTypeDef GPIO_InitStructure = 
		{
			.GPIO_Pin = GPIO_Buzzer_Pin,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode = GPIO_Mode_AF_PP
		};
		GPIO_Init(GPIO_Buzzer, &GPIO_InitStructure);
	}
	
	void DisableGPIO()
	{
		GPIO_InitTypeDef GPIO_InitStructure = 
		{
			.GPIO_Pin = GPIO_Buzzer_Pin,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode = GPIO_Mode_IN_FLOATING
		};
		GPIO_Init(GPIO_Buzzer, &GPIO_InitStructure);
	}
	
	void InnerPlaySound(const BeepSound* samples, uint8_t length)
	{
		for (uint8_t i = 0; i < length; i++)
		{
			Freq(samples->Frequency);
			
			//vTaskDelay(samples->Duration / portTICK_PERIOD_MS);
			bool interrupted = (xSemaphoreTake(m_xSemaphorePause, samples->Duration / portTICK_PERIOD_MS) == pdTRUE);
			if (interrupted)
			{
				break;
			}
			samples++;
		}
		Freq(0);
	}
};

extern Buzzer g_buzzer;
