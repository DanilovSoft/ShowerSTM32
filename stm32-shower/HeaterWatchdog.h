#pragma once
#include "RTCInterval.h"

class HeaterWatchdog
{
public:

	volatile bool TimeoutOccurred;
	volatile bool AbsoluteTimeoutOccured;

	void Init();
	void Reset();
	void ResetAbsolute();
	bool TimeOut();
	// Абсолютный таймаут отсчитывает время когда включен рубильник автомата.
    bool AbsoluteTimeout();
	
private:
	
	uint32_t _intervalSec;
	uint32_t _absoluteIntervalSec;	
	RTCInterval _timeoutCounter;
	RTCInterval _absoluteTimeoutCounter;
};