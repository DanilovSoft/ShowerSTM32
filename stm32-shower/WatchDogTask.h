#pragma once
#include "iActiveTask.h"

class WatchDogTask final : public iActiveTask
{
public:
	
	bool GetWasReset();
	
private:
	
	bool m_wasReset = false;
	void Init();
	void Run();
};

extern WatchDogTask g_watchDogTask;
