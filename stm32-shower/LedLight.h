#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "Properties.h"
#include "iActiveTask.h"
#include "stm32f10x_iwdg.h"

#define GPIO_LED		(GPIOB)
#define GPIO_Pin_LED	(GPIO_Pin_7)
#define LED_TIM			(TIM4)

class LedLight : public iActiveTask
{
	bool turnedOn;
	
	void Init()
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_LED;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(GPIO_LED, &GPIO_InitStructure);

    	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    	TIM_TimeBaseInitStruct.TIM_Prescaler = SystemCoreClock / 1000000 - 1;
    	TIM_TimeBaseInitStruct.TIM_Period = 100 - 1;
    	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;     // без  делителя
    	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    	TIM_TimeBaseInit(LED_TIM, &TIM_TimeBaseInitStruct);
        
    	TIM_OCInitTypeDef TIM_OCInitStruct;
    	TIM_OCStructInit(&TIM_OCInitStruct);
    	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    	TIM_OCInitStruct.TIM_Pulse = (TIM_TimeBaseInitStruct.TIM_Period + 1) / 100 * Properties.LightBrightness;
    	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    	TIM_OC2Init(LED_TIM, &TIM_OCInitStruct);
    	TIM_Cmd(LED_TIM, ENABLE);
	}
    
	void Run()
	{
		while (true)
		{	
			if (HasMainPower())
			{
				if (turnedOn)
				{
					GPIO_InitTypeDef GPIO_InitStructure;
					GPIO_InitStructure.GPIO_Pin = GPIO_Pin_LED;
					GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
					GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
					GPIO_Init(GPIO_LED, &GPIO_InitStructure);
					turnedOn = false;
				}
			}
			else
			{
				if (!turnedOn)
				{
					GPIO_InitTypeDef GPIO_InitStructure;
					GPIO_InitStructure.GPIO_Pin = GPIO_Pin_LED;
					GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
					GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
					GPIO_Init(GPIO_LED, &GPIO_InitStructure);
					turnedOn = true;
				}
			}
			taskYIELD();
		}
	}
};

extern LedLight ledLight;