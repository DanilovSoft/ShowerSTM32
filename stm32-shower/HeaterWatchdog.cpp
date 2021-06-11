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
		
    _timeoutCounter.Reset();
    _absoluteTimeoutCounter.Reset();
}

void HeaterWatchdog::Reset()
{
    _timeoutCounter.Reset();
    TimeoutOccurred = false;
}
	
void HeaterWatchdog::ResetAbsolute()
{
    _absoluteTimeoutCounter.Reset();
    AbsoluteTimeoutOccured = false;
}
	
bool HeaterWatchdog::TimeOut()
{
	if (TimeoutOccurred)
	{
		return true;
	}
		
    if (_timeoutCounter.Timeout(_intervalSec))
    {
        TimeoutOccurred = true;
        return true;
    }
		
    return false;
}
	
bool HeaterWatchdog::AbsoluteTimeout()
{
	if (AbsoluteTimeoutOccured)
	{
		return true;
	}
		
    if (_absoluteTimeoutCounter.Timeout(_absoluteIntervalSec))
    {
        AbsoluteTimeoutOccured = true;
        return true;
    }
		
    return false;
}
