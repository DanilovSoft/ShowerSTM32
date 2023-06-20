#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "Common.h"

// Измеряет время в миллисекундах на основе тиков FreeFTOS.
class Stopwatch final
{
public:
    
    Stopwatch()
    {
        Reset();
    }
    
    static auto GetTimestamp()
    {
        return xTaskGetTickCount();
    }
    
    static auto GetElapsed(uint32_t timestamp)
    {
        auto tickCount = xTaskGetTickCount();
        return Common::abs(tickCount, timestamp);
    }
    
    static auto GetElapsedMsec(uint32_t timestamp)
    {
        auto tickCount = xTaskGetTickCount();
        return Common::abs(tickCount, timestamp) * portTICK_PERIOD_MS;
    }
    
    static auto TimestampToMsec(uint32_t timestamp)
    {
        return timestamp * portTICK_PERIOD_MS;
    }
    
    auto GetTicks()
    {
        return m_tickCount;
    }
    
    // Сбрасывает отсчёт времени.
    void Reset()
    {
        m_tickCount = xTaskGetTickCount();
    }
    
    bool TimedOut(uint32_t msec)
    {
        return GetElapsedMsec() > msec;
    }
    
    // Возвращает число квантов времени FreeRTOS которое прошло с момента Reset.
    uint32_t GetElapsedTicks()
    {
        uint32_t cur_tick_count = xTaskGetTickCount();
        return Common::abs(cur_tick_count, m_tickCount);
    }
    
    // Возвращает сколько прошло миллисекунд с момента Reset.
    uint32_t GetElapsedMsec()
    {
        uint32_t cur_tick_count = xTaskGetTickCount();
        return Common::abs(cur_tick_count, m_tickCount) * portTICK_PERIOD_MS;
    }
    
private:
    
    volatile uint32_t m_tickCount;
};