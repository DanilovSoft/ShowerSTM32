#include "TaskHelper.h"

TaskHelper g_taskHelper;

void TaskHelper::CreateTask(iActiveTask* obj, const char* name, UBaseType_t uxPriority)
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

void TaskHelper::CreateTask(TaskHelper* taskHelper, void* func, const char* name)
{
    taskHelper->taskHandle = xTaskCreateStatic(
        (TaskFunction_t)func,       /* Function that implements the task. */
        name,          /* Text name for the task. */
        TaskHelper::ulStackDepth,      /* Number of indexes in the xStack array. */
        &taskHelper,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY,/* Priority at which the task is created. */
        taskHelper->xStack,          /* Array to use as the task's stack. */
        &(taskHelper->xTaskBuffer));  /* Variable to hold the task's data structure. */
}

void TaskHelper::Run(void* parm)
{
    iActiveTask* task = (iActiveTask*)parm;
    task->Run();
#if INCLUDE_vTaskDelete
    vTaskDelete(task->taskHandle);
#endif
}
