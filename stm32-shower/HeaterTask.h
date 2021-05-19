#pragma once
#include "stdint.h"
#include "iActiveTask.h"
#include "TickCounter.h"
#include "HeaterWatchdog.h"

#define GPIO_Heater             (GPIOA)
#define GPIO_Pin_Heater         (GPIO_Pin_10)
#define GPIO_Heater_Led         (GPIOC)
#define GPIO_Heater_Led_Pin     (GPIO_Pin_13)

class HeaterTask : public iActiveTask
{
	bool _heaterHasPower;
	bool _heaterEnabled;
	TickCounter _tickCounter;
	HeaterWatchdog _heaterWatchdog;
	
	void BeepHeating();
	void BeepTurnOff();
	void BeepTurnOn();
	void BeepReady();
	void BeepTimeout();
	void Init();
	void Run();
	void ControlTurnOn();
	void ControlTurnOff();
	void TurnOff();
	void TurnOffNoSound();
	void TurnOnNoSound();
	void TurnOn();
public:
	bool GetIsHeaterEnabled();
	bool WaterHeated();  // True если вода нагрета до нужного уровня
	uint8_t GetHeatingLimit();
	bool GetTimeoutOccured();
	bool GetAbsoluteTimeoutOccured();
	void ResetBeepTime();
};

extern HeaterTask _heaterTask;