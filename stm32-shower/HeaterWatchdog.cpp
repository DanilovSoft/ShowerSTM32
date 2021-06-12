#include "HeaterWatchdog.h"
#include "Properties.h"

void HeaterWatchdog::Init()
{	
    m_intervalSec = g_properties.HeatingTimeLimitMin * 60;
	
	// Переводим в секунды.
    m_absoluteIntervalSec = g_properties.AbsoluteHeatingTimeLimitHours * 60 * 60;
		
    m_timeoutStopwatch.Reset();
    m_absoluteTimeoutStopwatch.Reset();
}

void HeaterWatchdog::Reset()
{
    m_timeoutStopwatch.Reset();
    m_timeoutOccurred = false;
}
	
void HeaterWatchdog::ResetAbsolute()
{
    m_absoluteTimeoutStopwatch.Reset();
    m_absoluteTimeoutOccured = false;
}
	
bool HeaterWatchdog::TimeOut()
{
	if (m_timeoutOccurred)
	{
		return true;
	}
		
    if (m_timeoutStopwatch.Timeout(m_intervalSec))
    {
        m_timeoutOccurred = true;
        return true;
    }
		
    return false;
}
	
bool HeaterWatchdog::AbsoluteTimeout()
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

bool HeaterWatchdog::IsTimeoutOccurred()
{
	return m_timeoutOccurred;
}

bool HeaterWatchdog::IsAbsoluteTimeoutOccured()
{
	return m_absoluteTimeoutOccured;
}