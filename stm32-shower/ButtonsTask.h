#pragma once
#include "iActiveTask.h"
#include "stm32f10x_gpio.h"
#include "Buzzer.h"
#include "HeaterTask.h"
#include "Eeprom.h"
#include "Properties.h"
#include "TempSensor.h"
#include "HeaterTempLimit.h"
#include "ValveTask.h"
#include "ButtonDebounce.h"
#include "WaterSensorButton.h"

class ButtonsTask final : public iActiveTask
{
private:
    
    void Init()
    {
        GPIO_InitTypeDef gpio_init = 
        {
            .GPIO_Pin = Button_Temp_Plus | Button_Temp_Minus | Button_Water_Pin | Button_SensorSwitch_OUT,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_IPD
        };
        GPIO_Init(Button_GPIO, &gpio_init);
    }
    
    void PressSound()
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

    void LongPressSound()
    {
        static const BeepSound samples[]
        {
            BeepSound(2000, 500),
            BeepSound(50)
        }
        ;
        
        g_heaterTask.ResetBeepInterval();
        g_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
    }

    void Run()
    {
        ButtonDebounce debounce_temp_plus(Button_GPIO, Button_Temp_Plus, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_temp_minus(Button_GPIO, Button_Temp_Minus, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_valve(Button_GPIO, Button_Water_Pin, g_properties.ButtonPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        ButtonDebounce debounce_long_valve(Button_GPIO, Button_Water_Pin, g_properties.ButtonLongPressTimeMsec, g_properties.ButtonPressTimeMsec * 2);
        WaterSensorButton sensor_switch(Button_GPIO, Button_SensorSwitch_OUT);
   
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
                    // Принудительно включаем нагрев воды.
                    g_heaterTask.IgnoreWaterLevelOnce();
                }
                else
                {
                    // Пытаемся принудительно включить набор воды.
                    g_valveTask.ForceOpenValve();
                }           
            }
        
            bool sensorIsOn = sensor_switch.UpdateAndGet();
            g_valveTask.UpdateSensorState(sensorIsOn);
        
            taskYIELD();
        }
    }
    
    void TempPlus()
    {
        uint8_t externalTemp;
        if (g_heaterTempLimit.TryGetLastAirTemperature(externalTemp))
        {
            if (g_writeProperties.Chart.TempPlus(externalTemp))
            {
                g_eeprom.Save();
                g_properties.Chart.TempPlus(externalTemp);
            }
        }
    }
    
    void TempMinus()
    {
        uint8_t air_temp;
        if (g_heaterTempLimit.TryGetLastAirTemperature(air_temp))
        {
            if (g_writeProperties.Chart.TempMinus(air_temp))
            {
                g_eeprom.Save();
                g_properties.Chart.TempMinus(air_temp);
            }
        }
    }
    
    void WaterPushButton()
    {
        g_valveTask.OnButtonPress();
    }
};

extern ButtonsTask g_buttonsTask;