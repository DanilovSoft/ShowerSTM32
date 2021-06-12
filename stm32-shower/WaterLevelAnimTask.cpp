#include "WaterLevelAnimTask.h"
#include "WaterLevelTask.h"

#define AnimSpeedMsec   (300)

WaterLevelAnimTask g_waterLevelAnimTask;
TickType_t xLastWakeTime;

void WaterLevelAnimTask::Init()
{
    m_cur = ANIM1;
    m_pos = 0;
}
  
void WaterLevelAnimTask::Run()
{   
    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    
    while (!g_waterLevelTask.Initialized)
    {
        Pause();
            
        switch (m_pos)
        {
        case 0:
            {
                m_cur = ANIM2; // '|'
                m_pos = 1;
                break;   
            }
        case 1:
            {
                m_cur = ANIM3; // '/'
                m_pos = 2;
                break;   
            }
        case 2:
            {
                m_cur = ANIM4; // '–'
                m_pos = 3;
                break;   
            }
        case 3:
            {
                m_cur = ANIM1; // '\'
                m_pos = 0;
                break;   
            }
        }
    }
}
    
void WaterLevelAnimTask::Pause()
{
    vTaskDelayUntil(&xLastWakeTime, (AnimSpeedMsec / portTICK_PERIOD_MS));
}
   
char WaterLevelAnimTask::GetChar()
{
    return m_cur;
}
