#include "ButtonsTask.h"
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

ButtonsTask g_buttonsTask;

void ButtonsTask::Init()
{
	GPIO_InitTypeDef gpio_init = 
	{
		.GPIO_Pin = Button_Temp_Plus | Button_Temp_Minus | Button_Water | Button_SensorSwitch_OUT,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_IPD
	};
    GPIO_Init(Button_GPIO, &gpio_init);
}
	
void ButtonsTask::PressSound()
{
    const BeepSound samples[]
    {
        BeepSound(2000, 150),
        BeepSound(50)
    }
    ;
		
    g_heaterTask.ResetBeepTime();
    g_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
}

void ButtonsTask::LongPressSound()
{
	const BeepSound samples[]
	{
		BeepSound(2000, 500),
		BeepSound(50)
	}
	;
		
	g_heaterTask.ResetBeepTime();
	g_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
}

void ButtonsTask::Run()
{
	ButtonDebounce buttonTempPlus(Button_GPIO, Button_Temp_Plus, g_properties.ButtonPressTimeMs, g_properties.ButtonPressTimeMs * 2);
	ButtonDebounce buttonTempMinus(Button_GPIO, Button_Temp_Minus, g_properties.ButtonPressTimeMs, g_properties.ButtonPressTimeMs * 2);
	ButtonDebounce buttonValve(Button_GPIO, Button_Water, g_properties.ButtonPressTimeMs, g_properties.ButtonPressTimeMs * 2);
	ButtonDebounce longPressButtonValve(Button_GPIO, Button_Water, g_properties.ButtonLongPressTimeMs, g_properties.ButtonPressTimeMs * 2);
    WaterSensorButton sensorSwitch(Button_GPIO, Button_SensorSwitch_OUT);
   
    while (true)
    {
        if (buttonTempPlus.IsPressed())
        {
            PressSound();
            TempPlus();
        }
        
        if (buttonTempMinus.IsPressed())
        {
            PressSound();
            TempMinus();
        }
        
        if (buttonValve.IsPressed())
        {
            PressSound();
            g_valveTask.PushButton();
        }

	    if (longPressButtonValve.IsPressed())
	    {
		    LongPressSound();

		    if (CircuitBreakerIsOn())
		    {
                // Пытаемся принудительно включить нагрев воды.
			    g_heaterTask.IgnoreWaterLevelOnce();
		    }
		    else
		    {
			     // Пытаемся принудительно включить набор воды.
		    }		    
	    }
	    
        if (sensorSwitch.IsOn())
        {
            g_valveTask.SensorOn();
        }
        else
        {
            g_valveTask.SensorOff();
        }
		
        taskYIELD();
    }
}
	
void ButtonsTask::TempPlus()
{
	uint8_t externalTemp;
	if (g_heaterTempLimit.TryGetLastExternalTemp(externalTemp))
    {
        if (g_writeProperties.Chart.TempPlus(externalTemp))
        {
            g_eeprom.Save();
            g_properties.Chart.TempPlus(externalTemp);
        }
    }
}
	
void ButtonsTask::TempMinus()
{
	uint8_t externalTemp;
	if (g_heaterTempLimit.TryGetLastExternalTemp(externalTemp))
    {
        if (g_writeProperties.Chart.TempMinus(externalTemp))
        {
            g_eeprom.Save();
            g_properties.Chart.TempMinus(externalTemp);
        }
    }
}
	
void ButtonsTask::WaterPushButton()
{
    g_valveTask.PushButton();
}
