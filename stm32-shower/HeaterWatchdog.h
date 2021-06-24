#pragma once
#include "RealTimeClockStopwatch.h"

class HeaterWatchdog final
{
public:

    void Init(const uint32_t interval_sec, const uint32_t absolute_interval_sec)
    {
        m_intervalSec = interval_sec;
        m_absoluteIntervalSec = absolute_interval_sec;
        m_timeoutStopwatch.Reset();
        m_absoluteTimeoutStopwatch.Reset();
    }

    void ResetSession()
    {
        m_timeoutStopwatch.Reset();
        m_sessionTimeoutOccurred = false;
    }
    
    void ResetAbsolute()
    {
        m_absoluteTimeoutStopwatch.Reset();
        m_absoluteTimeoutOccured = false;
    }
    
    bool TimeOut()
    {
        if (m_sessionTimeoutOccurred)
        {
            return true;
        }
        
        if (m_timeoutStopwatch.Timeout(m_intervalSec))
        {
            m_sessionTimeoutOccurred = true;
            return true;
        }
        
        return false;
    }
    
    // Абсолютный таймаут отсчитывает время когда включен рубильник автомата.
    bool AbsoluteTimeout()
    {
        if (m_absoluteTimeoutOccured)
        {
            return true;
        }
        
        if (m_absoluteTimeoutStopwatch.Timeout(m_absoluteIntervalSec))
        {
            m_absoluteTimeoutOccured = true;
            return true;
        }
        
        return false;
    }

    bool IsSessionTimeoutOccurred() volatile
    {
        return m_sessionTimeoutOccurred;
    }

    bool IsAbsoluteTimeoutOccured() volatile
    {
        return m_absoluteTimeoutOccured;
    }
    
private:
    
    uint32_t m_intervalSec = 0;
    uint32_t m_absoluteIntervalSec = 0;
    RealTimeClockStopwatch m_timeoutStopwatch;
    RealTimeClockStopwatch m_absoluteTimeoutStopwatch;
    volatile bool m_sessionTimeoutOccurred = false;
    volatile bool m_absoluteTimeoutOccured = false;
};