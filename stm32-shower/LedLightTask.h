#pragma once
#include "iActiveTask.h"

class LedLightTask final : public iActiveTask
{
private:
	
	bool m_turnedOn;
	void Init();
	void Run();
};

extern LedLightTask g_ledLightTask;
