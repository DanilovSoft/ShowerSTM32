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

void InitializationTask::InitAllTasks()
{
    // Все таски должны быть проинициализированы перед запуском, последовательно, а не параллельно.
    
    g_waterLevelTask.Init();
    g_wlAnimationTask.Init();
    g_wifiTask.Init();
    g_lcdTask.Init();
    g_tempSensorTask.Init();
    g_heaterTask.Init();
    g_ledLightTask.Init();
    g_buttonsTask.Init();
    g_valveTask.Init();
}

void InitializationTask::Run()
{
    g_i2cHelper.InitI2C();   // Инициализируем шину I2C.
    
    g_eepromHelper.InitProperties();   // Использует шину I2C.
    
    // Инициализируем структуру актуальными значениями.
    g_heatingTimeLeft = new HeatingTimeLeft(g_properties.WaterTankVolumeLitre, g_properties.WaterHeaterPowerKWatt);
    
    InitAllTasks();
    
    // Разрешаем другим потокам доступ к Property.
    m_propertyInitialized = true;
}