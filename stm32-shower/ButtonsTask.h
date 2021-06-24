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

class ButtonsTask final : public TaskBase
{
public:
    
    ButtonsTask(PropertyStruct* const properties)
        : m_properties(properties)
    {
        Debug::Assert(properties != NULL);
    }
    
private:
    
    PropertyStruct* const m_properties;
    
    static void PressSound()
    {
        BeepSound samples[]
        {
            BeepSound(2000, 150),
            BeepSound(50)
        }
        ;
        
        g_heaterTask.ResetBeepInterval();
        g_buzzer->BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
    }

    void Run()
    {
        Common::AssertAllTasksInitialized();
        
        ButtonDebounce debounce_temp_plus(&Common::ButtonTempPlussPressed, m_properties->ButtonPressTimeMsec, m_properties->ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_temp_minus(&Common::ButtonTempMinusPressed, m_properties->ButtonPressTimeMsec, m_properties->ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_valve(&Common::ButtonValvePressed, m_properties->ButtonPressTimeMsec, m_properties->ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_long_valve(&Common::ButtonValvePressed, m_properties->ButtonLongPressTimeMsec, m_properties->ButtonPressTimeMsec * 2);
        WaterSensorButton sensor_switch(&Common::ButtonSensorSwitchIsOn, m_properties);
   
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
                g_valveTask->OnButtonPress();
            }

            if (debounce_long_valve.UpdateAndGet())
            {
                if (Common::CircuitBreakerIsOn())
                {
                    // Принудительно включаем нагрев воды.
                    g_heaterTask.IgnoreWaterLevelOnce();
                }
                else
                {
                    // Пытаемся принудительно включить набор воды.
                    g_valveTask->ForceOpenValve();
                }           
            }
        
            bool sensorIsOn = sensor_switch.UpdateAndGet();
            g_valveTask->UpdateSensorState(sensorIsOn);
        
            taskYIELD();
        }
    }
    
    void TempPlus()
    {
        uint8_t externalTemp;
        if (g_heaterTempLimit->TryGetAirTemperature(externalTemp))
        {
            if (g_writeProperties.Chart.TempPlus(externalTemp))
            {
                g_eepromHelper.Save();
                m_properties->Chart.TempPlus(externalTemp);
            }
        }
    }
    
    void TempMinus()
    {
        uint8_t air_temp;
        if (g_heaterTempLimit->TryGetAirTemperature(air_temp))
        {
            if (g_writeProperties.Chart.TempMinus(air_temp))
            {
                g_eepromHelper.Save();
                m_properties->Chart.TempMinus(air_temp);
            }
        }
    }
    
    void WaterPushButton()
    {
        g_valveTask->OnButtonPress();
    }
};

extern ButtonsTask* g_buttonsTask;