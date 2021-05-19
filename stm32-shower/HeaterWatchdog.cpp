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
    interval = Properties.HeatingTimeLimitMin * 60;
    absoluteInterval = Properties.AbsoluteHeatingTimeLimitHours * 60 * 60;
		
    timeoutCounter.Reset();
    absoluteTimeoutCounter.Reset();
}
	

void HeaterWatchdog::Reset()
{
    timeoutCounter.Reset();
    TimeOutOccurred = false;
}
	

void HeaterWatchdog::ResetAbsolute()
{
    absoluteTimeoutCounter.Reset();
    AbsoluteTimeOutOccured = false;
}
	

bool HeaterWatchdog::TimeOut()
{
    if (TimeOutOccurred)
        return true;
		
    if (timeoutCounter.Timeout(interval))
    {
        TimeOutOccurred = true;
        return true;
    }
		
    return false;
}
	

// јбсолютный таймаут отсчитывает врем¤ когда включен рубильник автомата
bool HeaterWatchdog::AbsoluteTimeout()
{
    if (AbsoluteTimeOutOccured)
        return true;
		
    if (absoluteTimeoutCounter.Timeout(absoluteInterval))
    {
        AbsoluteTimeOutOccured = true;
        return true;
    }
		
    return false;
}