#pragma once
#include "iActiveTask.h"

class RTOSwrapperClass
{   
public:
	void CreateTask(iActiveTask* obj, const char* name, UBaseType_t uxPriority);
	static void Run(void* parm);
};

extern RTOSwrapperClass _rtosHelper;
