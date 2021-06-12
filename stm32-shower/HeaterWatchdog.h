#pragma once
#include "RealTimeClockStopwatch.h"

class HeaterWatchdog final
{
public:

	void Init();
	void Reset();
	void ResetAbsolute();
	bool TimeOut();
	// Абсолютный таймаут отсчитывает время когда включен рубильник автомата.
    bool AbsoluteTimeout();
	bool IsTimeoutOccurred() const;
	bool IsAbsoluteTimeoutOccured() const;
	
private:
	
	volatile bool m_timeoutOccurred;
	volatile bool m_absoluteTimeoutOccured;
	uint32_t m_intervalSec;
	uint32_t m_absoluteIntervalSec;	
	RealTimeClockStopwatch m_timeoutStopwatch;
	RealTimeClockStopwatch m_absoluteTimeoutStopwatch;
};