#pragma once
#include "iActiveTask.h"
#include "Stopwatch.h"
#include "HeaterWatchdog.h"

class HeaterTask final : public iActiveTask
{
public:
	
	bool GetIsHeaterEnabled();
	bool WaterHeated();   
	uint8_t GetHeatingLimit();
	bool GetTimeoutOccured();
	bool GetAbsoluteTimeoutOccured();
	void ResetBeepInterval();
	void IgnoreWaterLevelOnce();
	
private:
	
	bool m_heaterHasPower;
	bool m_heaterEnabled;
	Stopwatch m_beepStopwatch; // Для периодического звукового сигнала.
	HeaterWatchdog m_heaterWatchdog; // Для защиты от слишком долгого нагрева.
	volatile bool m_forcedSessionRequired; // Для принудительного включения нагрева игнорируя датчик уровня воды.
	
	void Init();
	void Run();
	void PeriodicBeepHeating();
	void BeepTurnOff();
	void BeepTurnOn();
	void PeriodicBeepIfWaterHeated();
	void PeriodicBeepTimeout();
	void ControlTurnOn();
	void ControlTurnOff();
	void TurnOffWithSound();
	void TurnOff();
	void TurnOn();
	void TurnOnWithSound();
};

extern HeaterTask g_heaterTask;
