#pragma once
#include "iActiveTask.h"
#include "LiquidCrystal.h"

class LcdTask final : public iActiveTask
{
private:

    LiquidCrystal m_lc;
	void Init();
	void Run();
	void DisplayTemp(char* buf, int16_t temp);
};

extern LcdTask g_lcdTask;
