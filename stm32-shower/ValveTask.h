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
    
    // Вызывается когда пользователь нажимает на кнопку или на сенсорную панель.
    void OpenValveRequest()
    {
        if (m_state != ValveTask::WaitingRequest)
        {
            return;
        }
        
        m_state = ValveTask::OpenRequest; // Запрещаем пользователю повторные запросы.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
    
    void CloseValveRequest()
    {
        m_state = ValveTask::StopRequest; // Нужно остановить воду.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
    
    // Вызывается при нажатии на кнопку.
    void OnButtonPress()
    {
        if (Common::ValveIsOpen())
        {
            m_state = ValveTask::StopRequest; // Нужно остановить воду.
            xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
        }
        else
        {
            OpenValveRequest(); // Попытка включить воду.
        }
    }

    // Вызывается при долгом нажатии на кнопку.
    void ForceOpenValve()
    {
        if (m_state != ValveTask::WaitingRequest)
        {
            return;
        }
        
        m_state = ValveTask::ForceOpenRequest; // Запрещаем пользователю повторные запросы.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
    
    bool IsWaitingRequest()
    {
        return m_state == ValveTask::WaitingRequest;
    }
    
private:
    
    enum ValveState
    {
        WaitingRequest, // Поток готов принимать запросы.
        OpenRequest, // Просим поток открыть клапан.
        ForceOpenRequest, // Просим поток открыть клапан игнорируя все проверки.
        StopRequest
    };
    
    // Пауза для повторного включения клапана.
    static const auto ValveDebounceMsec = 300;
    // Флаг предотвращающий повторные запросы на открытие клапана от пользователя,
    // пока другой поток обрабатывает первый запрос (троттлинг).
    volatile ValveState m_state = ValveState::OpenRequest;
    
    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        while (true)
        {
            m_state = ValveTask::WaitingRequest; // Разрешить следующий запрос.
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Спим пока не разбудит другой поток.
            
            if (Common::CircuitBreakerIsOn()) // Лучше не набирать воду когда включен автомат нагревателя.
            {
                continue;
            }
            
            switch (m_state)
            {
            case ValveTask::OpenRequest:
                {
                    if (!g_waterLevelTask.IsInitialized())
                    {
                        continue;
                    }

                    if (g_waterLevelTask.GetPercent() >= g_properties.WaterValveCutOffPercent) // Если уровень воды меньше уровня автоматического отключения.
                    {					
                        continue;
                    }

                    Common::OpenValve(); // Открыть воду.
                    while (m_state != ValveTask::StopRequest && g_waterLevelTask.GetPercent() < g_properties.WaterValveCutOffPercent)
                    {
                        taskYIELD(); // Ожидаем достижение порогового уровня воды или ручной остановки.
                    }
                }
                break;
            case ValveTask::ForceOpenRequest:
                {
                    Common::OpenValve(); // Открыть воду.
                    while (m_state != ValveTask::StopRequest) // Ожидаем когда другой поток сообщит нам что пора закрыть воду.
                    {
                        taskYIELD();
                    }   
                }
                break;
            default:
                break;
            }

            CloseValve(); // Закрываем клапан с небольшой паузой для антидребезга.
        }
    }
    
    // Закрывает клапан.
    // Выключает сенсор и выдерживает небольшую паузу 
    // и опять включает сенсор (так мы сбрасываем его активное состояние).
    // Блокирует поток на время паузы.
    void CloseValve()
    {   
        if (!Common::ValveIsOpen())
        {
            return;
        }
        
        Common::CloseValve(); // Закрыть клапан.
        vTaskDelay(ValveDebounceMsec / portTICK_PERIOD_MS); // Выдержать паузу для антидребезга.
    }
};

extern ValveTask g_valveTask;
