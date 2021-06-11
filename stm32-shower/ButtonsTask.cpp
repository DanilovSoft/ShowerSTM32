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

ButtonsTask _buttonsTask;

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
		
    _heaterTask.ResetBeepTime();
    _buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
}

void ButtonsTask::LongPressSound()
{
	const BeepSound samples[]
	{
		BeepSound(2000, 500),
		BeepSound(50)
	}
	;
		
	_heaterTask.ResetBeepTime();
	_buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
}

void ButtonsTask::Run()
{
	ButtonDebounce buttonTempPlus(Button_GPIO, Button_Temp_Plus, Properties.ButtonPressTimeMs, Properties.ButtonPressTimeMs * 2);
	ButtonDebounce buttonTempMinus(Button_GPIO, Button_Temp_Minus, Properties.ButtonPressTimeMs, Properties.ButtonPressTimeMs * 2);
	ButtonDebounce buttonValve(Button_GPIO, Button_Water, Properties.ButtonPressTimeMs, Properties.ButtonPressTimeMs * 2);
	ButtonDebounce longPressButtonValve(Button_GPIO, Button_Water, Properties.ButtonLongPressTimeMs, Properties.ButtonPressTimeMs * 2);
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
            _valveTask.PushButton();
        }

	    if (longPressButtonValve.IsPressed())
	    {
		    LongPressSound();

		    if (CircuitBreakerIsOn())
		    {
                // Пытаемся принудительно включить нагрев воды.
			    _heaterTask.IgnoreWaterLevelOnce();
		    }
		    else
		    {
			     // Пытаемся принудительно включить набор воды.
		    }		    
	    }
	    
        if (sensorSwitch.IsOn())
        {
            _valveTask.SensorOn();
        }
        else
        {
            _valveTask.SensorOff();
        }
		
        taskYIELD();
    }
}
	
void ButtonsTask::TempPlus()
{
	uint8_t externalTemp;
	if (_heaterTempLimit.TryGetLastExternalTemp(externalTemp))
    {
        if (WriteProperties.Chart.TempPlus(externalTemp))
        {
            _eeprom.Save();
            Properties.Chart.TempPlus(externalTemp);
        }
    }
}
	
void ButtonsTask::TempMinus()
{
	uint8_t externalTemp;
	if (_heaterTempLimit.TryGetLastExternalTemp(externalTemp))
    {
        if (WriteProperties.Chart.TempMinus(externalTemp))
        {
            _eeprom.Save();
            Properties.Chart.TempMinus(externalTemp);
        }
    }
}
	
void ButtonsTask::WaterPushButton()
{
    _valveTask.PushButton();
}
