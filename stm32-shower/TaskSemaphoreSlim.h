#pragma once
#include "core_cm3.h"
#include "Interlocked.h"
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"

class TaskSemaphoreSlim
{
	volatile uint8_t Lock_Variable;
	
public:

	void Take()
	{
		uint32_t status = 0;
		do 
		{
			while (__LDREXB(&Lock_Variable) != 0)
			{
				taskYIELD();
			}; // Wait until
			
			// Lock_Variable is free
			status = __STREXB(1, &Lock_Variable); // Try to set
			
			if (status == 0)
				break;
			
			taskYIELD();
			
		} while (1); //retry until lock successfully
		
		__DMB();		// Do not start any other memory access
		// until memory barrier is completed
		return;
	}
	
	void Give()
	{ // Note: __LDREXW and __STREXW are CMSIS functions
		
		__DMB(); // Ensure memory operations completed before
		
		// releasing lock
		Lock_Variable = 0;
		return;
	}
};

