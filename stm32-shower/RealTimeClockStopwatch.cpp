#include "RealTimeClockStopwatch.h"
#include "stm32f10x_rtc.h"
#include "Common.h"

void RealTimeClockStopwatch::Reset()
{
    m_lastTime = RTC_GetCounter();
}

uint32_t RealTimeClockStopwatch::ElapsedSeconds()
{
    return abs(RTC_GetCounter(), m_lastTime);
}

bool RealTimeClockStopwatch::Timeout(uint16_t seconds)
{
    uint32_t elapsedSec = abs(RTC_GetCounter(), m_lastTime);
    return elapsedSec > seconds;
}