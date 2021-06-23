#pragma once
#include "TaskBase.h"

class WaterLevelAnimationTask final : public TaskBase
{
public:
    
    WaterLevelAnimationTask()
    {
        
    }
    
    // По очереди меняет символы ['\', '|', '/', '-'].
    char GetWaterLevelAnimChar() volatile
    {
        return m_lastAnimChar;
    }
    
    void ResumeAnimation()
    {
        // Пробуждаем текущий таск.
        xTaskNotifyGive(m_taskHandle);
    }
    
private:
    
    static constexpr char kAnimStep1 = '\x02';   // '\'
    static constexpr char kAnimStep2 = '\x03';   // '|'
    static constexpr char kAnimStep3 = '/';
    static constexpr char kAnimStep4 = '-';
    static constexpr auto kAnimSpeedMsec = 300;
    static constexpr TickType_t  kAnimSpeedPortMsec = kAnimSpeedMsec / portTICK_PERIOD_MS;
    
    volatile char m_lastAnimChar = kAnimStep1;
    
    // Блокирует поток на бесконечное время в ожидании сигнала.
    void WaitNotification()
    {
        ulTaskNotifyTake(pdTRUE, /* Clear the notification value before exiting. */ portMAX_DELAY);
    }
    
    void Run();
};

extern WaterLevelAnimationTask* g_wlAnimationTask;
