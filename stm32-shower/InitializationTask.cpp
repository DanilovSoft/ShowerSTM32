#include "InitializationTask.h"
#include "I2CHelper.h"
#include "EepromHelper.h"
#include "WaterLevelTask.h"
#include "HeatingTimeLeft.h"
#include "WiFiTask.h"
#include "LcdTask.h"
#include "LedLightTask.h"
#include "ButtonsTask.h"

void InitializationTask::InitAllTasks()
{
    g_waterLevelTask.Init();
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
        
    // Initialise the xLastWakeTime variable with the current time.
    m_xLastWakeTime = xTaskGetTickCount();
    
    while (!g_waterLevelTask.GetIsInitialized())
    {
        Pause();
            
        switch (m_lastAnimPosition)
        {
        case 0:
            {
                m_lastAnimStep = kAnimStep2;    // '|'
                m_lastAnimPosition = 1;
                break;   
            }
        case 1:
            {
                m_lastAnimStep = kAnimStep3;    // '/'
                m_lastAnimPosition = 2;
                break;   
            }
        case 2:
            {
                m_lastAnimStep = kAnimStep4;    // '–'
                m_lastAnimPosition = 3;
                break;   
            }
        case 3:
            {
                m_lastAnimStep = kAnimStep1;    // '\'
                m_lastAnimPosition = 0;
                break;   
            }
        }
    }
}