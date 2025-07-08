#include "stm32f10x_tim.h"
#include "Common.h"

extern "C"
{
    volatile SoundSensorTimerState TIM_CAPTURE_STATE = SoundSensorTimerState::WAITING_RISING_EDGE;
    volatile uint16_t TIM_CAPTURE_VAL = 0;
    
    // Прерывание таймера — обнаружен Rising или Falling фронты
    void TIM1_CC_IRQHandler()
    {	
        TIM_ClearITPendingBit(WL_TIM, TIM_IT_CC1);
        
        if (TIM_CAPTURE_STATE & SoundSensorTimerState::COMPLETED)
        {
            return;
        }
        
        // Falling edge
        if (TIM_CAPTURE_STATE & SoundSensorTimerState::WAITING_FALLING_EDGE)
        {
            TIM_CAPTURE_VAL = TIM_GetCapture1(WL_TIM);
            TIM_CAPTURE_STATE = SoundSensorTimerState::COMPLETED;
            TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Rising);
        }
        else // Rising Edge
        {	
            TIM_CAPTURE_STATE = SoundSensorTimerState::WAITING_FALLING_EDGE;
            TIM_SetCounter(WL_TIM, 0);
            TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Falling); // CC1P=1 is set to decrease along the capture.
        }
    }
    
    // Прерывание — переполнение таймера.
    void TIM1_UP_IRQHandler()
    {
        TIM_ClearITPendingBit(WL_TIM, TIM_IT_Update);
        
        if (TIM_CAPTURE_STATE & SoundSensorTimerState::COMPLETED)
        {
            return;
        }
        
        TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Rising); // Restore initial polarity.
        TIM_CAPTURE_STATE = (SoundSensorTimerState)(SoundSensorTimerState::OVERFLOW | SoundSensorTimerState::COMPLETED);
    }
}