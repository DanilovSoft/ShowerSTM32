#include "HeaterTempLimit.h"
#include "TempSensor.h"
#include "Properties.h"
#include "math.h"
#include "Common.h"

HeaterTempLimit g_heaterTempLimit;

bool HeaterTempLimit::TryGetTargetTemperature(uint8_t& internalTempLimit)
{
	if (g_tempSensorTask.ExternalSensorInitialized)
	{
		float extTemp = g_tempSensorTask.AverageExternalTemp;
		uint8_t t = round(extTemp);
		
		if (abs(t, m_lastExternalTemp) > 1)
		{
			m_lastExternalTemp = t;
		}
		else
		{
			t = m_lastExternalTemp;
		}
		
		internalTempLimit = g_properties.Chart.GetLimit(t);	
		return true;
	}
	return false;
}
	
bool HeaterTempLimit::TryGetLastExternalTemp(uint8_t& lastExternalTemp)
{
	if (g_tempSensorTask.ExternalSensorInitialized)
	{
		lastExternalTemp = m_lastExternalTemp;
		return true;
	}
	return false;
}