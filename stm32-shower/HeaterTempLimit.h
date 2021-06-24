#pragma once
#include "stdint.h"
#include "Properties.h"
#include "math.h"
#include "Common.h"
#include "TempSensorTask.h"

class HeaterTempLimit final
{
public:

    HeaterTempLimit(const PropertyStruct* properties)
        : m_properties(properties)
    {
        
    }
    
	// Если датчик окружающего воздуха был инициализирован то возвращает желаемую температуру воды в баке.
	bool TryGetTargetTemperature(uint8_t& target_temp)
	{
    	float air_temp;
    	if (g_tempSensorTask.TryGetAirTemp(air_temp))
		{	
			uint8_t int_air_temp = round(air_temp);
		
			if (Common::abs(int_air_temp, m_airTemp) > 1)
			{
				m_airTemp = int_air_temp;
			}
			else
			{
				int_air_temp = m_airTemp;
			}
		
    		target_temp = m_properties->Chart.GetLimit(int_air_temp);	
			return true;
		}
		return false;
	}
	
	// Если датчик окружающего воздуха был инициализирован то возвращает температуру окружающего воздуха в градусах.
	bool TryGetAirTemperature(uint8_t& air_temp)
	{
		if (g_tempSensorTask.GetAirSensorInitialized())
		{
			air_temp = m_airTemp;
			return true;
		}
		return false;
	}
	
private:

    const PropertyStruct* m_properties;
    
	// Температура окружающего воздуха в градусах.
	volatile uint8_t m_airTemp;
};

extern HeaterTempLimit* g_heaterTempLimit;
