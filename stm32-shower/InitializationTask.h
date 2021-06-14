#pragma once
#include "TaskBase.h"

class InitializationTask final : public TaskBase
{
public:
    
    InitializationTask()
    {
        m_lastAnimStep = kAnimStep1;
        m_lastAnimPosition = 0;
    }
    
    // По очереди меняет символы ['\', '|', '/', '-'].
    char GetWaterLevelAnimChar() volatile
    {
        return m_lastAnimStep;
    }
    
    void WaitForPropertiesInitialization()
    {
        while (!m_propertyInitialized)
        {
            taskYIELD();
        }
    }
    
private:
    
    static constexpr char kAnimStep1 = '\x02';  // '\'
    static constexpr char kAnimStep2 = '\x03';  // '|'
    static constexpr char kAnimStep3 = '/';
    static constexpr char kAnimStep4 = '-';
    static constexpr auto kAnimSpeedMsec = 300;
    
    volatile bool m_propertyInitialized = false;
    
    uint8_t m_lastAnimPosition = 0;
    TickType_t m_xLastWakeTime;
    volatile char m_lastAnimStep;
  
    void InitAllTasks();
    void Run();
    
    void Pause()
    {
        vTaskDelayUntil(&m_xLastWakeTime, (kAnimSpeedMsec / portTICK_PERIOD_MS));
    }
};

extern InitializationTask g_initializationTask;
