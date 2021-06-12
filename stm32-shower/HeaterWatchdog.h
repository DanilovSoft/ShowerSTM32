#pragma once
#include "RealTimeClockStopwatch.h"

class HeaterWatchdog final
{
public:

	void Init();
	void ResetSession();
	void ResetAbsolute();
	bool TimeOut();
	// Абсолютный таймаут отсчитывает время когда включен рубильник автомата.
    bool AbsoluteTimeout();
	bool IsSessionTimeoutOccurred();
	bool IsAbsoluteTimeoutOccured();
	
private:
	
	uint32_t m_intervalSec;
	uint32_t m_absoluteIntervalSec;	
	RealTimeClockStopwatch m_timeoutStopwatch;
	RealTimeClockStopwatch m_absoluteTimeoutStopwatch;
	volatile bool m_sessionTimeoutOccurred;
	volatile bool m_absoluteTimeoutOccured;
};