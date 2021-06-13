#pragma once
#include "RealTimeClockStopwatch.h"

class HeaterWatchdog final
{
public:

    HeaterWatchdog()
    {
    }
    
    HeaterWatchdog(uint32_t interval_sec, uint32_t absolute_interval_sec)
    {
        m_intervalSec = interval_sec;
        m_absoluteIntervalSec = absolute_interval_sec;
        m_timeoutStopwatch.Reset();
        m_absoluteTimeoutStopwatch.Reset();
        m_sessionTimeoutOccurred = false;
        m_absoluteTimeoutOccured = false;
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

    bool IsSessionTimeoutOccurred()
    {
        return m_sessionTimeoutOccurred;
    }

    bool IsAbsoluteTimeoutOccured()
    {
        return m_absoluteTimeoutOccured;
    }
    
private:
    
    uint32_t m_intervalSec;
    uint32_t m_absoluteIntervalSec;	
    RealTimeClockStopwatch m_timeoutStopwatch;
    RealTimeClockStopwatch m_absoluteTimeoutStopwatch;
    volatile bool m_sessionTimeoutOccurred;
    volatile bool m_absoluteTimeoutOccured;
};