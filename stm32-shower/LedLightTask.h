#pragma once
#include "TaskBase.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "Properties.h"
#include "HeaterTask.h"
#include "InitializationTask.h"

class LedLightTask final : public TaskBase
{
public:
    
    void Init()
    {
        // Тактуем таймер от шины APB1.
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_Pin_LED,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_IN_FLOATING
        };
        GPIO_Init(GPIO_LED, &gpio_init_struct);

        // Частота таймера - 1 мегагерц.
        uint16_t prescaler = SystemCoreClock / 1000000 - 1;
    
        TIM_TimeBaseInitTypeDef tim_time_base_init_struct = 
        {
            .TIM_Prescaler = prescaler,
            .TIM_CounterMode = TIM_CounterMode_Up,
            .TIM_Period = 100 - 1,
            .TIM_ClockDivision = TIM_CKD_DIV1, 
            // без  делителя.
            .TIM_RepetitionCounter = 0,
        };
        TIM_TimeBaseInit(LED_TIM, &tim_time_base_init_struct);
    
        // Скважность от 0 до 100%.
        uint16_t pulse = (tim_time_base_init_struct.TIM_Period + 1) / 100 * g_properties.LightBrightness;
    
        TIM_OCInitTypeDef tim_oc_init_struct = 
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
        TIM_OC2Init(LED_TIM, &tim_oc_init_struct);

        // Включаем таймер.
        TIM_Cmd(LED_TIM, ENABLE);
    }
    
private:

    void Run()
    {
        g_initializationTask.WaitForPropertiesInitialization();
     
        bool light_is_on;
        
        while (true)
        {	
            if (Common::CircuitBreakerIsOn())
            {
                if (light_is_on)
                {
                    light_is_on = false;
                    Common::TurnOffLight();
                }
            }
            else if (!light_is_on)
            {
                light_is_on = true;
                Common::TurnOnLight();
            }
        
            taskYIELD();
        }
    }
};

extern LedLightTask g_ledLightTask;
