#pragma once
#include "RTCInterval.h"

class HeaterWatchdog
{
	uint32_t interval;
	uint32_t absoluteInterval;	
	RTCInterval timeoutCounter;
	RTCInterval absoluteTimeoutCounter;
	
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