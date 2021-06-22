#pragma once
#include "TaskBase.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"
#include "InitializationTask.h"

class WatchDogTask final : public TaskBase
{
public:
    
    bool GetWasReset()
    {
        return m_wasReset;
    }
    
    void Init()
    {
        // Был сброс по сторожевому таймеру.
        if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST))
        {	
            m_wasReset = true;
        
            // Clear reset flags.
            RCC_ClearFlag();
        }
    
        ////////// Watchdog на 4 секунды ////////////////
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
        IWDG_SetPrescaler(IWDG_Prescaler_32);  	// 1кгц.
        IWDG_SetReload(4000);  	// Не более 0xFFF (4095).
        IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
        IWDG_ReloadCounter();   // Сброс счетчика.
        IWDG_Enable();
    }
    
private:
    
    bool m_wasReset = false;
    
    void Run()
    {
        while (true)
        {
            IWDG_ReloadCounter();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
};

extern WatchDogTask* g_watchDogTask;
