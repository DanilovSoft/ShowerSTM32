#pragma once
#include "stdint.h"
#include "RTCInterval.h"

#define Tank_Height                     (297)
#define Tank_Diameter                   (400)
#define Heater_Resistance               (31)


class HeatingTimeLeft
{
    RTCInterval rtcInterval;
    static uint8_t CalcTimeLeft(float internalTemp, uint8_t limitTemp, uint8_t volumePercent);
public:
    void StartHeating();
    uint8_t GetTimeLeft();
    uint8_t GetProgress();
    uint8_t GetProgress2();
};

extern HeatingTimeLeft heatingTimeLeft;