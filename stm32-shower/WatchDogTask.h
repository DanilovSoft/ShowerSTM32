#pragma once
#include "iActiveTask.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"

class WatchDogTask final : public iActiveTask
{
public:
	
	bool GetWasReset()
	{
		return m_wasReset;
	}
	
private:
	
	bool m_wasReset = false;
	
	void Init()
	{
		// Был сброс по сторожевому таймеру.
		if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST))
		{	
			m_wasReset = true;
		
			// Clear reset flags.
			RCC_ClearFlag();
		}
	
		////////// Watchdog на 4 секунды ////////////////////
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
		IWDG_SetPrescaler(IWDG_Prescaler_32); 	// 1кгц.
		IWDG_SetReload(4000); 	// Не более 0xFFF
		IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
		IWDG_ReloadCounter();  // Сброс счетчика.
		IWDG_Enable();
	}
	
	void Run()
	{
		while (true)
		{
			IWDG_ReloadCounter();
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
	}
};

extern WatchDogTask g_watchDogTask;
