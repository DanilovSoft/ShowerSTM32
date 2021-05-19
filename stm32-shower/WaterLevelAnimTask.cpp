#include "WaterLevelAnimTask.h"
#include "WaterLevelTask.h"

#define AnimSpeedMsec   (300)

WaterLevelAnimTask _waterLevelAnimTask;
TickType_t xLastWakeTime;


void WaterLevelAnimTask::Init()
{
    _cur = ANIM1;
    _pos = 0;
}
  

void WaterLevelAnimTask::Run()
{   
    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    
    while (!_waterLevelTask.Initialized)
    {
        Pause();
            
        switch (_pos)
        {
        case 0:
            {
                _cur = ANIM2; /* | */
                _pos = 1;
                break;   
            }
        case 1:
            {
                _cur = ANIM3; /* / */
                _pos = 2;
                break;   
            }
        case 2:
            {
                _cur = ANIM4; /* - */
                _pos = 3;
                break;   
            }
        case 3:
            {
                _cur = ANIM1; /* \ */
                _pos = 0;
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
    return _cur;
}