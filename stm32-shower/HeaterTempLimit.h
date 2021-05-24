#pragma once
#include "stdint.h"

class HeaterTempLimit
{
private:

	// Температура окружающего воздуха в градусах.
	volatile uint8_t _lastExternalTemp;

public:
	
	// Если датчик окружающего воздуха был инициализирован то возвращает желаемую температуру воды в баке.
	bool TryGetTargetTemperature(uint8_t& internalTempLimit);
	
	// Если датчик окружающего воздуха был инициализирован то возвращает температуру окружающего воздуха в градусах.
	bool TryGetLastExternalTemp(uint8_t& lastExternalTemp);
};

extern HeaterTempLimit _heaterTempLimit;