#include "stm32f10x_tim.h"
#include "Common.h"

extern "C"
{
//    enum TIM_CAPTURE_STATE {
//        WL_NONE = 0,
//        WL_SUCCESS     = 1,
//        WL_OVERFLOW    = 2,
//        WL_RISING_EDGE = 4
//    };
    
    volatile uint8_t TIM_CAPTURE_STATE = 0;     // Input capture state.
    volatile uint16_t TIM_CAPTURE_VALUE = 0;
    
    // Событие переполнение таймера.
    void TIM1_UP_IRQHandler()
    {
        TIM_ClearITPendingBit(WL_TIM, TIM_IT_Update);
        
        if (!(TIM_CAPTURE_STATE & WL_SUCCESS))
        {
            TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Rising);	// Restore initial polarity.
            TIM_CAPTURE_STATE = (WL_OVERFLOW | WL_SUCCESS);
        }
    }
    
    void TIM1_CC_IRQHandler()
    {	
        TIM_ClearITPendingBit(WL_TIM, TIM_IT_CC1);
        
        if (TIM_CAPTURE_STATE & WL_SUCCESS)
        {
            return;
        }
           
        // Falling edge.
        if (TIM_CAPTURE_STATE & WL_RISING_EDGE)
        {
            TIM_CAPTURE_VALUE = TIM_GetCapture1(WL_TIM);
            TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Rising);
            TIM_CAPTURE_STATE = WL_SUCCESS;
        }
        else
        {	
            // Rising Edge.
            TIM_SetCounter(WL_TIM, 0);
            TIM_OC1PolarityConfig(WL_TIM, TIM_ICPolarity_Falling); // CC1P=1 is set to decrease along the capture.
            TIM_CAPTURE_STATE = WL_RISING_EDGE;
        }
    }
}
