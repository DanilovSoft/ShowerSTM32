#pragma once
#include "TaskBase.h"

class RtosHelper final
{   
public:
    
    static void Run(void* parm)
    {
        TaskBase* task = (TaskBase*)parm;
        task->Run();
     
        // ! ВНИМАНИЕ ! Если таск завершится, а опция INCLUDE_vTaskDelete будет выключена 
        // то диспетчер потоков заглохнет и сработает сторожевой таймер.
        
#if INCLUDE_vTaskDelete
        vTaskDelete(task->taskHandle);
#else
        __asm("bkpt 255");
#endif
    }
    
    static void CreateTask(TaskBase* obj, const char* name, UBaseType_t uxPriority = tskIDLE_PRIORITY)
    {
        obj->taskHandle = xTaskCreateStatic(
            (TaskFunction_t)Run,
            /* Function that implements the task. */
            name,
            /* Text name for the task. */
            TaskBase::kULStackDepth,
            /* Number of indexes in the xStack array. */
            obj,
            /* Parameter passed into the task. */
            uxPriority,
            /* Priority at which the task is created. */
            obj->xStack,
            /* Array to use as the task's stack. */
            &obj->xTaskBuffer); /* Variable to hold the task's data structure. */
    }
};
