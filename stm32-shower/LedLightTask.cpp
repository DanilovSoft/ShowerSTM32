#include "LedLightTask.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "Properties.h"

LedLightTask g_ledLightTask;

void LedLightTask::Init()
{
	// Тактуем таймер от шины APB1.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure = 
	{
		.GPIO_Pin = GPIO_Pin_LED,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_IN_FLOATING
	};
    GPIO_Init(GPIO_LED, &GPIO_InitStructure);

	// Частота таймера - 1 мегагерц.
	uint16_t prescaler = SystemCoreClock / 1000000 - 1;
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = 
	{
		.TIM_Prescaler = prescaler,
		.TIM_CounterMode = TIM_CounterMode_Up,
		.TIM_Period = 100 - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,  // без  делителя.
		.TIM_RepetitionCounter = 0,
	};
    TIM_TimeBaseInit(LED_TIM, &TIM_TimeBaseInitStruct);
    
	// Скважность от 0 до 100%.
	uint16_t pulse = (TIM_TimeBaseInitStruct.TIM_Period + 1) / 100 * g_properties.LightBrightness;
	
	TIM_OCInitTypeDef TIM_OCInitStruct = 
	{
		.TIM_OCMode = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OutputNState = TIM_OutputNState_Disable,
		.TIM_Pulse = pulse,
		.TIM_OCPolarity = TIM_OCPolarity_High,
		.TIM_OCNPolarity = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
		.TIM_OCNIdleState = TIM_OCNIdleState_Reset
	};
    TIM_OC2Init(LED_TIM, &TIM_OCInitStruct);

	// Включаем таймер.
    TIM_Cmd(LED_TIM, ENABLE);
}

void LedLightTask::Run()
{
    while (true)
    {	
	    if (CircuitBreakerIsOn())
	    {
		    if (m_turnedOn)
		    {
			    m_turnedOn = false;
	            
			    GPIO_InitTypeDef GPIO_InitStructure = 
			    {
				    .GPIO_Pin = GPIO_Pin_LED,
				    .GPIO_Speed = GPIO_Speed_2MHz,
				    .GPIO_Mode = GPIO_Mode_IN_FLOATING
			    };
			    GPIO_Init(GPIO_LED, &GPIO_InitStructure);
		    }
	    }
	    else if (!m_turnedOn)
	    {
		    m_turnedOn = true;
            
		    GPIO_InitTypeDef GPIO_InitStructure = 
		    {
			    .GPIO_Pin = GPIO_Pin_LED,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode = GPIO_Mode_AF_PP
		    };
		    GPIO_Init(GPIO_LED, &GPIO_InitStructure);
	    }
	    
        taskYIELD();
    }
}