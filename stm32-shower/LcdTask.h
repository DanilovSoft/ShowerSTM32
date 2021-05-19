#pragma once
#include "iActiveTask.h"
#include "LiquidCrystal.h"


class LcdTask : public iActiveTask
{
    LiquidCrystal _lc;
	void Init();
	void DisplayTemp(char* buf, int16_t temp);
	void Run();
};

extern LcdTask _lcdTask;