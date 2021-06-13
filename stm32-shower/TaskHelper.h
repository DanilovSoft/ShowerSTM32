#pragma once
#include "iActiveTask.h"

class TaskHelper
{   
public:
    
    static void Run(void* parm)
    {
        iActiveTask* task = (iActiveTask*)parm;
        task->Run();
#if INCLUDE_vTaskDelete
        vTaskDelete(task->taskHandle);
#endif
    }
    
    static void CreateTask(TaskHelper* task_helper, void* func, const char* name)
    {
        task_helper->taskHandle = xTaskCreateStatic(
            (TaskFunction_t)func,
            /* Function that implements the task. */
            name,
            /* Text name for the task. */
            TaskHelper::kULStackDepth,
            /* Number of indexes in the xStack array. */
            &task_helper,
            /* Parameter passed into the task. */
            tskIDLE_PRIORITY,
            /* Priority at which the task is created. */
            task_helper->xStack,
            /* Array to use as the task's stack. */
            &(task_helper->xTaskBuffer)); /* Variable to hold the task's data structure. */
    }
        
    void CreateTask(iActiveTask* obj, const char* name, UBaseType_t ux_priority)
    {
        obj->Init();
        
        obj->taskHandle = xTaskCreateStatic(
            (TaskFunction_t)Run,
            /* Function that implements the task. */
            name,
            /* Text name for the task. */
            iActiveTask::ulStackDepth,
            /* Number of indexes in the xStack array. */
            obj,
            /* Parameter passed into the task. */
            ux_priority,
            /* Priority at which the task is created. */
            obj->xStack,
            /* Array to use as the task's stack. */
            &obj->xTaskBuffer); /* Variable to hold the task's data structure. */
    }
    
private:
    
    static const uint32_t kULStackDepth = configMINIMAL_STACK_SIZE;

    TaskHandle_t taskHandle;

    /* Buffer that the task being created will use as its stack.  Note this is
    an array of StackType_t variables.  The size of StackType_t is dependent on
    the RTOS port. */
    StackType_t xStack[kULStackDepth];

    /* Structure that will hold the TCB of the task being created. */
    StaticTask_t xTaskBuffer;
};

extern TaskHelper g_taskHelper;
