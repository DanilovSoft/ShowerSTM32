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
        if (m_openValveAllowed != ValveTask::WaitingRequest)
        {
            return;
        }
        
        m_openValveAllowed = ValveTask::OpenRequest; // Запрещаем пользователю повторные запросы.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
    
    void CloseValveRequest()
    {
        m_stopRequired = true; // Нужно остановить воду.
    }
    
    // Вызывается при нажатии на кнопку.
    void OnButtonPress()
    {
        if (Common::ValveIsOpen())
        {
            m_stopRequired = true; // Нужно остановить воду.
        }
        else
        {
            OpenValveRequest(); // Попытка включить воду.
        }
    }

    // Вызывается при долгом нажатии на кнопку.
    void ForceOpenValve()
    {
        if (m_openValveAllowed != ValveTask::WaitingRequest)
        {
            return;
        }
        
        m_openValveAllowed = ValveTask::ForceOpenRequest; // Запрещаем пользователю повторные запросы.
        xTaskNotifyGive(m_taskHandle); // Пробуждаем поток.
    }
    
    bool OpenAllowed()
    {
        return m_openValveAllowed == ValveTask::WaitingRequest;
    }
    
private:
    
    enum ValveState
    {
        WaitingRequest, // Поток готов принимать запросы.
        OpenRequest, // Просим поток открыть клапан.
        ForceOpenRequest // Просим поток открыть клапан игнорируя все проверки.
    };
    
    // Пауза для повторного включения клапана.
    static const auto ValveDebounceMsec = 300;
    //bool m_lastSensorIsOn = false;
    // Флаг прекращающий набор воды по запросу пользователя.
    volatile bool m_stopRequired = false;
    // Флаг предотвращающий повторные запросы на открытие клапана от пользователя,
    // пока другой поток обрабатывает первый запрос (троттлинг).
    volatile ValveState m_openValveAllowed = ValveState::OpenRequest;
    
    // Закрывает клапан.
    // Выключает сенсор и выдерживает небольшую паузу 
    // и опять включает сенсор (так мы сбрасываем его активное состояние).
    // Блокирует поток на время паузы.
    void CloseValve()
    {
        Common::CloseValve(); // Закрыть клапан.
        //Common::PowerOffSensorSwitch(); // Выключить сенсор.
        vTaskDelay(ValveDebounceMsec / portTICK_PERIOD_MS); // Выдержать паузу для антидребезга.
        //Common::PowerOnSensorSwitch(); // Включить питание сенсора.
    }
    
    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        m_openValveAllowed = ValveTask::WaitingRequest; // Нужно включить флаг перед включением сенсора.
        //Common::PowerOnSensorSwitch(); // Включить сенсор.
        
        while (true)
        {
            ulTaskNotifyTake(pdTRUE, /* Clear the notification value before exiting. */ portMAX_DELAY); // Спим пока не поступит запрос на открытие клапана.
            m_stopRequired = false; // PS. Текущий поток ДОЛЖЕН устанавливать ТОЛЬКО значение false этому флагу.
                
            if (!Common::CircuitBreakerIsOn()) // Лучше не набирать воду когда включен автомат нагревателя.
            {
                if (m_openValveAllowed == ValveTask::ForceOpenRequest)
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
};

extern ValveTask g_valveTask;
