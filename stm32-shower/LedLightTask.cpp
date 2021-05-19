#include "LedLightTask.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "Properties.h"

LedLightTask _ledLightTask;

void LedLightTask::Init()
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
    

void LedLightTask::Run()
{
    while (1)
    {	
        if (HasMainPower())
        {
            if (_turnedOn)
            {
                GPIO_InitTypeDef GPIO_InitStructure;
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_LED;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
                GPIO_Init(GPIO_LED, &GPIO_InitStructure);
                _turnedOn = false;
            }
        }
        else
        {
            if (!_turnedOn)
            {
                GPIO_InitTypeDef GPIO_InitStructure;
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_LED;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
                GPIO_Init(GPIO_LED, &GPIO_InitStructure);
                _turnedOn = true;
            }
        }
        taskYIELD();
    }
}