#pragma once
#include "iActiveTask.h"

class WatchDogTask : public iActiveTask
{
	void Init();
	void Run();
public:
	bool WasReset = false;
};

extern WatchDogTask _watchDogTask;
