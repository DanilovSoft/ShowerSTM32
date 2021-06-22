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
    
    ValveTask(const PropertyStruct* const properties)
        : m_properties(properties)
    {
        
    }
    
    // Вызывается каждый раз, после OnButtonPushed().
    void UpdateSensorState(bool is_on)
    {
        if (is_on)
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
    
    const PropertyStruct* const m_properties;
    bool m_sensorSwitchIsOnLastState = false;
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
        // Закрыть клапан.
        Common::CloseValve();
        
        // Выключить сенсор.
        Common::TurnOffSensorSwitch();
        
        // Выдержать паузу для антидребезга.
        vTaskDelay(kValveDebounceMsec / portTICK_PERIOD_MS);

        // Включить питание сенсора.
        Common::TurnOnSensorSwitch();
    }
    
    void Run()
    {
        // Нужно включить флаг перед включением сенсора.
        m_openValveAllowed = ValveTask::WaitingRequest;
        
        // Включить сенсор.
        Common::TurnOnSensorSwitch();
        
        while (true)
        {
            // Спим пока не поступит запрос на открытие клапана.
            ulTaskNotifyTake(pdTRUE, /* Clear the notification value before exiting. */ portMAX_DELAY);
            
            // PS. Текущий поток ДОЛЖЕН устанавливать ТОЛЬКО значение false этому флагу.
            m_stopRequired = false;
                
            // Нельзя набирать воду если включен автомат нагревателя из-за вероятности ложного срабатывания.
            if(!Common::CircuitBreakerIsOn())
            {
                if (m_openValveAllowed == ValveTask::PendingForceOpen)
                {
                    // Включить воду.
                    Common::OpenValve();	
                        
                    // Ожидаем достижение порогового уровня воды или ручной остановки.
                    while (!m_stopRequired)
                    {
                        taskYIELD();
                    }
                }
                else
                {
                    if (g_waterLevelTask->GetIsInitialized())
                    {
                        // Если уровень воды меньше уровня автоматического отключения.
                        if(g_waterLevelTask->Percent < m_properties->WaterValveCutOffPercent)
                        {					
                            // Включить воду.
                            Common::OpenValve();
                        
                            // Ожидаем достижение порогового уровня воды или ручной остановки.
                            while(!m_stopRequired && g_waterLevelTask->Percent < m_properties->WaterValveCutOffPercent)
                            {
                                taskYIELD();
                            }
                        }
                    }
                }
            }
                
            // Закрываем клапан с небольшой паузой. Здесь получаем дополнительный эффект — гистерезис или антидребезг от повторных включений.
            CloseValve();

            // Разрешить следующий запрос на открытие клапана.
            // PS. Текущий поток может устанавливать ТОЛЬКО значение WaitingRequest этому флагу.
            m_openValveAllowed = ValveTask::WaitingRequest;
        }
    }
    
    // Вызывается когда пользователь нажимает на кнопку или на сенсорную панель.
    void TryOpenValve()
    {
        if (m_openValveAllowed == ValveTask::WaitingRequest)
        {
            // Запрещаем пользователю повторные запросы.
            m_openValveAllowed = ValveTask::PendingOpen;
            
            // Пробуждаем поток.
            xTaskNotifyGive(m_taskHandle);
        }
    }
};

extern ValveTask* g_valveTask;
