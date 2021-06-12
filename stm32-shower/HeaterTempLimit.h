#pragma once
#include "stdint.h"
#include "TempSensor.h"
#include "Properties.h"
#include "math.h"
#include "Common.h"

class HeaterTempLimit final
{
public:

	// Если датчик окружающего воздуха был инициализирован то возвращает желаемую температуру воды в баке.
	bool TryGetTargetTemperature(uint8_t& internal_temp_limit)
	{
		if (g_tempSensorTask.ExternalSensorInitialized)
		{
			float air_temp = g_tempSensorTask.AverageExternalTemp;
			uint8_t air_temp_c = round(air_temp);
		
			if (Common::abs(air_temp_c, m_lastExternalTemp) > 1)
			{
				m_lastExternalTemp = air_temp_c;
			}
			else
			{
				air_temp_c = m_lastExternalTemp;
			}
		
			internal_temp_limit = g_properties.Chart.GetLimit(air_temp_c);	
			return true;
		}
		return false;
	}
	
	// Если датчик окружающего воздуха был инициализирован то возвращает температуру окружающего воздуха в градусах.
	bool TryGetLastAirTemperature(uint8_t& last_external_temp)
	{
		if (g_tempSensorTask.ExternalSensorInitialized)
		{
			last_external_temp = m_lastExternalTemp;
			return true;
		}
		return false;
	}
	
private:

	// Температура окружающего воздуха в градусах.
	volatile uint8_t m_lastExternalTemp;
};

extern HeaterTempLimit g_heaterTempLimit;
