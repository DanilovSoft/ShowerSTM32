#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "Buzzer.h"
#include "TempSensor.h"
#include "HeatingTimeLeft.h"
#include "HeaterTempLimit.h"
#include "Common.h"

// Выключает питание ТЭНа и тушит светодиод.
void HeaterTask::TurnOff()
{
	GPIO_ResetBits(GPIO_Heater, GPIO_Pin_Heater);
	GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}

// Включает питание ТЭНа и зажигает светодиод.
void HeaterTask::TurnOn()
{
	GPIO_SetBits(GPIO_Heater, GPIO_Pin_Heater);
	GPIO_ResetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}

// Воспроизводит звук отключения питания ТЭНа.
// Блокирует поток на время воспроизведения.
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

// Воспроизводит звук подачи питания на ТЭН.
// Блокирует поток на время воспроизведения.
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

// Воспроизводит звук активного процесса нагрева.
// Звук воспроизводится только по прошествию интервала.
// Блокирует поток на время воспроизведения.
void HeaterTask::PeriodicBeepHeating()
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

// Воспроизводит звук если вода нагрета до требуемой температуры.
// Звук воспроизводится только по прошествию интервала.
// Блокирует поток на время воспроизведения.
void HeaterTask::PeriodicBeepIfWaterHeated()
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

// Воспроизводит звук аварии.
// Звук воспроизводится только по прошествию интервала.
// Блокирует поток на время воспроизведения.
void HeaterTask::PeriodicBeepTimeout()
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
		
    TurnOff();
}

void HeaterTask::Run()
{
    m_heaterWatchdog.Init();
    m_beepStopwatch.Reset();
    g_tempSensorTask.WaitFirstConversion();
	
    while (true)
    {
    	// Если есть 220в.
        if(Common::CircuitBreakerIsOn())
        {
            if (!m_circuitBreakerIsOn)
            {
                // Запомним что автомат теперь включен.
	            m_circuitBreakerIsOn = true;
	            
                m_heaterWatchdog.ResetSession();
                m_heaterWatchdog.ResetAbsolute();	// Абсолютный таймаут можно сбросить только отключив автомат нагревателя.
                m_beepStopwatch.Reset();
            }
				
            if (!m_heaterWatchdog.AbsoluteTimeout())
            {
	            // Аварии нет - можно продолжать.
	            
	            // Если реле нагревателя включено.
				if(Common::HeaterIsOn())
	            {
		            // Сброс таймера если включился нагреватель.
					if(!m_heaterEnabled)
		            {
			            // Запомним что нагреватель теперь включен.
			            m_heaterEnabled = true;
			            m_heaterWatchdog.ResetSession(); // Сброс таймаута сессии.
		            }
						
		            // Таймаут нагрева.
		            if(!m_heaterWatchdog.TimeOut())
		            {
			            PeriodicBeepHeating();
			            ControlTurnOff();
		            }
		            else
		            {
			            TurnOffHeaterWithSound();
			            PeriodicBeepTimeout();
		            }
	            }
	            else
	            {
		            m_heaterEnabled = false;
						
		            // Сбросить таймаут можно только отключив автомат нагревателя.
		            if(m_heaterWatchdog.IsSessionTimeoutOccurred())
		            {
			            PeriodicBeepTimeout();
		            }
		            else
		            {
			            PeriodicBeepIfWaterHeated();
			            ControlTurnOn();
		            }
	            }
            }
            else
            {
	            // Был достигнут абсолютный таймаут.
	            
	            // Выключить если нагреватель включен.
                if(Common::HeaterIsOn())
	            {	
		            TurnOffHeaterWithSound();
	            }
					
	            PeriodicBeepTimeout();
            }
        }
        else
        {
	        // Автомат выключен.
	        
	        m_circuitBreakerIsOn = false;  // Запомним что автомат отключен.
	        m_forcedSessionRequired = false; // Автомат отключен = сессия завершена.
				
            // Выключить реле нагревателя (Отсутствует 220в).
            if(Common::HeaterIsOn())
	        {
		        TurnOffHeaterWithSound();
	        }
        }

        taskYIELD();
    }
}

// Безусловное включение ТЭНа.
void HeaterTask::TurnOnHeaterWithSound()
{
	BeepTurnOn();
	g_heatingTimeLeft.OnStartHeating();
	TurnOn();
}

void HeaterTask::ControlTurnOn()
{
	uint8_t targetTemp;
	if (g_heaterTempLimit.TryGetTargetTemperature(targetTemp))
	{
		float internalTemp = g_tempSensorTask.AverageInternalTemp;
		
		if (m_forcedSessionRequired)
		{
			// Если температура в баке меньше необходимой.
			if(internalTemp < targetTemp)
			{
				TurnOnHeaterWithSound();
			}
		}
		else if (g_waterLevelTask.GetIsInitialized())
		{
			// Если уровень воды больше допустимого минимума И температура в баке меньше необходимой.
			if(internalTemp < targetTemp && !g_waterLevelTask.SensorIsBlocked && g_waterLevelTask.DisplayingPercent >= g_properties.MinimumWaterHeatingPercent)
			{
				TurnOnHeaterWithSound();
			}
		}
	}
}

// Безусловное отключение ТЭНа.
void HeaterTask::TurnOffHeaterWithSound()
{
	TurnOff();
	BeepTurnOff();	
}

// Определяет пора ли выключить нагрев.
void HeaterTask::ControlTurnOff()
{	
	uint8_t targetTemp;
	if (g_heaterTempLimit.TryGetTargetTemperature(targetTemp))
	{
		float internalTemp = g_tempSensorTask.AverageInternalTemp;
		
		if (m_forcedSessionRequired)
		{
			if (internalTemp >= targetTemp)
			{
				TurnOffHeaterWithSound();     
				m_heaterWatchdog.ResetSession();
			}
		}
		else if (g_waterLevelTask.GetIsInitialized())
		{
			// Температура в баке выше порога отключения ИЛИ уровень воды меньше допустимого.
			if((internalTemp >= targetTemp) || g_waterLevelTask.DisplayingPercent < g_properties.MinimumWaterHeatingPercent)
			{
				TurnOffHeaterWithSound();      // Безусловное отключение ТЭНа.
				m_heaterWatchdog.ResetSession();
			}
		}
	}
}

bool HeaterTask::GetIsHeaterEnabled()
{
    return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
}
	
// True если вода нагрета до нужного уровня.
bool HeaterTask::WaterHeated()
{
	uint8_t targetTemp;
	if (g_heaterTempLimit.TryGetTargetTemperature(targetTemp))
	{
		return g_tempSensorTask.AverageInternalTemp >= targetTemp;
	}
	else
	{
		return false;
	}
}
	
uint8_t HeaterTask::GetHeatingLimit()
{
	uint8_t limit;
	g_heaterTempLimit.TryGetTargetTemperature(limit);
	return limit;
}
	
bool HeaterTask::GetTimeoutOccured()
{
    return m_heaterWatchdog.IsSessionTimeoutOccurred();
}
	
bool HeaterTask::GetAbsoluteTimeoutOccured()
{
    return m_heaterWatchdog.IsAbsoluteTimeoutOccured();
}
	
// Сбрасывает время периодического звукового сигнала на начало.
void HeaterTask::ResetBeepInterval()
{
    m_beepStopwatch.Reset();
}

// Разрешает включить нагрев воды для текущей сессии (сессия — пока не выключат автомат).
void HeaterTask::IgnoreWaterLevelOnce()
{
	// Просим другой поток выполнить принудительное включение нагрева.
	m_forcedSessionRequired = true;
}
