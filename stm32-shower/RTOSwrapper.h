#pragma once
#include "iActiveTask.h"

class RTOSwrapperClass final
{   
public:
	
	static void Run(void* parm);
	void CreateTask(iActiveTask* obj, const char* name, UBaseType_t uxPriority);
};

extern RTOSwrapperClass g_rtosHelper;
