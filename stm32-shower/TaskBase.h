#pragma once
#include "FreeRTOS.h"
#include "task.h"

class TaskBase
{	
public:
    
    static constexpr uint32_t kULStackDepth = configMINIMAL_STACK_SIZE;
    
    // Точка входа потока FreeRTOS.
    virtual void Run(void) = 0;
    
    TaskHandle_t m_taskHandle;
    
    /* Buffer that the task being created will use as its stack.  Note this is
    an array of StackType_t variables.  The size of StackType_t is dependent on
    the RTOS port. */
    StackType_t xStack[kULStackDepth];

    /* Structure that will hold the TCB of the task being created. */
    StaticTask_t xTaskBuffer;

    // Точка входа потока.
    static void InnerRun(void* parm)
    {
        TaskBase* task = (TaskBase*)parm;
        task->Run();
     
        // ! ВНИМАНИЕ ! Если таск завершится, а опция INCLUDE_vTaskDelete будет выключена 
        // то диспетчер потоков заглохнет и сработает сторожевой таймер.
        
#if INCLUDE_vTaskDelete
        vTaskDelete(task->m_taskHandle);
#else
        __asm("bkpt 255");
#endif
    }
    
    void StartTask(const char* name, UBaseType_t uxPriority = tskIDLE_PRIORITY)
    {
        m_taskHandle = xTaskCreateStatic(
            /* Function that implements the task. */
            InnerRun,
            /* Text name for the task. */
            name,
            /* Размер массива xStack */
            TaskBase::kULStackDepth,
            /* Parameter passed into the task.*/
            this,
            /* Priority at which the task is created. */
            uxPriority,
            /* Array to use as the task's stack. */
            xStack,
            /* Variable to hold the task's data structure. */
            &xTaskBuffer);
    }
};
