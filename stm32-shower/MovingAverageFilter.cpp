#include "MovingAverageFilter.h"

uint16_t MovingAverageFilter::AddValue(uint16_t value)
{
    m_sum = m_sum + (value - m_buf[m_head]);
	m_buf[m_head] = value;
    m_head = (m_head + 1) % g_properties.WaterLevel_Avg_Buffer_Size;
    return m_sum / g_properties.WaterLevel_Avg_Buffer_Size;
}

uint32_t MovingAverageFilter::GetAverage()
{
    return m_sum;
}
