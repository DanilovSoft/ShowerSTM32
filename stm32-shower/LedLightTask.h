#pragma once
#include "iActiveTask.h"

#define GPIO_LED        (GPIOB)
#define GPIO_Pin_LED    (GPIO_Pin_7)
#define LED_TIM         (TIM4)

class LedLightTask : public iActiveTask
{
	bool _turnedOn;
	void Init();
	void Run();
};

extern LedLightTask _ledLightTask;