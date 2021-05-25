#pragma once
#include "Common.h"
#include "TickCounter.h"

class TaskTimeout
{
private:
	
	uint16_t _timeoutMsec;
	TickCounter _counter;

public:
	
	TaskTimeout(uint16_t timeoutMsec)
	{
		_timeoutMsec = timeoutMsec;
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
};
