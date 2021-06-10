#pragma once
#include "stdint.h"
#include "iActiveTask.h"
#include "TickCounter.h"
#include "HeaterWatchdog.h"

class HeaterTask : public iActiveTask
{
public:
	
	bool GetIsHeaterEnabled();
	bool WaterHeated();   // True если вода нагрета до нужного уровня
	uint8_t GetHeatingLimit();
	bool GetTimeoutOccured();
	bool GetAbsoluteTimeoutOccured();
	void ResetBeepTime();
	
private:
	
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
};

extern HeaterTask _heaterTask;