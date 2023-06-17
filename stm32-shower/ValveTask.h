#pragma once
#include "TaskBase.h"
#include "semphr.h"
#include "stm32f10x_gpio.h"
#include "WaterLevelTask.h"
#include "HeaterTask.h"
#include "FreeRTOS.h"
#include "Interlocked.h"

class ValveTask final : public TaskBase
{	
public:
    
    // Вызывается каждый раз, после OnButtonPushed().
    void UpdateSensorState(bool sensorIsOn)
    {
        if (sensorIsOn)
        {
            if (m_lastSensorIsOn)
            {
                return;
            }
            
            m_lastSensorIsOn = true;
            TryOpenValve(); // Попытка включить воду.
        }
        else
        {
            if (!m_lastSensorIsOn)
            {
                return;
            }
            
            m_lastSensorIsOn = false;
            m_stopRequired = true; // Нужно остановить воду.
        }
    }
    
    // Вызывается при нажатии на кнопку.
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

    // Вызывается при долгом нажатии на кнопку.
    void ForceOpenValve()
    {
        if (m_openValveAllowed == ValveTask::WaitingRequest)
        {
            // Запрещаем пользователю повторные запросы.
            m_openValveAllowed = ValveTask::PendingForceOpen;
            
            // Пробуждаем поток.
            xTaskNotifyGive(m_taskHandle);
        }
    }
    
private:
    
    enum ValveState
    {
        WaitingRequest,
        PendingOpen,
        PendingForceOpen
    };
    
    // Пауза для повторного включения клапана.
    static const auto kValveDebounceMsec = 300;
    bool m_lastSensorIsOn = false;
    // Флаг прекращающий набор воды по запросу пользователя.
    volatile bool m_stopRequired = false;
    // Флаг предотвращающий повторные запросы на открытие клапана от пользователя,
    // пока другой поток обрабатывает первый запрос (троттлинг).
    volatile ValveState m_openValveAllowed = ValveState::PendingOpen;
    
    // Закрывает клапан.
    // Выключает сенсор и выдерживает небольшую паузу 
    // и опять включает сенсор (так мы сбрасываем его активное состояние).
    // Блокирует поток на время паузы.
    void CloseValve()
    {
        Common::CloseValve(); // Закрыть клапан.
        Common::TurnOffSensorSwitch(); // Выключить сенсор.
        vTaskDelay(kValveDebounceMsec / portTICK_PERIOD_MS); // Выдержать паузу для антидребезга.
        Common::TurnOnSensorSwitch(); // Включить питание сенсора.
    }
    
    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        m_openValveAllowed = ValveTask::WaitingRequest; // Нужно включить флаг перед включением сенсора.
        Common::TurnOnSensorSwitch(); // Включить сенсор.
        
        while (true)
        {
            ulTaskNotifyTake(pdTRUE, /* Clear the notification value before exiting. */ portMAX_DELAY); // Спим пока не поступит запрос на открытие клапана.
            m_stopRequired = false; // PS. Текущий поток ДОЛЖЕН устанавливать ТОЛЬКО значение false этому флагу.
                
            if (!Common::CircuitBreakerIsOn()) // Лучше не набирать воду когда включен автомат нагревателя.
            {
                if (m_openValveAllowed == ValveTask::PendingForceOpen)
                {
                    Common::OpenValve(); // Включить воду.
                    
                    while (!m_stopRequired) // Ожидаем достижение порогового уровня воды или ручной остановки.
                    {
                        taskYIELD();
                    }
                }
                else
                {
                    if (g_waterLevelTask.GetInitialized())
                    {
                        // Если уровень воды меньше уровня автоматического отключения.
                        if (g_waterLevelTask.GetPercent() < g_properties.WaterValveCutOffPercent)
                        {					
                            Common::OpenValve(); // Включить воду.
                        
                            // Ожидаем достижение порогового уровня воды или ручной остановки.
                            while (!m_stopRequired && g_waterLevelTask.GetPercent() < g_properties.WaterValveCutOffPercent)
                            {
                                taskYIELD();
                            }
                        }
                    }
                }
            }
                
            CloseValve(); // Закрываем клапан с небольшой паузой. Здесь получаем дополнительный эффект — гистерезис или антидребезг от повторных включений.

            // Разрешить следующий запрос на открытие клапана.
            // PS. Текущий поток может устанавливать ТОЛЬКО значение WaitingRequest этому флагу.
            m_openValveAllowed = ValveTask::WaitingRequest;
        }
    }
    
    // Вызывается когда пользователь нажимает на кнопку или на сенсорную панель.
    void TryOpenValve()
    {
        if (m_openValveAllowed != ValveTask::WaitingRequest)
        {
            return;
        }
        
        m_openValveAllowed = ValveTask::PendingOpen; // Запрещаем пользователю повторные запросы.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
};

extern ValveTask g_valveTask;
