#pragma once
#include "stdint.h"

class HeaterTempLimit final
{
public:
	
	// Если датчик окружающего воздуха был инициализирован то возвращает желаемую температуру воды в баке.
	bool TryGetTargetTemperature(uint8_t& internalTempLimit);
	
	// Если датчик окружающего воздуха был инициализирован то возвращает температуру окружающего воздуха в градусах.
	bool TryGetLastExternalTemp(uint8_t& lastExternalTemp);
	
private:

	// Температура окружающего воздуха в градусах.
	volatile uint8_t m_lastExternalTemp;
};

extern HeaterTempLimit g_heaterTempLimit;
