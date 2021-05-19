#include "HeatingTimeLeft.h"
#include "Common.h"
#include "stm32f10x_rtc.h"
#include "math.h"
#include "RTCInterval.h"
#include "HeaterTask.h"
#include "HeaterTempLimit.h"
#include "TempSensor.h"
#include "RTCInterval.h"
#include "WaterLevelTask.h"

HeatingTimeLeft heatingTimeLeft;


uint8_t HeatingTimeLeft::CalcTimeLeft(float internalTemp, uint8_t limitTemp, uint8_t volumePercent)
{
    if (internalTemp >= limitTemp)
        return 0;
		
    const float pi = 3.14159265358979323846;

    // V = Pi * (r^2) * h
    const float TANK_VOLUME_PER_PERCENT = (pi * pow(Tank_Diameter / 2.0, 2) * Tank_Height) / 99.0 / 1000000.0;		// Объем в литрах
    const float HEATER_POWER = (220.0 * (220.0 / Heater_Resistance)) / 1000.0;		// Мощность в киловатах
    float actualVolume = TANK_VOLUME_PER_PERCENT * volumePercent;
		
    // T= 0,00117*V*(tк-tн)/W
    float timeH = 0.00117 * actualVolume * (limitTemp - internalTemp) / HEATER_POWER;
		
    // В минутах
    uint8_t minutes = round(timeH * 60.0);
		
    if (minutes == 0)
        minutes = 1;
		
    return minutes;
}
	

void HeatingTimeLeft::StartHeating()
{
    rtcInterval.Reset();
}
	

uint8_t HeatingTimeLeft::GetTimeLeft()
{
    float intTemp = _tempSensorTask.AverageInternalTemp;
    float extTemp = _tempSensorTask.AverageExternalTemp;
	uint8_t limitTemp;
	_heaterTempLimit.TryGetLimit(limitTemp);

    uint8_t volumePercent = _waterLevelTask.DisplayingPercent;
    uint8_t minutes = CalcTimeLeft(intTemp, limitTemp, volumePercent);
    return minutes;
}
	

uint8_t HeatingTimeLeft::GetProgress()
{
    float intTemp = _tempSensorTask.AverageInternalTemp;
    
	uint8_t limitTemp;
	_heaterTempLimit.TryGetLimit(limitTemp);
		
    if (intTemp > limitTemp)
        intTemp = limitTemp;
			
    float percent = 100.0 / (limitTemp - LOWER_BOUND) * (intTemp - LOWER_BOUND);
    percent = round(percent);
    if (percent > 100)
        percent = 100;
			
    return percent;
}
	

uint8_t HeatingTimeLeft::GetProgress2()
{
    uint32_t time_passed_sec = rtcInterval.Elapsed();		// Сколько прошло времени с момента включения
    uint8_t min_left = GetTimeLeft();
    if (min_left == 0)
        return 100;
		
    uint32_t sec_total = time_passed_sec + (min_left * 60);
    uint8_t percent = round(10.0 / sec_total * time_passed_sec) * 10;
		
    if (percent > 90)
        percent = 98;
		
    return percent;
}