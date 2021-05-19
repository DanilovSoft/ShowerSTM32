#include "WatchDogTask.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"



WatchDogTask _watchDogTask;

void WatchDogTask::Init()
{
	// Был сброс по ватчдогу
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST))
    {	
        WasReset = true;
		
    	/* Clear reset flags */
        RCC_ClearFlag();
    }
	
	////////// Watchdog на 4 секунды ////////////////////
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_32);	// 1кгц
    IWDG_SetReload(4000);	// Не более 0xFFF
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
    IWDG_ReloadCounter(); // сброс счетчика
    IWDG_Enable();
}
	

void WatchDogTask::Run()
{
    while (1)
    {
        IWDG_ReloadCounter();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}