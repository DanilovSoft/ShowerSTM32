#pragma once
#include "TaskBase.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Common.h"
#include "Debug.h"

class InitializationTask final : public TaskBase
{
public:
    
    InitializationTask(const TaskFunction_t func)
        : m_func(func)
    {
        Debug::Assert(func != NULL);
    }
    
private:
    
    const TaskFunction_t m_func;
    
    virtual void Run() override
    {
        m_func(NULL);
    }
};