#include "ButtonsTask.h"
#include "stm32f10x_gpio.h"
#include "Buzzer.h"
#include "HeaterTask.h"
#include "Eeprom.h"
#include "Properties.h"
#include "TempSensor.h"
#include "HeaterTempLimit.h"
#include "ValveTask.h"
#include "FrontPanelButton.h"
#include "SensorSwitch.h"

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
    buzzer.BeepHighPrio(samples, sizeof(samples) / sizeof(*samples));
}

void ButtonsTask::Run()
{
    FrontPanelButton buttonTempPlus(Button_GPIO, Button_Temp_Plus);
    FrontPanelButton buttonTempMinus(Button_GPIO, Button_Temp_Minus);
    FrontPanelButton buttonTempValve(Button_GPIO, Button_Water);
    SensorSwitch sensorSwitch(Button_GPIO, Button_SensorSwitch_OUT);
   
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
        
        if (buttonTempValve.IsPressed())
        {
            PressSound();
            _valveTask.PushButton();
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
        if (_writeOnlyPropertiesStruct.Chart.TempPlus(externalTemp))
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
        if (_writeOnlyPropertiesStruct.Chart.TempMinus(externalTemp))
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
