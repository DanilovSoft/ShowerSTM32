#pragma once

enum ShowerCode
{
	None = 0,
	SetWaterLevelFull = 1,
	SetWaterLevelEmpty = 2,
	GetWaterLevelFull = 3,
	GetWaterLevelEmpty = 4,
	GetTempChart = 5,
	SetTempChart = 6,
	Ping = 7,
	GetWaterLevel = 8,
	GetWaterPercent = 9,
	GetExternalTemp = 10,
    GetInternalTemp = 11,
	GetAverageInternalTemp = 12,
	GetAverageExternalTemp = 13,
	GetMinimumWaterHeatingLevel = 14,
	SetMinimumWaterHeatingLevel = 15,
	
	GetHeatingTimeLimit = 18,
	SetHeatingTimeLimit = 19,
	GetLightBrightness = 20,
	SetLightBrightness = 21,
	GetTimeLeft = 22,
	GetHeatingProgress = 23,
	GetWaterHeated = 24,
	GetHeatingTimeoutState = 25,
	GetHeaterEnabled = 26,
	GetAbsoluteHeatingTimeLimit = 27,
	SetAbsoluteHeatingTimeLimit = 28,
	GetWiFiPower = 29,
	SetWiFiPower = 30,
	GetAbsoluteTimeoutStatus = 31,
	GetWatchDogWasReset = 32,
	GetHasMainPower = 33,
	GetHeatingLimit = 34,
	SetCurAP = 35,
	SetDefAP = 36,
	
	GetWaterLevelMeasureInterval = 37,
	SetWaterLevelMeasureInterval = 38,
	
	GetWaterValveCutOffPercent = 39,
	SetWaterValveCutOffPercent = 40,
	
	GetWaterLevelAverageBufferSize = 45,
	SetWaterLevelAverageBufferSize = 46,
	
	GetTempSensorInternalTempAverageSize = 47,
	SetTempSensorInternalTempAverageSize = 48,
	
	GetWaterLevelUsecPerDeg = 49,
	SetWaterLevelUsecPerDeg = 50,
	
	GetWaterValveTimeoutSec = 51,
	SetWaterValveTimeoutSec = 52,
	
    GetWaterLevelRaw = 53,
    
    GetButtonTimeMsec = 54,
    SetButtonTimeMsec = 55,
    
    GetTempLowerBound = 56,
    GetTempUpperBound = 57,
    
	GetWaterLevelMedianBufferSize = 58,
	SetWaterLevelMedianBufferSize = 59,
	
	GetWaterTankVolumeLitre = 60,
	SetWaterTankVolumeLitre = 61,
	
	GetWaterHeaterPowerKWatt = 62,
	SetWaterHeaterPowerKWatt = 63,
	
	OK = 200,
	UnknownCode = 253,
	Reset = 254,
	Save = 255,
};
