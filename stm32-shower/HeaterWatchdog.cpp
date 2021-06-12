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

void HeaterWatchdog::ResetSession()
{
    m_timeoutStopwatch.Reset();
    m_sessionTimeoutOccurred = false;
}
	
void HeaterWatchdog::ResetAbsolute()
{
    m_absoluteTimeoutStopwatch.Reset();
    m_absoluteTimeoutOccured = false;
}
	
bool HeaterWatchdog::TimeOut()
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

bool HeaterWatchdog::IsSessionTimeoutOccurred()
{
	return m_sessionTimeoutOccurred;
}

bool HeaterWatchdog::IsAbsoluteTimeoutOccured()
{
	return m_absoluteTimeoutOccured;
}