#pragma once
#include "Stopwatch.h"

class TaskTimeout
{
public:
	
	TaskTimeout(uint16_t timeoutMsec)
		: _timeoutMsec(timeoutMsec)
	{
		_counter.Reset();
	}

	uint32_t GetElapsedTicks()
	{
		return _counter.GetElapsedTicks();
	}
    
	bool TimeIsUp()
	{
		return _counter.TimedOut(_timeoutMsec);
	}
	
private:
	
	const uint16_t _timeoutMsec;
	Stopwatch _counter;
};
