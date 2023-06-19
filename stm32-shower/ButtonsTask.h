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
    
    enum SensorState
    {
        IsOff,
        OpenValveRequest,
        PowerOff
    };
    
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
        Stopwatch sensorSwitchPowerOffStopwatch; // Выключает сенсор с небольшой задержкой; возвращает питание сенсору с небольшой задержкой.
        auto sensorState = ButtonsTask::IsOff;
        
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
                switch (sensorState)
                {
                case ButtonsTask::IsOff:
                    {
                        sensorState = ButtonsTask::OpenValveRequest;
                        sensorSwitchPowerOffStopwatch.Reset();
                        g_valveTask.OpenValveRequest();
                    }
                    break;
                case ButtonsTask::OpenValveRequest:
                    {
                        if (g_valveTask.IsWaitingRequest())
                        {
                            if (sensorSwitchPowerOffStopwatch.GetElapsedMsec() > SensorPowerOffDelayMsec) // Сенсор после включения может самостоятельно потухнуть не раньше чем через 1 сек.
                            {
                                sensorSwitch.PowerOff();
                                sensorState = ButtonsTask::PowerOff;
                                sensorSwitchPowerOffStopwatch.Reset();
                            }
                        }
                    }
                    break;
                case ButtonsTask::PowerOff: // Сюда можем попасть если модуль ещё остался включен из-за остаточной ёмкости.
                    break;
                default:
                    break;
                }
            }
            else
            {
                switch (sensorState)
                {
                case ButtonsTask::OpenValveRequest:
                    {
                        sensorState = ButtonsTask::IsOff;
                        g_valveTask.CloseValveRequest();
                    }
                    break;
                case ButtonsTask::PowerOff:
                    {
                        if (sensorSwitchPowerOffStopwatch.GetElapsedMsec() > 300)
                        {
                            sensorSwitch.PowerOn();
                            sensorState = ButtonsTask::IsOff;
                        }
                    }
                    break;
                case ButtonsTask::IsOff:
                    break;
                default:
                    break;
                }
            }
        
            if (sensorSwitchPatternPress.IsPatternMatch())
            {
                sensorSwitchPatternPress.Reset(); // Сбрасываем флаг о том что набрали паттерн.
                sensorSwitchPowerOffStopwatch.Reset();
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