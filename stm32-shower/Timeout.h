#pragma once
#include "Common.h"
#include "TickCounter.h"

class TaskTimeout
{
	uint16_t _timeout;
	TickCounter _counter;

public:
	TaskTimeout(uint16_t timeout)
	{
		this->_timeout = timeout;
		_counter.Restart();
	}

    uint32_t GetElapsedTicks()
    {
        return _counter.GetElapsedTicks();
    }
    
	bool TimeIsUp()
	{
		return _counter.TimeOut(_timeout);
	}
};
