#pragma once

enum ShowerCode
{
    kNone = 0,
    kSetWaterLevelFull = 1,
    kSetWaterLevelEmpty = 2,
    kGetWaterLevelFull = 3,
    kGetWaterLevelEmpty = 4,
    kGetTempChart = 5,
    kSetTempChart = 6,
    kPing = 7,
    kGetWaterLevel = 8,
    kGetWaterPercent = 9,
    kGetAirTemp = 10,
    kGetInternalTemp = 11,
    kGetAverageInternalTemp = 12,
    kGetAverageAirTemp = 13,
    kGetMinimumWaterHeatingLevel = 14,
    kSetMinimumWaterHeatingLevel = 15,
    
    kGetHeatingTimeLimit = 18,
    kSetHeatingTimeLimit = 19,
    kGetLightBrightness = 20,
    kSetLightBrightness = 21,
    kGetTimeLeft = 22,
    kGetHeatingProgress = 23,
    kGetWaterHeated = 24,
    kGetHeatingTimeoutState = 25,
    kGetHeaterEnabled = 26,
    kGetAbsoluteHeatingTimeLimit = 27,
    kSetAbsoluteHeatingTimeLimit = 28,
    kGetWiFiPower = 29,
    kSetWiFiPower = 30,
    kGetAbsoluteTimeoutStatus = 31,
    kGetWatchDogWasReset = 32,
    kGetHasMainPower = 33,
    kGetHeatingLimit = 34,
    kSetCurAP = 35,
    kSetDefAP = 36,
    
    kGetWaterLevelMeasureInterval = 37,
    kSetWaterLevelMeasureInterval = 38,
    
    kGetWaterValveCutOffPercent = 39,
    kSetWaterValveCutOffPercent = 40,
    
    kGetWaterLevelAverageBufferSize = 45,
    kSetWaterLevelAverageBufferSize = 46,
    
    kGetTempSensorInternalTempAverageSize = 47,
    kSetTempSensorInternalTempAverageSize = 48,
    
//    kGetWaterLevelUsecPerDeg = 49,
//    kSetWaterLevelUsecPerDeg = 50,
    
    //kGetWaterValveTimeoutSec = 51,
    //kSetWaterValveTimeoutSec = 52,
    
    kGetWaterLevelRaw = 53,
    
    kGetButtonTimeMsec = 54,
    kSetButtonTimeMsec = 55,
    
    kGetTempLowerBound = 56,
    kGetTempUpperBound = 57,
    
    kGetWaterLevelMedianBufferSize = 58,
    kSetWaterLevelMedianBufferSize = 59,
    
    kGetWaterTankVolumeLitre = 60,
    kSetWaterTankVolumeLitre = 61,
    
    kGetWaterHeaterPowerKWatt = 62,
    kSetWaterHeaterPowerKWatt = 63,
    
    kGetButtonLongPressTimeMsec = 70,
    kSetButtonLongPressTimeMsec = 71,
    
    kGetWaterLevelErrorThreshold = 80,
    kSetWaterLevelErrorThreshold = 81,
    
    kOK = 200,
    kUnknownCode = 253,
    kReset = 254,
    kSave = 255,
};
