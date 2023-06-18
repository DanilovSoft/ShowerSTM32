#pragma once
#include "TaskBase.h"
#include "stm32f10x_gpio.h"
#include "Buzzer.h"
#include "HeaterTask.h"
#include "EepromHelper.h"
#include "Properties.h"
#include "TempSensorTask.h"
#include "HeaterTempLimit.h"
#include "ValveTask.h"
#include "ButtonDebounce.h"
#include "SensorSwitch.h"
#include "SensorPatternPress.h"

class ButtonsTask final : public TaskBase
{
public:
    
private:
    
    const static uint16_t SensorPowerOffDelayMsec = 1000;
    const static uint16_t SensorPowerOnDelayMsec = 300;
    
    static void PressSound()
    {
        static const BeepSound samples[]
        {
            BeepSound(150, 2000),
            BeepSound(50)
        }
        ;
        
        g_heaterTask.ResetBeepInterval();
        g_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
    }

    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        SensorSwitch sensorSwitch;
        ButtonDebounce debounceTempPlus(&Common::ButtonTempPlussPressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounceTempMinus(&Common::ButtonTempMinusPressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounceValve(&Common::ButtonValvePressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounceLongPressValve(&Common::ButtonValvePressed, g_properties.ButtonLongPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        SensorPatternPress sensorSwitchPatternPress;
        
        Stopwatch m_sensorStopwatch;
        m_sensorStopwatch.Reset();
        bool lastSensorSwitchIsOn = false;
        
        while (true)
        {   
            if (debounceTempPlus.IsPressed())
            {
                PressSound();
                TempPlus();
            }
        
            if (debounceTempMinus.IsPressed())
            {
                PressSound();
                TempMinus();
            }
        
            if (debounceValve.IsPressed())
            {
                PressSound();
                g_valveTask.OnButtonPress();
            }

            if (debounceLongPressValve.IsPressed())
            {
                if (Common::CircuitBreakerIsOn())
                {
                    g_heaterTask.IgnoreWaterLevelOnce(); // Принудительно включаем нагрев воды.
                }
                else
                {
                    g_valveTask.ForceOpenValve(); // Пытаемся принудительно включить набор воды.
                }           
            }
            
            if (sensorSwitch.IsOn())
            {
                if (lastSensorSwitchIsOn)
                {
                    if (!g_valveTask.OpenAllowed() && m_sensorStopwatch.GetElapsedMsec() > SensorPowerOffDelayMsec) // Клапан открывать пока нельзя — сенсор следует потушить.
                    {
                        sensorSwitch.PowerOff();   
                    }
                }
                else
                {
                    lastSensorSwitchIsOn = true;
                    m_sensorStopwatch.Reset();
                    g_valveTask.OpenValveRequest();
                }
            }
            else
            {
                if (lastSensorSwitchIsOn)
                {
                    lastSensorSwitchIsOn = false;
                    g_valveTask.CloseValveRequest();
                }
                else
                {
                    if (g_valveTask.OpenAllowed())
                    {
                        sensorSwitch.PowerOn();
                    }
                }
            }
        
            if (sensorSwitchPatternPress.IsPatternMatch())
            {
                sensorSwitchPatternPress.Reset();
                g_valveTask.ForceOpenValve(); // Пытаемся принудительно включить набор воды.
            }
            
            taskYIELD();
        }
    }
    
    void TempPlus()
    {
        uint8_t externalTemp;
        if (g_heaterTempLimit.TryGetAirTemperature(externalTemp))
        {
            if (g_writeProperties.Chart.IncrementInternalTemp(externalTemp))
            {
                g_eepromHelper.Save();
                g_properties.Chart.IncrementInternalTemp(externalTemp);
            }
        }
    }
    
    void TempMinus()
    {
        uint8_t air_temp;
        if (g_heaterTempLimit.TryGetAirTemperature(air_temp))
        {
            if (g_writeProperties.Chart.DecrementInternalTemp(air_temp))
            {
                g_eepromHelper.Save();
                g_properties.Chart.DecrementInternalTemp(air_temp);
            }
        }
    }
    
    void WaterPushButton()
    {
        g_valveTask.OnButtonPress();
    }
};

extern ButtonsTask g_buttonsTask;