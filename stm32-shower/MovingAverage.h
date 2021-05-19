#pragma once
#include "stdint.h"
#include "Properties.h"

class MovingAverage
{	
    uint16_t _buf[WL_BUF_MAX_SIZE] = { }; // инициализировать обнуленным
	uint32_t _sum = 0;
	uint8_t _head = 0;
	
public:
    void Init();
	uint16_t moving_average(uint16_t value);
    uint32_t get_sum();
};
