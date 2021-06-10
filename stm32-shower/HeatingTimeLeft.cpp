#include "HeatingTimeLeft.h"
#include "stm32f10x_rtc.h"
#include "RTCInterval.h"
#include "HeaterTask.h"
#include "HeaterTempLimit.h"
#include "TempSensor.h"
#include "RTCInterval.h"
#include "WaterLevelTask.h"

HeatingTimeLeft _heatingTimeLeft;

void HeatingTimeLeft::Init(float tankVolumeLitre, float heaterPowerKWatt)
{
	_tankVolumeLitre = tankVolumeLitre;
	_heaterPowerKWatt = heaterPowerKWatt;
}

float HeatingTimeLeft::CalcTimeLeft(float internalTemp, uint8_t targetTemp, uint8_t tankPercent)
{
	if (internalTemp >= targetTemp)
	{
		return 0;
	}
	
	//  Формула расчета времени нагрева T = 0.00117 * V * (tк - tн) / W
	//  Т – время нагрева воды, час
	//  V – объем водонагревательного бака(л)
	//  tк – конечная температура воды, °С(обычно 60°C)
	//  tн – начальная температура воды, °С
	//  W – электрическая мощность нагревательного элемента — ТЭНа, кВТ

	float timeH = Q * _tankVolumeLitre * (targetTemp - internalTemp) / _heaterPowerKWatt;

	// В минутах.
    float minutes = timeH * 60;
			
	return minutes;
}

void HeatingTimeLeft::OnStartHeating()
{
    //_rtcInterval.Reset();
}

float HeatingTimeLeft::GetTimeLeft()
{
    float intTemp = _tempSensorTask.AverageInternalTemp;
    float extTemp = _tempSensorTask.AverageExternalTemp;
	
	// Узнаём желаемую температуру воды в баке.
	uint8_t limitTemp;
	_heaterTempLimit.TryGetTargetTemperature(limitTemp);

	// Нужно учесть на сколько процентов заполнен бак.
	uint8_t tankPercent = _waterLevelTask.DisplayingPercent;
	
	float minutes = CalcTimeLeft(intTemp, limitTemp, tankPercent);
    return minutes;
}

uint8_t HeatingTimeLeft::GetProgress()
{
    float internalTemp = _tempSensorTask.AverageInternalTemp;
    
	// Узнаём желаемую температуру воды в баке.
	uint8_t limitTemp;
	_heaterTempLimit.TryGetTargetTemperature(limitTemp);
		
	if (internalTemp > limitTemp)
	{
		internalTemp = limitTemp;
	}
			
    float percent = 100.0 / (limitTemp - LOWER_BOUND) * (internalTemp - LOWER_BOUND);
    percent = round(percent);
	if (percent > 100)
	{
		percent = 100;
	}
			
    return percent;
}
