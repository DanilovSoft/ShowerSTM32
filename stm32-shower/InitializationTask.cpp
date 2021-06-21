#include "InitializationTask.h"
#include "I2CHelper.h"
#include "EepromHelper.h"
#include "WaterLevelTask.h"
#include "HeatingTimeLeft.h"
#include "WiFiTask.h"
#include "LcdTask.h"
#include "LedLightTask.h"
#include "ButtonsTask.h"
#include "WaterLevelAnimationTask.h"

void InitializationTask::Run()
{
    g_i2cHelper.InitI2C();   // Инициализируем шину I2C.
    
    g_eepromHelper.InitProperties();   // Использует шину I2C.
    
    // Инициализируем структуру актуальными значениями.
    g_heatingTimeLeft = new HeatingTimeLeft(g_properties.WaterTankVolumeLitre, g_properties.WaterHeaterPowerKWatt);
    
    Common::InitPeripheral();
    
    g_waterLevelTask = new WaterLevelTask(&g_properties);
    g_tempSensorTask = new TempSensorTask(&g_properties);
    
    g_waterLevelTask->StartTask("WaterLevel");
    g_tempSensorTask->StartTask("TempSensor");
    
    // Разрешаем другим потокам доступ к Property.
    m_propertyInitialized = true;
}