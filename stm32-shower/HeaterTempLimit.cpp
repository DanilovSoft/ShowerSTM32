#include "HeaterTempLimit.h"
#include "TempSensor.h"
#include "Properties.h"
#include "math.h"
#include "Common.h"

HeaterTempLimit _heaterTempLimit;

bool HeaterTempLimit::TryGetTargetTemperature(uint8_t& internalTempLimit)
{
	if (_tempSensorTask.ExternalSensorInitialized)
	{
		float extTemp = _tempSensorTask.AverageExternalTemp;
		uint8_t t = round(extTemp);
		
		if (abs(t, _lastExternalTemp) > 1)
		{
			_lastExternalTemp = t;
		}
		else
		{
			t = _lastExternalTemp;
		}
		
		internalTempLimit = Properties.Chart.GetLimit(t);	
		return true;
	}
	return false;
}
	
bool HeaterTempLimit::TryGetLastExternalTemp(uint8_t& lastExternalTemp)
{
	if (_tempSensorTask.ExternalSensorInitialized)
	{
		lastExternalTemp = _lastExternalTemp;
		return true;
	}
	return false;
}