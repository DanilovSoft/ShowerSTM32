#pragma once
#include "TaskBase.h"
#include "Common.h"

class LedLightTask final : public TaskBase
{
public:
    
    LedLightTask()
    {
    }
    
private:

    virtual void Run() override
    {   
        bool light_is_on;
        
        while (true)
        {	
            if (Common::CircuitBreakerIsOn())
            {
                if (light_is_on)
                {
                    light_is_on = false;
                    Common::TurnOffLight();
                }
            }
            else if (!light_is_on)
            {
                light_is_on = true;
                Common::TurnOnLight();
            }
        
            taskYIELD();
        }
    }
};

extern LedLightTask g_ledLightTask;
