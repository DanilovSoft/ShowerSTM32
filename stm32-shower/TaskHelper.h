#pragma once
#include "iActiveTask.h"

class TaskHelper
{   
public:
	
	static void Run(void* parm);
	static void CreateTask(TaskHelper* taskHelper, void* func, const char* name);
	void CreateTask(iActiveTask* obj, const char* name, UBaseType_t uxPriority);
	
private:
	
	static const uint32_t ulStackDepth = configMINIMAL_STACK_SIZE;

    TaskHandle_t taskHandle;

    /* Buffer that the task being created will use as its stack.  Note this is
    an array of StackType_t variables.  The size of StackType_t is dependent on
    the RTOS port. */
    StackType_t xStack[ulStackDepth];

    /* Structure that will hold the TCB of the task being created. */
    StaticTask_t xTaskBuffer;
};

extern TaskHelper g_taskHelper;
