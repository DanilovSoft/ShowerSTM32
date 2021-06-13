#pragma once
#include "iActiveTask.h"
#include "semphr.h"
#include "stm32f10x_gpio.h"
#include "WaterLevelTask.h"
#include "HeaterTask.h"
#include "FreeRTOS.h"
#include "Interlocked.h"

class ValveTask final : public iActiveTask
{	
public:
    
    ValveTask()
    {
        m_sensorSwitchIsOnLastState = false;
        m_openValveAllowed = false;
        m_stopRequired = false;
    }
    
    // Вызывается если кнопка была нажата.
    void OnButtonPress()
    {
        if (Common::ValveIsOpen())
        {
            // Нужно остановить воду.
            m_stopRequired = true;
        }
        else
        {
            // Попытка включить воду.
            TryOpenValve();
        }
    }

    // Вызывается каждый раз, после OnButtonPushed().
    void UpdateSensorState(bool isOn)
    {
        if (isOn)
        {
            if (!m_sensorSwitchIsOnLastState)
            {
                m_sensorSwitchIsOnLastState = true;
            
                // Попытка включить воду.
                TryOpenValve();
            }
        }
        else
        {
            if (m_sensorSwitchIsOnLastState)
            {
                m_sensorSwitchIsOnLastState = false;
            
                // Нужно остановить воду.
                m_stopRequired = true;
            }
        }
    }

    void ForceOpenValve()
    {
        
    }
    
private:
    
    // Пауза для повторного включения клапана.
    static const auto kValveDebounceMsec = 300;
    SemaphoreHandle_t m_xValveSemaphore;
    StaticSemaphore_t m_xValveSemaphoreBuffer;
    bool m_sensorSwitchIsOnLastState;
    // Флаг прекращающий набор воды по запросу пользователя.
    volatile bool m_stopRequired;
    // Флаг предотвращающий повторные запросы на открытие клапана от пользователя,
    // пока другой поток обрабатывает первый запрос (троттлинг).
    volatile bool m_openValveAllowed;
    
    void Init()
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
    
    // Включает питание сенсора.
    void GpioTurnOnSensorSwitch()
    {
        GPIO_SetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
    }
    
    // Выключает питание сенсора.
    void GpioTurnOffSensorSwitch()
    {
        // Выключить сенсор.
        GPIO_ResetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
    }
    
    void OpenValve()
    {
        // Включить воду.
        GPIO_SetBits(Valve_GPIO, Valve_Pin);
    }
    
    // Закрывает клапан.
    // Выключает сенсор и выдерживает небольшую паузу 
    // и опять включает сенсор (так мы сбрасываем его активное состояние).
    // Блокирует поток на время паузы.
    void CloseValve()
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
    
    void Run()
    {
        // Ожидание инициализации датчика уровня воды.
        g_waterLevelTask.WaitInitialization();
        
        // Нужно включить флаг перед включением сенсора.
        m_openValveAllowed = true;
        
        // Включить сенсор.
        GpioTurnOnSensorSwitch();
        
        while (true)
        {
            // Спим пока не поступит запрос на открытие клапана.
            xSemaphoreTake(m_xValveSemaphore, portMAX_DELAY);
            
            // PS. Текущий поток может устанавливать ТОЛЬКО значение false этому флагу.
            m_stopRequired = false;
                
            // Нельзя набирать воду если включен нагрев из-за вероятности ложного срабатывания.
            if(!Common::GetIsHeaterEnabled())
            {
                uint8_t waterPercent = g_waterLevelTask.DisplayingPercent;
                
                // Если уровень воды меньше уровень автоматического отключения.
                if(waterPercent < g_properties.WaterValve_Cut_Off_Percent)
                {					
                    // Включить воду.
                    OpenValve();	
                        
                    // Ожидаем достижение порогового уровня воды или ручной остановки.
                    while (!m_stopRequired && g_waterLevelTask.DisplayingPercent < g_properties.WaterValve_Cut_Off_Percent)
                    {
                        taskYIELD();
                    }
                }
            }
                
            // Закрываем клапан с небольшой паузой. Здесь получаем дополнительный эффект — гистерезис или антидребезг от повторных включений.
            CloseValve();

            // Разрешить следующий запрос на открытие клапана.
            // PS. Текущий поток может устанавливать ТОЛЬКО значение true этому флагу.
            m_openValveAllowed = true;
        }
    }
    
    // Вызывается когда пользователь нажимает на кнопку или на сенсорную панель.
    void TryOpenValve()
    {
        if (m_openValveAllowed)
        {
            // Запрещаем пользователю повторные запросы.
            m_openValveAllowed = false;
            
            // Пробуждаем поток.
            xSemaphoreGive(m_xValveSemaphore);
        }
    }
};

extern ValveTask g_valveTask;
