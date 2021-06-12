#pragma once
#include "iActiveTask.h"

class ButtonsTask final : public iActiveTask
{
private:
	void Init();
	void Run();
	void PressSound();
	void LongPressSound();
	void TempPlus();
	void TempMinus();
	void WaterPushButton();
};

extern ButtonsTask g_buttonsTask;