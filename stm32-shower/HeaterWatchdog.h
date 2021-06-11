#pragma once
#include "RealTimeClockStopwatch.h"

class HeaterWatchdog
{
public:

	void Init();
	void Reset();
	void ResetAbsolute();
	bool TimeOut();
	// Абсолютный таймаут отсчитывает время когда включен рубильник автомата.
    bool AbsoluteTimeout();
	bool IsTimeoutOccurred();
	bool IsAbsoluteTimeoutOccured();
	
private:
	
	volatile bool _timeoutOccurred;
	volatile bool _absoluteTimeoutOccured;
	uint32_t _intervalSec;
	uint32_t _absoluteIntervalSec;	
	RealTimeClockStopwatch _timeoutStopwatch;
	RealTimeClockStopwatch _absoluteTimeoutStopwatch;
};