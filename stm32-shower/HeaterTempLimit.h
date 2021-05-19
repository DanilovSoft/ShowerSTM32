#pragma once
#include "stdint.h"

class HeaterTempLimit
{
private:

	// Температура окружающего воздуха в градусах.
	volatile uint8_t _lastExternalTemp;

public:
	
	bool TryGetLimit(uint8_t& internalTempLimit);
	
	// Возвращает температуру окружающего воздуха в градусах.
	bool TryGetLastExternalTemp(uint8_t& lastExternalTemp);
};

extern HeaterTempLimit _heaterTempLimit;