#pragma once
#include "stdint.h"
#include "iActiveTask.h"
#include "Stopwatch.h"
#include "HeaterWatchdog.h"

class HeaterTask final : public iActiveTask
{
public:
	
	bool GetIsHeaterEnabled();
	bool WaterHeated();   // True если вода нагрета до нужного уровня
	uint8_t GetHeatingLimit();
	bool GetTimeoutOccured() const;
	bool GetAbsoluteTimeoutOccured() const;
	void ResetBeepTime();
	// Разрешает включить нагрев воды для текущей сессии (сессия — пока не выключат автомат).
	void IgnoreWaterLevelOnce();
	
private:
	
	bool m_heaterHasPower;
	bool m_heaterEnabled;
	Stopwatch m_beepStopwatch; // Для периодического звукового сигнала.
	HeaterWatchdog m_heaterWatchdog; // Для защиты от слишком долгого нагрева.
	volatile bool m_forcedSessionRequired; // Для принудительного включения нагрева игнорируя датчик уровня воды.
	
	void Init();
	void Run();
	// Воспроизводит звук активного процесса нагрева.
	// Блокирует поток на время воспроизведения.
	void BeepHeating();
	// Воспроизводит звук при отключении питания тена (когда отключается реле).
	// Блокирует поток на время воспроизведения.
	void BeepTurnOff();
	// Воспроизводит звук при подаче питания тена (когда включается реле).
	// Блокирует поток на время воспроизведения.
	void BeepTurnOn();
	// Воспроизводит звук когда вода нагрета до требуемой температуры.
	// Блокирует поток на время воспроизведения.
	void BeepReady();
	void BeepTimeout();
	void ControlTurnOn();
	void ControlTurnOff();
	void TurnOff();
	void TurnOffWithNoSound();
	void TurnOnWithNoSound();
	void TurnOn();
};

extern HeaterTask g_heaterTask;