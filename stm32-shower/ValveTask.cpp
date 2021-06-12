#include "ValveTask.h"
#include "stm32f10x_gpio.h"
#include "WaterLevelTask.h"
#include "HeaterTask.h"
#include "FreeRTOS.h"
#include "Interlocked.h"
#include "semphr.h"

// Включен ли клапан воды.
bool ValveTask::ValveIsOpen()
{
	return GPIO_ReadInputDataBit(Valve_GPIO, Valve_Pin);
}

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

    m_xValveSemaphore = xSemaphoreCreateBinaryStatic(&m_xValveSemaphoreBuffer);	
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
    vTaskDelay(kValveDebounceMsec / portTICK_PERIOD_MS);

    // Включить питание сенсора.
    GpioTurnOnSensorSwitch();
}
	
void ValveTask::Run()
{
	// Ожидание инициализации датчика уровня воды.
    g_waterLevelTask.WaitInitialization();
		
    m_openValveAllowed = true;
		
    // Включить сенсор.
    GpioTurnOnSensorSwitch();
		
    while (true)
    {
        xSemaphoreTake(m_xValveSemaphore, portMAX_DELAY);
        {
            m_stopRequired = false;
				
            // Нельзя набирать воду если включен нагрев из-за вероятности ложного срабатывания.
            if (!g_heaterTask.GetIsHeaterEnabled())
            {
	            uint8_t waterPercent = g_waterLevelTask.DisplayingPercent;
				
            	// Если уровень воды меньше уровень автоматического отключения.
                if (waterPercent < g_properties.WaterValve_Cut_Off_Percent)
                {					
                	// Включить воду.
                    GPIOOpenValve();	
    					
                    // Ожидать ручной остановки или достижение порогового уровня воды.
	                while (!m_stopRequired && g_waterLevelTask.DisplayingPercent < g_properties.WaterValve_Cut_Off_Percent)
                    {
                        taskYIELD();
                    }
                }
            }
				
            // Выключить воду и сенсор с небольшой паузой.
            GPIOCloseValve();
				
            // Разрешить повторный набор воды.
            m_openValveAllowed = true;
        }
    }
}
	
void ValveTask::OpenValveIfAllowed()
{
    if (m_openValveAllowed)
    {
        m_openValveAllowed = false;
        xSemaphoreGive(m_xValveSemaphore);
    }
}
	
void ValveTask::PushButton()
{
    if (ValveIsOpen())
    {
    	// Нужно остановить воду.
        m_stopRequired = true;
    }
    else
    {
    	// Попытка включить воду.
        OpenValveIfAllowed();
    }
}

void ValveTask::SensorOn()
{
    if (m_sensorSwitchLastState != ValveTask::StateOn)
    {
        m_sensorSwitchLastState = ValveTask::StateOn;
			
        // Попытка включить воду.
        OpenValveIfAllowed();
    }
}

void ValveTask::SensorOff()
{
    if (m_sensorSwitchLastState != ValveTask::StateOff)
    {
        m_sensorSwitchLastState = ValveTask::StateOff;
			
        // Нужно остановить воду.
        m_stopRequired = true;
    }
}
