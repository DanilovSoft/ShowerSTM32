#pragma once
#include "TaskBase.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "Properties.h"
#include "HeaterTask.h"
#include "InitializationTask.h"

class LedLightTask final : public TaskBase
{
public:
    
private:

    void Run()
    {
        g_initializationTask.WaitForPropertiesInitialization();
     
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
