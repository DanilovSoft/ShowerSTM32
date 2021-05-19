#include "MovingAverage.h"

uint16_t MovingAverage::moving_average(uint16_t value)
{
    _sum = _sum + (value - _buf[_head]);
	_buf[_head] = value;
    _head = (_head + 1) % Properties.Customs.WaterLevel_Ring_Buffer_Size;
    return _sum / Properties.Customs.WaterLevel_Ring_Buffer_Size;
}

uint32_t MovingAverage::get_sum()
{
    return _sum;
}

void MovingAverage::Init()
{
    // буфер уже обнуленный
//    _head = 0;
//    _sum = 0;
//    for (auto i = 0; i < WL_BUF_MAX_SIZE; i++)
//    {
//        _buf[i] = 0;
//    }
}