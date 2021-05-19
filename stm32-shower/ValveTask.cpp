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
    GPIO_InitTypeDef gpio_init;
		
    /* Клапан */
    gpio_init.GPIO_Pin = Valve_Pin;	
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(Valve_GPIO, &gpio_init);
		
    /* Закрыть клапан */
    GPIO_ResetBits(Valve_GPIO, Valve_Pin);
		
    /* Питающий вывод сенсора */
    gpio_init.GPIO_Pin = SensorSwitch_Power_Pin;	
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(SensorSwitch_Power_GPIO, &gpio_init);
		
    /* Выключить питание сенсора до окончания инициали датчика уровня воды*/
    GpioTurnOffSensorSwitch();
		
    xValveSemaphore = xSemaphoreCreateBinaryStatic(&xValveSemaphoreBuffer);	
}
	

void ValveTask::GpioTurnOnSensorSwitch()
{
	/* Включить сенсор */
    GPIO_SetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
}
	

void ValveTask::GpioTurnOffSensorSwitch()
{
	/* Выключить сенсор */
    GPIO_ResetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
}
	

void ValveTask::GPIOOpenValve()
{
	/* Включить воду */
    GPIO_SetBits(Valve_GPIO, Valve_Pin);
}
	

void ValveTask::GPIOCloseValve()
{
	/* Закрыть клапан */
    GPIO_ResetBits(Valve_GPIO, Valve_Pin);
		
    /* Выключить сенсор */
    GpioTurnOffSensorSwitch();
		
    /* Выдержать паузу ради антидребезга */
    vTaskDelay(Valve_Delay / portTICK_PERIOD_MS);

    /* Включить питание сенсора */
    GpioTurnOnSensorSwitch();
}
	

void ValveTask::Run()
{
	/* Ожидание инициализации датчика уровня воды */
    _waterLevelTask.WaitInitialization();
		
    OpenValveAllowed = true;
		
    /* Включить сенсор */
    GpioTurnOnSensorSwitch();
		
    while (1)
    {
        xSemaphoreTake(xValveSemaphore, portMAX_DELAY);
        {
            StopRequired = false;
				
            /* Нельзя набирать воду если включен нагрев из-за вероятности ложного срабатывания */
            if (!_heaterTask.GetIsHeaterEnabled())
            {
	            uint8_t waterPercent = _waterLevelTask.DisplayingPercent;
				
            	/* Если уровень воды меньше уровень автоматического отключения */
                if (waterPercent < Properties.Customs.WaterValve_Cut_Off_Percent)
                {					
                	/* Включить воду */
                    GPIOOpenValve();	
    					
                    /* Ожидать ручной остановки или достижение порогового уровня воды */
	                while (!StopRequired && _waterLevelTask.DisplayingPercent < Properties.Customs.WaterValve_Cut_Off_Percent)
                    {
                        taskYIELD();
                    }
                }
            }
				
            /* Выключить воду и сенсор с небольшой паузой */
            GPIOCloseValve();
				
            /* Разрешить повторный набор воды */
            OpenValveAllowed = true;
        }
    }
}
	

void ValveTask::OpenValveIfAllowed()
{
    if (OpenValveAllowed)
    {
        OpenValveAllowed = false;
        xSemaphoreGive(xValveSemaphore);
    }
}
	
	
	/* Вызывается только если кнопка была нажата */
void ValveTask::PushButton()
{
    if (ValveOpened())
    {
    	/* Нужно остановить воду */
        StopRequired = true;
    }
    else
    {
    	/* Попытка включить воду */
        OpenValveIfAllowed();
    }
}
	

/* Вызывается каждый раз, после PushButton() */
void ValveTask::SensorOn()
{
    if (SensorSwitchLastState != ValveTask::StateOn)
    {
        SensorSwitchLastState = ValveTask::StateOn;
			
        /* Попытка включить воду */
        OpenValveIfAllowed();
    }
}
	

/* Вызывается каждый раз, после PushButton() */
void ValveTask::SensorOff()
{
    if (SensorSwitchLastState != ValveTask::StateOff)
    {
        SensorSwitchLastState = ValveTask::StateOff;
			
        /* Нужно остановить воду */
        StopRequired = true;
    }
}
    
volatile bool ValveTask::ValveIsOpen()
{
    return ValveOpened();
}