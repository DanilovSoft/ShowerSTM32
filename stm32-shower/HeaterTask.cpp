#include "HeaterTask.h"
#include "stdint.h"
#include "Common.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"
#include "Settings.h"
#include "WaterLevelTask.h"
#include "Buzzer.h"
#include "Stopwatch.h"
#include "HeaterWatchdog.h"
#include "TempSensor.h"
#include "HeatingTimeLeft.h"
#include "HeaterTempLimit.h"

HeaterTask g_heaterTask;

void HeaterTask::BeepHeating()
{
	static const BeepSound samples[]
    {
        BeepSound(4000, 100),
        BeepSound(30),
    }
    ;
	
    if (m_beepStopwatch.TimedOut(7000))
    {
        g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));
        m_beepStopwatch.Reset();
    }
}

void HeaterTask::BeepTurnOff()
{
	static const BeepSound samples[]
    {
        BeepSound(3000, 140),
        BeepSound(30),
        BeepSound(2400, 320),
        BeepSound(30)
    }
    ;
		
    g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));
    m_beepStopwatch.Reset();
}

void HeaterTask::BeepTurnOn()
{
	static const BeepSound samples[]
    {
        BeepSound(2400, 140),
        BeepSound(30),
        BeepSound(3000, 320),
        BeepSound(30)
    }
    ;
		
    g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));
    m_beepStopwatch.Reset();
}

void HeaterTask::BeepReady()
{
	static const BeepSound samples[]
    {
        BeepSound(4000, 80),
        BeepSound(100),
        BeepSound(4000, 80),
        BeepSound(30)
    }
    ;
		
    if (m_beepStopwatch.TimedOut(4000))
    {
        if (WaterHeated())
        {
            g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));
            m_beepStopwatch.Reset();
        }
    }
}

void HeaterTask::BeepTimeout()
{
	static const BeepSound samples[]
	{
		BeepSound(4000, 300),
		BeepSound(100),
		BeepSound(4000, 300),
		BeepSound(100),
		BeepSound(4000, 300),
		BeepSound(30)
	}
	;
		
	if (m_beepStopwatch.TimedOut(7000))
	{
		g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));
		m_beepStopwatch.Reset();
	}
}
    
void HeaterTask::Init()
{
   // Нагреватель.
	GPIO_InitTypeDef gpio_init = 
	{
		.GPIO_Pin = GPIO_Pin_Heater,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_Out_PP,
	};
		
    GPIO_Init(GPIO_Heater, &gpio_init);
		
	// Светодиод нагревателя.
	gpio_init = 
	{
		.GPIO_Pin = GPIO_Heater_Led_Pin,
		.GPIO_Speed = GPIO_Speed_2MHz,	
		.GPIO_Mode = GPIO_Mode_Out_PP,
	};
	
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
    GPIO_Init(GPIO_Heater_Led, &gpio_init);
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
		
    TurnOffWithNoSound();
}

