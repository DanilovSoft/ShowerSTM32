#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "Buzzer.h"
#include "TempSensor.h"
#include "HeatingTimeLeft.h"
#include "HeaterTempLimit.h"

HeaterTask g_heaterTask;

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
	
//	while (!g_waterLevelTask.Initialized && !m_forcedSessionRequired)
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
                    TurnOffWithSound();
                }
					
                PeriodicBeepTimeout();
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
                        TurnOffWithSound();
                        PeriodicBeepTimeout();
                    }
                    else
                    {
                        PeriodicBeepHeating();
                        ControlTurnOff();
                    }
                }
                else
                {
                    m_heaterEnabled = false;
						
                    // Сбросить таймаут можно только отключив автомат нагревателя.
                    if (m_heaterWatchdog.IsTimeoutOccurred())
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
        }
        else
        {
            m_heaterHasPower = false;
	        m_forcedSessionRequired = false; // Автомат выключен = сессия завершена.
				
            // Выключить реле нагревателя (Отсутствует 220в).
            if(HeaterIsOn())
            {
                TurnOffWithSound();
            }
        }

        taskYIELD();
    }
}

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

void HeaterTask::ControlTurnOn()
{	
	uint8_t limitTemp;
	if (g_heaterTempLimit.TryGetTargetTemperature(limitTemp))
	{
		float internalTemp = g_tempSensorTask.AverageInternalTemp;
	
		
		// Если уровень воды больше допустимого минимума И температура в баке меньше необходимой
		if(internalTemp < limitTemp && !g_waterLevelTask.SensorIsBlocked && g_waterLevelTask.DisplayingPercent >= g_properties.MinimumWaterHeatingPercent)
		{
			TurnOnWithSound(); // Безусловное включение.
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
        TurnOffWithSound(); // Безусловное отключение ТЭНа.
        m_heaterWatchdog.Reset();
    }
}

// Безусловное отключение ТЭНа.
void HeaterTask::TurnOffWithSound()
{
    TurnOff();
    BeepTurnOff();	
}

// Безусловное включение ТЭНа.
void HeaterTask::TurnOnWithSound()
{
    BeepTurnOn();
    g_heatingTimeLeft.OnStartHeating();
    TurnOn();
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
    return m_heaterWatchdog.IsTimeoutOccurred();
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
