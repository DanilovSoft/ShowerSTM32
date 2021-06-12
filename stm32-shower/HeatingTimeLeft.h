#pragma once
#include "stdint.h"

class HeatingTimeLeft final
{
public:
	
	// Вместо конструктора.
	void Init(float tankVolumeLitre, float heaterPowerKWatt);
	
	// Вызывается как событие когда включается нагрев.
    void OnStartHeating();
	
	// Возвращает время до окончания нагрева в минутах (от 0 до x).
    float GetTimeLeft();
	
	// Возвращает прогресс нагрева воды от 0 до 100%.
	uint8_t GetProgress();
	
private:
    
	static constexpr float kQ = 0.00117;
	
	// Объём воды полного бака в литрах.
	float m_tankVolumeLitre;

	// Электрическая мощность нагревательного элемента — ТЭНа с учётом его КПД, кВТ.
	float m_heaterPowerKWatt;
	
	// Время до окончания нагрева в минутах.
	// "internalTemp" - Текущая температура воды в баке.
	// "limitTemp" - Целевая темпаратура.
    float CalcTimeLeft(float internalTemp, uint8_t targetTemp, uint8_t tankPercent);
};

extern HeatingTimeLeft g_heatingTimeLeft;