void HeaterTask::Run()
{
    m_heaterWatchdog.Init();
    m_beepStopwatch.Reset();
    g_tempSensorTask.WaitFirstConversion();
	
//	while (!_waterLevelTask.Initialized && !_forcedSessionRequired)
//	{
//		taskYIELD();
//	}
		
    while (true)
    {
    	// Если есть 220в.
        if (CircuitBreakerIsOn())
        {
        	// Если автомат был включён.
            if (!m_heaterHasPower)
            {
                m_heaterHasPower = true;
                m_heaterWatchdog.Reset();
                m_heaterWatchdog.ResetAbsolute();	// Сбросить абсолютный таймер.
                m_beepStopwatch.Reset();
            }
				
            // Абсолютный таймаут можно сбросить только отключив автомат нагревателя.
            if (m_heaterWatchdog.AbsoluteTimeout())
            {
            	// Выключить если нагреватель включен.
                if(HeaterIsOn())
                {	
                    TurnOff();
                }
					
                BeepTimeout();
            }
            else
            {
            	// Если нагреватель включен.
                if(HeaterIsOn())
                {
                	// Сброс таймера если включился нагреватель.
                    if (!m_heaterEnabled)
                    {
                        m_heaterEnabled = true;
                        m_heaterWatchdog.Reset();		// Сброс таймера как только включился нагреватель.
                    }
						
                    // Таймаут нагрева.
                    if (m_heaterWatchdog.TimeOut())
                    {
                        TurnOff();
                        BeepTimeout();
                    }
                    else
                    {
                        BeepHeating();
                        ControlTurnOff();
                    }
                }
                else
                {
                    m_heaterEnabled = false;
						
                    // Сбросить таймаут можно только отключив автомат нагревателя.
                    if (m_heaterWatchdog.IsTimeoutOccurred())
                    {
                        BeepTimeout();
                    }
                    else
                    {
                        BeepReady();
                        ControlTurnOn();
                    }
                }
            }
        }
        else
        {
            m_heaterHasPower = false;
	        m_forcedSessionRequired = false; // Автомат выключен = сессия завершена.
				
            // Выключить реле нагревателя (Отсутствует 220в).
            if(HeaterIsOn())
            {
                TurnOff();
            }
        }

        taskYIELD();
    }
}

void HeaterTask::ControlTurnOn()
{	
	uint8_t limitTemp;
	if (g_heaterTempLimit.TryGetTargetTemperature(limitTemp))
	{
		float internalTemp = g_tempSensorTask.AverageInternalTemp;
	
		
		// Если уровень воды больше допустимого минимума И температура в баке меньше необходимой
		if(internalTemp < limitTemp && !g_waterLevelTask.SensorIsBlocked && g_waterLevelTask.DisplayingPercent >= g_properties.MinimumWaterHeatingPercent)
		{
			TurnOn();
		}
	}
}

void HeaterTask::ControlTurnOff()
{	
	uint8_t limitTemp;
	g_heaterTempLimit.TryGetTargetTemperature(limitTemp);
    float internalTemp = g_tempSensorTask.AverageInternalTemp;
			
    // Температура в баке выше порога отключения или уровень воды меньше допустимого
    if ((internalTemp >= limitTemp) || g_waterLevelTask.DisplayingPercent < g_properties.MinimumWaterHeatingPercent)
    {
        TurnOff();
        m_heaterWatchdog.Reset();
    }
}

void HeaterTask::TurnOff()
{
    TurnOffWithNoSound();
    BeepTurnOff();	
}
	
void HeaterTask::TurnOffWithNoSound()
{
    GPIO_ResetBits(GPIO_Heater, GPIO_Pin_Heater);
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}

void HeaterTask::TurnOnWithNoSound()
{
    GPIO_SetBits(GPIO_Heater, GPIO_Pin_Heater);
    GPIO_ResetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}
	
void HeaterTask::TurnOn()
{
    BeepTurnOn();
    g_heatingTimeLeft.OnStartHeating();
    TurnOnWithNoSound();
}

bool HeaterTask::GetIsHeaterEnabled()
{
    return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
}
	
bool HeaterTask::WaterHeated()
{
	uint8_t limit;
    g_heaterTempLimit.TryGetTargetTemperature(limit);
    return g_tempSensorTask.AverageInternalTemp >= limit;
}
	
uint8_t HeaterTask::GetHeatingLimit()
{
	uint8_t limit;
	g_heaterTempLimit.TryGetTargetTemperature(limit);
	return limit;
}
	
bool HeaterTask::GetTimeoutOccured() const
{
    return m_heaterWatchdog.IsTimeoutOccurred();
}
	
bool HeaterTask::GetAbsoluteTimeoutOccured() const
{
    return m_heaterWatchdog.IsAbsoluteTimeoutOccured();
}
	
void HeaterTask::ResetBeepTime()
{
    m_beepStopwatch.Reset();
}

void HeaterTask::IgnoreWaterLevelOnce()
{
	// Просим другой поток выполнить принудительное включение нагрева.
	m_forcedSessionRequired = true;
}