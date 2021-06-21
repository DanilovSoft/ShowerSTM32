#pragma once
#include "stdint.h"
#include "Properties.h"
#include "math.h"
#include "Common.h"
#include "TempSensorTask.h"

class HeaterTempLimit final
{
public:

	// Если датчик окружающего воздуха был инициализирован то возвращает желаемую температуру воды в баке.
	bool TryGetTargetTemperature(uint8_t& target_temp)
	{
		if (g_tempSensorTask->ExternalSensorInitialized)
		{
			float air_temp = g_tempSensorTask->AverageExternalTemp;
			uint8_t air_temp_c = round(air_temp);
		
			if (Common::abs(air_temp_c, m_airTemp) > 1)
			{
				m_airTemp = air_temp_c;
			}
			else
			{
				air_temp_c = m_airTemp;
			}
		
			target_temp = g_properties.Chart.GetLimit(air_temp_c);	
			return true;
		}
		return false;
	}
	
	// Если датчик окружающего воздуха был инициализирован то возвращает температуру окружающего воздуха в градусах.
	bool TryGetAirTemperature(uint8_t& air_temp)
	{
		if (g_tempSensorTask->ExternalSensorInitialized)
		{
			air_temp = m_airTemp;
			return true;
		}
		return false;
	}
	
private:

	// Температура окружающего воздуха в градусах.
	volatile uint8_t m_airTemp;
};

extern HeaterTempLimit g_heaterTempLimit;
