#pragma once
#include "stdint.h"
#include "Properties.h"

// Фильтр скользящее среднее.
class MovingAverageFilter
{	
    uint16_t _buf[WL_AVG_BUF_MAX_SIZE] = { }; // инициализировать обнуленным
	uint32_t _sum = 0;
	uint8_t _head = 0;
	
public:
    void Init();
	uint16_t AddValue(uint16_t value);
    uint32_t GetAverage();
};
