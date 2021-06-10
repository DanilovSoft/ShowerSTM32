#include "ValveTask.h"
#include "stm32f10x_gpio.h"
#include "WaterLevelTask.h"
#include "HeaterTask.h"
#include "FreeRTOS.h"
#include "Interlocked.h"
#include "semphr.h"

ValveTask _valveTask;

void ValveTask::Init()
{
    // Клапан.
	GPIO_InitTypeDef gpio_init = 
	{
		.GPIO_Pin = Valve_Pin,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_Out_PP
	};
    GPIO_Init(Valve_GPIO, &gpio_init);
		
    // Закрыть клапан.
    GPIO_ResetBits(Valve_GPIO, Valve_Pin);
		
	// Питающий вывод сенсора.
	gpio_init = 
	{
		.GPIO_Pin = SensorSwitch_Power_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
	};
    GPIO_Init(SensorSwitch_Power_GPIO, &gpio_init);

    // Выключить питание сенсора до окончания инициали датчика уровня воды.
    GpioTurnOffSensorSwitch();

    _xValveSemaphore = xSemaphoreCreateBinaryStatic(&_xValveSemaphoreBuffer);	
}
	
void ValveTask::GpioTurnOnSensorSwitch()
{
    GPIO_SetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
}
	
void ValveTask::GpioTurnOffSensorSwitch()
{
	// Выключить сенсор.
    GPIO_ResetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
}
	
void ValveTask::GPIOOpenValve()
{
	// Включить воду.
    GPIO_SetBits(Valve_GPIO, Valve_Pin);
}
	
void ValveTask::GPIOCloseValve()
{
	// Закрыть клапан.
    GPIO_ResetBits(Valve_GPIO, Valve_Pin);
		
    // Выключить сенсор.
    GpioTurnOffSensorSwitch();
		
    // Выдержать паузу ради антидребезга.
    vTaskDelay(ValveDebounceMsec / portTICK_PERIOD_MS);

    // Включить питание сенсора.
    GpioTurnOnSensorSwitch();
}
	
void ValveTask::Run()
{
	// Ожидание инициализации датчика уровня воды.
    _waterLevelTask.WaitInitialization();
		
    _openValveAllowed = true;
		
    // Включить сенсор.
    GpioTurnOnSensorSwitch();
		
    while (true)
    {
        xSemaphoreTake(_xValveSemaphore, portMAX_DELAY);
        {
            _stopRequired = false;
				
            // Нельзя набирать воду если включен нагрев из-за вероятности ложного срабатывания.
            if (!_heaterTask.GetIsHeaterEnabled())
            {
	            uint8_t waterPercent = _waterLevelTask.DisplayingPercent;
				
            	// Если уровень воды меньше уровень автоматического отключения.
                if (waterPercent < Properties.WaterValve_Cut_Off_Percent)
                {					
                	// Включить воду.
                    GPIOOpenValve();	
    					
                    // Ожидать ручной остановки или достижение порогового уровня воды.
	                while (!_stopRequired && _waterLevelTask.DisplayingPercent < Properties.WaterValve_Cut_Off_Percent)
                    {
                        taskYIELD();
                    }
                }
            }
				
            // Выключить воду и сенсор с небольшой паузой.
            GPIOCloseValve();
				
            // Разрешить повторный набор воды.
            _openValveAllowed = true;
        }
    }
}
	
void ValveTask::OpenValveIfAllowed()
{
    if (_openValveAllowed)
    {
        _openValveAllowed = false;
        xSemaphoreGive(_xValveSemaphore);
    }
}
	
void ValveTask::PushButton()
{
    if (ValveOpened())
    {
    	// Нужно остановить воду.
        _stopRequired = true;
    }
    else
    {
    	// Попытка включить воду.
        OpenValveIfAllowed();
    }
}

void ValveTask::SensorOn()
{
    if (_sensorSwitchLastState != ValveTask::StateOn)
    {
        _sensorSwitchLastState = ValveTask::StateOn;
			
        // Попытка включить воду.
        OpenValveIfAllowed();
    }
}

void ValveTask::SensorOff()
{
    if (_sensorSwitchLastState != ValveTask::StateOff)
    {
        _sensorSwitchLastState = ValveTask::StateOff;
			
        // Нужно остановить воду.
        _stopRequired = true;
    }
}
    
volatile bool ValveTask::ValveIsOpen()
{
    return ValveOpened();
}
