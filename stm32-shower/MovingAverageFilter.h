#pragma once
#include "stdint.h"
#include "Properties.h"

// Фильтр скользящее среднее.
class MovingAverageFilter final
{	
public:
	uint16_t AddValue(uint16_t value);
	uint32_t GetAverage();
	
private:
	
    uint16_t m_buf[WL_AVG_BUF_MAX_SIZE] = { }; // Инициализировать обнуленным.
	uint32_t m_sum = 0;
	uint8_t m_head = 0;
};
