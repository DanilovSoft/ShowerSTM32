#pragma once
#include "iActiveTask.h"


#define Button_GPIO                     (GPIOA)
#define Button_Temp_Minus               (GPIO_Pin_2)
#define Button_Temp_Plus                (GPIO_Pin_3)
#define Button_Water                    (GPIO_Pin_4)
#define Button_SensorSwitch_OUT         (GPIO_Pin_7)

class ButtonsTask : public iActiveTask
{
private:
	void Init();
	void PressSound();
	void LongPressSound();
	void Run();
	void TempPlus();
	void TempMinus();
	void WaterPushButton();
};

extern ButtonsTask _buttonsTask;