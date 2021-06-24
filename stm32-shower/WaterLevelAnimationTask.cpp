#include "WaterLevelAnimationTask.h"
#include "WaterLevelTask.h"

void WaterLevelAnimationTask::Run()
{    
    // Initialise the xLastWakeTime variable with the current time.
    TickType_t last_wake_time = xTaskGetTickCount();
    
    uint8_t last_anim_position = 0;
        
    while (true)
    {
        while (!g_waterLevelTask.GetInitialized() || g_waterLevelTask.GetIsError())
        {
            // Что-бы анимация была равномерной, используем более точную паузу.
            vTaskDelayUntil(&last_wake_time, kAnimSpeedPortMsec);
            
            switch (last_anim_position)
            {
            case 0:
                {
                    // '|'
                    m_lastAnimChar = kAnimStep2;
                    last_anim_position = 1;
                    break;   
                }
            case 1:
                {
                    // '/'
                    m_lastAnimChar = kAnimStep3;
                    last_anim_position = 2;
                    break;   
                }
            case 2:
                {
                    // '–'
                    m_lastAnimChar = kAnimStep4;      
                    last_anim_position = 3;
                    break;   
                }
            case 3:
                {
                    // '\'
                    m_lastAnimChar = kAnimStep1;      
                    last_anim_position = 0;
                    break;   
                }
            }
        }
    
        // Ждём когда другой поток нас разбудит.
        WaitNotification();
    
        // Сбрасываем анимацию на начало.
        m_lastAnimChar = kAnimStep1;
        last_anim_position = 0;
    }
}