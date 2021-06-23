#pragma once
#include "stdint.h"
#include "Common.h"

// Фильтр скользящее среднее.
class MovingAverageFilter final
{	
public:
    
    void Init(const uint8_t buffer_size)
    {
        m_bufferSize = buffer_size;
    }
    
    uint16_t AddValue(uint16_t value)
    {
        m_sum = m_sum + (value - m_buf[m_head]);
        m_buf[m_head] = value;
        m_head = (m_head + 1) % m_bufferSize;
        return m_sum / m_bufferSize;
    }

    uint32_t GetAverage()
    {
        return m_sum;
    }
    
private:
    
    uint8_t m_bufferSize;
    uint16_t m_buf[kWaterLevelAvgFilterMaxSize] = {0}; // Инициализировать обнуленным.
    uint32_t m_sum = 0;
    uint8_t m_head = 0;
};
