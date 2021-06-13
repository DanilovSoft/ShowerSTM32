#include "RTOSwrapper.h"

void RTOSwrapperClass::CreateTask(iActiveTask* obj, const char* name, UBaseType_t uxPriority)
{
    obj->Init();
        
    obj->taskHandle = xTaskCreateStatic(
        (TaskFunction_t)Run,       /* Function that implements the task. */
        name,          /* Text name for the task. */
        iActiveTask::ulStackDepth,      /* Number of indexes in the xStack array. */
        obj,    /* Parameter passed into the task. */
        uxPriority,/* Priority at which the task is created. */
        obj->xStack,          /* Array to use as the task's stack. */
        &obj->xTaskBuffer);  /* Variable to hold the task's data structure. */
}


void RTOSwrapperClass::Run(void* parm)
{
    iActiveTask* task = (iActiveTask*)parm;
    task->Run();
    
#if INCLUDE_vTaskDelete
    vTaskDelete(task->taskHandle);
#endif
}