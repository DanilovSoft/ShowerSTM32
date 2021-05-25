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
#include "TickCounter.h"
#include "HeaterWatchdog.h"
#include "TempSensor.h"
#include "HeatingTimeLeft.h"
#include "HeaterTempLimit.h"

HeaterTask _heaterTask;

void HeaterTask::BeepHeating()
{
    const BeepSound samples[]
    {
        BeepSound(4000, 100),
        BeepSound(30),
    }
    ;
	
    if (_tickCounter.TimedOut(7000))
    {
        buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));
        _tickCounter.Reset();
    }
}

void HeaterTask::BeepTurnOff()
{
    const BeepSound samples[]
    {
        BeepSound(3000, 140),
        BeepSound(30),
        BeepSound(2400, 320),
        BeepSound(30)
    }
    ;
		
    buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));
    _tickCounter.Reset();
}

void HeaterTask::BeepTurnOn()
{
    const BeepSound samples[]
    {
        BeepSound(2400, 140),
        BeepSound(30),
        BeepSound(3000, 320),
        BeepSound(30)
    }
    ;
		
    buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));
    _tickCounter.Reset();
}

void HeaterTask::BeepReady()
{
    const BeepSound samples[]
    {
        BeepSound(4000, 80),
        BeepSound(100),
        BeepSound(4000, 80),
        BeepSound(30)
    }
    ;
		
    if (_tickCounter.TimedOut(4000))
    {
        if (WaterHeated())
        {
            buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));
            _tickCounter.Reset();
        }
    }
}

void HeaterTask::BeepTimeout()
{
    const BeepSound samples[]
    {
        BeepSound(4000, 300),
        BeepSound(100),
        BeepSound(4000, 300),
        BeepSound(100),
        BeepSound(4000, 300),
        BeepSound(30)
    }
    ;
		
    if (_tickCounter.TimedOut(7000))
    {
        buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));
        _tickCounter.Reset();
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
		
    TurnOffNoSound();
}

void HeaterTask::Run()
{
    _heaterWatchdog.Init();
    _tickCounter.Reset();
    _tempSensorTask.WaitFirstConversion();
    _waterLevelTask.WaitInitialization();
		
    while (true)
    {
    	// Если есть 220в.
        if (HasMainPower())
        {
        	// Если автомат был включён.
            if (!_heaterHasPower)
            {
                _heaterHasPower = true;
                _heaterWatchdog.Reset();
                _heaterWatchdog.ResetAbsolute();	// Сбросить абсолютный таймер.
                _tickCounter.Reset();
            }
				
            // Абсолютный таймаут можно сбросить только отключив автомат нагревателя.
            if (_heaterWatchdog.AbsoluteTimeout())
            {
            	// Выключить если нагреватель включен.
                if (GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET)
                {	
                    TurnOff();
                }
					
                BeepTimeout();
            }
            else
            {
            	// Если нагреватель включен.
                if (GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET)
                {
                	// Сброс таймера если включился нагреватель.
                    if (!_heaterEnabled)
                    {
                        _heaterEnabled = true;
                        _heaterWatchdog.Reset();		// Сброс таймера как только включился нагреватель.
                    }
						
                    // Таймаут нагрева.
                    if (_heaterWatchdog.TimeOut())
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
                    _heaterEnabled = false;
						
                    // Сбросить таймаут можно только отключив автомат нагревателя.
                    if (_heaterWatchdog.TimeOutOccurred)
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
            _heaterHasPower = false;
				
            // Выключить реле нагревателя (Отсутствует 220в)
            if (GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET)
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
	_heaterTempLimit.TryGetTargetTemperature(limitTemp);
    float internalTemp = _tempSensorTask.AverageInternalTemp;
		
    // Если уровень воды больше допустимого минимума И температура в баке меньше необходимой
    if (internalTemp < limitTemp && !_waterLevelTask.SensorIsBlocked && _waterLevelTask.DisplayingPercent >= Properties.MinimumWaterHeatingPercent)
    {
        TurnOn();
    }
}

void HeaterTask::ControlTurnOff()
{	
	uint8_t limitTemp;
	_heaterTempLimit.TryGetTargetTemperature(limitTemp);
    float internalTemp = _tempSensorTask.AverageInternalTemp;
			
    // Температура в баке выше порога отключения или уровень воды меньше допустимого
    if ((internalTemp >= limitTemp) || _waterLevelTask.DisplayingPercent < Properties.MinimumWaterHeatingPercent)
    {
        TurnOff();
        _heaterWatchdog.Reset();
    }
}

void HeaterTask::TurnOff()
{
    TurnOffNoSound();
    BeepTurnOff();	
}
	
void HeaterTask::TurnOffNoSound()
{
    GPIO_ResetBits(GPIO_Heater, GPIO_Pin_Heater);
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}

void HeaterTask::TurnOnNoSound()
{
    GPIO_SetBits(GPIO_Heater, GPIO_Pin_Heater);
    GPIO_ResetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
}
	
void HeaterTask::TurnOn()
{
    BeepTurnOn();
    _heatingTimeLeft.OnStartHeating();
    TurnOnNoSound();
}

bool HeaterTask::GetIsHeaterEnabled()
{
    return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
}
	
bool HeaterTask::WaterHeated()
{
	uint8_t limit;
    _heaterTempLimit.TryGetTargetTemperature(limit);
    return _tempSensorTask.AverageInternalTemp >= limit;
}
	
uint8_t HeaterTask::GetHeatingLimit()
{
	uint8_t limit;
	_heaterTempLimit.TryGetTargetTemperature(limit);
	return limit;
}
	
bool HeaterTask::GetTimeoutOccured()
{
    return _heaterWatchdog.TimeOutOccurred;
}
	
bool HeaterTask::GetAbsoluteTimeoutOccured()
{
    return _heaterWatchdog.AbsoluteTimeOutOccured;
}
	
void HeaterTask::ResetBeepTime()
{
    _tickCounter.Reset();
}
