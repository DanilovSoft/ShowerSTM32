#pragma once
#include "iActiveTask.h"
#include "WaterLevelTask.h"

class WaterLevelAnimTask final : public iActiveTask
{
public:
    
    char GetChar()
    {
        return m_cur;
    }
    
private:
    
    static constexpr uint8_t kAnimStep1 = '\x02'; // '\'
    static constexpr uint8_t kAnimStep2 = '\x03'; // '|'
    static constexpr uint8_t kAnimStep3 = '/';
    static constexpr uint8_t kAnimStep4 = '-';
    static constexpr auto kAnimSpeedMsec = 300;
    uint8_t m_pos = 0;
    TickType_t m_xLastWakeTime;
    volatile char m_cur;
    
    void Init()
    {
        m_cur = kAnimStep1;
        m_pos = 0;
    }
  
    void Run()
    {   
        // Initialise the xLastWakeTime variable with the current time.
        m_xLastWakeTime = xTaskGetTickCount();
    
        while (!g_waterLevelTask.GetIsInitialized())
        {
            Pause();
            
            switch (m_pos)
            {
            case 0:
                {
                    m_cur = kAnimStep2;  // '|'
                    m_pos = 1;
                    break;   
                }
            case 1:
                {
                    m_cur = kAnimStep3;  // '/'
                    m_pos = 2;
                    break;   
                }
            case 2:
                {
                    m_cur = kAnimStep4;  // '–'
                    m_pos = 3;
                    break;   
                }
            case 3:
                {
                    m_cur = kAnimStep1;  // '\'
                    m_pos = 0;
                    break;   
                }
            }
        }
    }
    
    void Pause()
    {
        vTaskDelayUntil(&m_xLastWakeTime, (kAnimSpeedMsec / portTICK_PERIOD_MS));
    }
};

extern WaterLevelAnimTask g_waterLevelAnimTask;
