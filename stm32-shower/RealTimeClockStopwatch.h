#pragma once
#include "stdint.h"

// Замеряет время на основе часов реального времени (RTC).
class RealTimeClockStopwatch final
{
public:
    
    void Reset();
    uint32_t ElapsedSeconds();
    bool Timeout(uint16_t seconds);
    
private:
    
    volatile uint32_t m_lastTime;
};

