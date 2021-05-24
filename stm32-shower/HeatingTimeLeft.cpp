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

byte HeatingTimeLeft::CalcTimeLeft(float internalTemp, byte targetTemp, byte volumePercent)
{
	if (internalTemp >= targetTemp)
	{
		return 0;
	}
	
	//  Формула расчета времени нагрева T = 0,00117 * V * (tк - tн) / W
	//  Т – время нагрева воды, час
	//  V – объем водонагревательного бака(л)
	//  tк – конечная температура воды, °С(обычно 60°C)
	//  tн – начальная температура воды, °С
	//  W – электрическая мощность нагревательного элемента — ТЭНа, кВТ

	double timeH = Q * _tankVolumeLitre * (targetTemp - internalTemp) / _heaterPowerKWatt;

	// В минутах.
    byte minutes = round(timeH * 60.0);
	
	if (minutes == 0)
	{
		minutes = 1;  // Нет смысла отображать что осталось 0 минут.
	}
				
	return minutes;
}

void HeatingTimeLeft::OnStartHeating()
{
    //_rtcInterval.Reset();
}

byte HeatingTimeLeft::GetTimeLeft()
{
    float intTemp = _tempSensorTask.AverageInternalTemp;
    float extTemp = _tempSensorTask.AverageExternalTemp;
	
	// Узнаём желаемую температуру воды в баке.
	byte limitTemp;
	_heaterTempLimit.TryGetTargetTemperature(limitTemp);

	// Нужно учесть на сколько процентов заполнен бак.
	byte volumePercent = _waterLevelTask.DisplayingPercent;
	
	byte minutes = CalcTimeLeft(intTemp, limitTemp, volumePercent);
    return minutes;
}

byte HeatingTimeLeft::GetProgress()
{
    float internalTemp = _tempSensorTask.AverageInternalTemp;
    
	// Узнаём желаемую температуру воды в баке.
	byte limitTemp;
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
