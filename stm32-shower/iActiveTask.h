#pragma once
#include "FreeRTOS.h"
#include "task.h"

class iActiveTask
{	
public:
    virtual void Init(void) = 0;
    virtual void Run(void) = 0;
    const static uint32_t ulStackDepth = configMINIMAL_STACK_SIZE;
	
    TaskHandle_t taskHandle;
	
    /* Buffer that the task being created will use as its stack.  Note this is
    an array of StackType_t variables.  The size of StackType_t is dependent on
    the RTOS port. */
    StackType_t xStack[ulStackDepth];

    /* Structure that will hold the TCB of the task being created. */
    StaticTask_t xTaskBuffer;
};
