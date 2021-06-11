#include "HeaterWatchdog.h"
#include "stm32f10x.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_pwr.h"
#include "system_stm32f10x.h"
#include "Properties.h"
#include "HeaterTask.h"


void HeaterWatchdog::Init()
{	
    _intervalSec = Properties.HeatingTimeLimitMin * 60;
	
	// Переводим в секунды.
    _absoluteIntervalSec = Properties.AbsoluteHeatingTimeLimitHours * 60 * 60;
		
    _timeoutStopwatch.Reset();
    _absoluteTimeoutStopwatch.Reset();
}

void HeaterWatchdog::Reset()
{
    _timeoutStopwatch.Reset();
    _timeoutOccurred = false;
}
	
void HeaterWatchdog::ResetAbsolute()
{
    _absoluteTimeoutStopwatch.Reset();
    _absoluteTimeoutOccured = false;
}
	
bool HeaterWatchdog::TimeOut()
{
	if (_timeoutOccurred)
	{
		return true;
	}
		
    if (_timeoutStopwatch.Timeout(_intervalSec))
    {
        _timeoutOccurred = true;
        return true;
    }
		
    return false;
}
	
bool HeaterWatchdog::AbsoluteTimeout()
{
	if (_absoluteTimeoutOccured)
	{
		return true;
	}
		
    if (_absoluteTimeoutStopwatch.Timeout(_absoluteIntervalSec))
    {
        _absoluteTimeoutOccured = true;
        return true;
    }
		
    return false;
}

bool HeaterWatchdog::IsTimeoutOccurred()
{
	return _timeoutOccurred;
}

bool HeaterWatchdog::IsAbsoluteTimeoutOccured()
{
	return _absoluteTimeoutOccured;
}