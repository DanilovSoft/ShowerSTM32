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
#include "WaterSensorButton.h"
#include "SensorPatternPress.h"

class ButtonsTask final : public TaskBase
{
public:
    
private:
    
    static void PressSound()
    {
        static const BeepSound samples[]
        {
            BeepSound(2000, 150),
            BeepSound(50)
        }
        ;
        
        g_heaterTask.ResetBeepInterval();
        g_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
    }

    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        ButtonDebounce debounce_temp_plus(&Common::ButtonTempPlussPressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_temp_minus(&Common::ButtonTempMinusPressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_valve(&Common::ButtonValvePressed, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_long_valve(&Common::ButtonValvePressed, g_properties.ButtonLongPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        WaterSensorButton sensor_switch(&Common::ButtonSensorSwitchIsOn);
        SensorPatternPress sensor_switch_pattern(&Common::ButtonSensorSwitchIsOn);
        
        while (true)
        {
            if (debounce_temp_plus.UpdateAndGet())
            {
                PressSound();
                TempPlus();
            }
        
            if (debounce_temp_minus.UpdateAndGet())
            {
                PressSound();
                TempMinus();
            }
        
            if (debounce_valve.UpdateAndGet())
            {
                PressSound();
                g_valveTask.OnButtonPress();
            }

            if (debounce_long_valve.UpdateAndGet())
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
            
            sensor_switch_pattern.Update();
            if (sensor_switch_pattern.GetLogicalPressed())
            {
                
            }
            
            bool sensorIsOn = sensor_switch.UpdateAndGet();
            g_valveTask.UpdateSensorState(sensorIsOn);
        
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