#pragma once
#include "stdint.h"
#include "iActiveTask.h"
#include "Stopwatch.h"
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
	// Разрешает включить нагрев воды для текущей сессии (сессия — пока не выключат автомат).
	void IgnoreWaterLevelOnce();
	
private:
	
	bool _heaterHasPower;
	bool _heaterEnabled;
	Stopwatch _beepStopwatch; // Для периодического звукового сигнала.
	HeaterWatchdog _heaterWatchdog; // Для защиты от слишком долгого нагрева.
	volatile bool _forcedSessionRequired; // Для принудительного включения нагрева игнорируя датчик уровня воды.
	
	void Init();
	void Run();
	void BeepHeating();
	void BeepTurnOff();
	void BeepTurnOn();
	void BeepReady();
	void BeepTimeout();
	void ControlTurnOn();
	void ControlTurnOff();
	void TurnOff();
	void TurnOffWithNoSound();
	void TurnOnWithNoSound();
	void TurnOn();
};

extern HeaterTask _heaterTask;