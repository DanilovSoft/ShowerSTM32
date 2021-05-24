#pragma once
#include "stdint.h"
#include "RTCInterval.h"
#include "math.h"
#include "Common.h"

#define M_PI			3.14159265358979323846  /* pi */
#define Q				(0.00117)

class HeatingTimeLeft
{
private:
    //RTCInterval _rtcInterval;

	// Объём вобы полного бака в литрах.
	float _tankVolumeLitre;

	// Электрическая мощность нагревательного элемента — ТЭНа с учётом его КПД, кВТ.
	float _heaterPowerKWatt;
	
	// Время до окончания нагрева в минутах.
	// "internalTemp" - Текущая температура воды в баке.
	// "limitTemp" - Целевая темпаратура.
    byte CalcTimeLeft(float internalTemp, byte targetTemp, byte volumePercent);
	
public:
	
	// Вместо конструктора.
	void Init(float tankVolumeLitre, float heaterPowerKWatt);
	
	// Вызывается как событие когда включается нагрев.
    void OnStartHeating();
	
	// Возвращает время до окончания нагрева в минутах.
    byte GetTimeLeft();
	
	// Возвращает прогресс нагрева воды от 0 до 100%.
	byte GetProgress();
};

extern HeatingTimeLeft _heatingTimeLeft;