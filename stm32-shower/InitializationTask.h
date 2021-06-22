#pragma once
#include "TaskBase.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Common.h"

class InitializationTask final : public TaskBase
{
public:
    
    InitializationTask(TaskFunction_t func)
        : m_func(func)
    {
        DebugAssert(func != NULL);
    }
    
private:
    
    const TaskFunction_t m_func;
    
    void Run()
    {
        m_func(NULL);
    }
};