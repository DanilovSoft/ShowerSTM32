#pragma once
#include "FreeRTOS.h"
#include "task.h"

class TaskBase
{	
public:
    
    static constexpr uint32_t kULStackDepth = configMINIMAL_STACK_SIZE;
    
    // Точка входа потока FreeRTOS.
    virtual void Run(void)
    {
        
    }
    
    TaskHandle_t taskHandle;
    
    /* Buffer that the task being created will use as its stack.  Note this is
    an array of StackType_t variables.  The size of StackType_t is dependent on
    the RTOS port. */
    StackType_t xStack[kULStackDepth];

    /* Structure that will hold the TCB of the task being created. */
    StaticTask_t xTaskBuffer;
};
