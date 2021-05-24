#pragma once
#include "RTCInterval.h"

class HeaterWatchdog
{
private:
	
	uint32_t _interval;
	uint32_t _absoluteIntervalSec;	
	RTCInterval _timeoutCounter;
	RTCInterval _absoluteTimeoutCounter;
	
public:

	volatile bool TimeOutOccurred;
	volatile bool AbsoluteTimeOutOccured;
	
    void Init();
	
    void Reset();
	
    void ResetAbsolute();
	    
    bool TimeOut();
	
	// абсолютный таймаут отсчитывает врем¤ когда включен рубильник автомата
    bool AbsoluteTimeout();
};