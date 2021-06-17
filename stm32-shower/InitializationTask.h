#pragma once
#include "TaskBase.h"
#include "FreeRTOS.h"
#include "task.h"

class InitializationTask final : public TaskBase
{
public:
    
    void WaitForPropertiesInitialization()
    {
        while (!m_propertyInitialized)
        {
            taskYIELD();
        }
    }
    
private:
    
    volatile bool m_propertyInitialized = false;
  
    void InitAllTasks();
    void Run();
};

extern InitializationTask g_initializationTask;
