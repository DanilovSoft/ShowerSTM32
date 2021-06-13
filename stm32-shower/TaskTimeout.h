#pragma once
#include "Stopwatch.h"

class TaskTimeout final
{
public:
    
    TaskTimeout(uint16_t timeoutMsec)
        : m_timeoutMsec(timeoutMsec)
    {
        m_counter.Reset();
    }

    uint32_t GetElapsedTicks()
    {
        return m_counter.GetElapsedTicks();
    }
    
    bool TimeIsUp()
    {
        return m_counter.TimedOut(m_timeoutMsec);
    }
    
private:
    
    const uint16_t m_timeoutMsec;
    Stopwatch m_counter;
};
