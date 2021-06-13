#pragma once
#include "stm32f10x_rtc.h"
#include "Common.h"

// Замеряет время на основе часов реального времени (RTC).
class RealTimeClockStopwatch final
{
public:
    
    void Reset()
    {
        m_lastTime = RTC_GetCounter();
    }

    uint32_t ElapsedSeconds()
    {
        return Common::abs(RTC_GetCounter(), m_lastTime);
    }

    bool Timeout(uint16_t seconds)
    {
        uint32_t elapsed_sec = Common::abs(RTC_GetCounter(), m_lastTime);
        return elapsed_sec > seconds;
    }
    
private:
    
    volatile uint32_t m_lastTime;
};

