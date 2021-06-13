#include "WiFiTask.h"
#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "LedLightTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "RTOSwrapper.h"
#include "stm32f10x_bkp.h"
#include "Eeprom.h"
#include "ButtonsTask.h"
#include "LcdTask.h"
#include "HeatingTimeLeft.h"
#include "WatchDogTask.h"
#include "ValveTask.h"
#include "WaterLevelAnimTask.h"
#include "EepromTask.h"
#include "Buzzer.h"
#include "I2C.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include "TempSensor.h"
#include "TaskHelper.h"
#include "MedianFilter.h"

// Эта структура существует только для чтения, потому что с ней 
// работают несколько потоков, а запись не волатильна и ничем не синхронизирована.
PropertyStruct g_properties;

// Эта структура накапливает изменения и затем сохраняет их в EEPROM и уже после
// перезугрузки устройства, изменения попадут в структуру Properties.
PropertyStruct g_writeProperties;

RTOSwrapperClass g_rtosHelper;
TaskHelper g_taskHelper;
ButtonsTask g_buttonsTask;
EepromTask g_eepromTask;
LcdTask g_lcdTask;
LedLightTask g_ledLightTask;
WatchDogTask g_watchDogTask;
WaterLevelAnimTask g_waterLevelAnimTask;
WaterLevelTask g_waterLevelTask;
HeaterTask g_heaterTask;
WiFiTask g_wifiTask;
ValveTask g_valveTask;
TempSensorTask g_tempSensorTask;
Buzzer g_buzzer;
Eeprom g_eeprom;
HeaterTempLimit g_heaterTempLimit;
HeatingTimeLeft g_heatingTimeLeft;
UartStream g_uartStream;

void Init()
{
#pragma region RCC
    
     // Включить прерывания.
    __enable_irq();
    
    // Включаем тактирование портов.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);		// часы
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    RCC_LSICmd(ENABLE);
    
    // Выключаем JTAG (он занимает нужные нам ноги).
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // Делитель АЦП.
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   // 72 / 6 = 12 мгц. Не должно превышать 14 мгц.
    
    // Наличие 230в для нагревателя.
    GPIO_InitTypeDef gpioInit =
    {
        GPIO_Pin_MainPower,
        GPIO_Speed_2MHz,
        GPIO_Mode_IPU
    };
    GPIO_Init(GPIO_MainPower, &gpioInit);
    
#pragma endregion	
    
#pragma region Часы реального времени RTC
                  
    PWR_BackupAccessCmd(ENABLE);	// Разрешить доступ к Backup.
    BKP_DeInit();					// Сбросить Backup область.
    
    // STM32L– Система тактирования
    // http://we.easyelectronics.ru/STM32/stm32l-sistema-taktirovaniya-obzor.html
    
    // Ждём запуск LSI.
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {} // LSI RC – внутренний низкочастотный RC-генератор (40 кГц).
    
    // Включаем LSE (LSE crystal – внешний низкочастотный кварцевый генератор с частотой 32.768 KHz).
    RCC_LSEConfig(RCC_LSE_ON);
    
    // Ждём запуск LSE.
//    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) 
//    {
//	    
//    } 
    
    // Выбрать LSE источник для часов.
    //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    
    // Выбрать LSI источник для часов (LSE не всегда запускается).
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

    // Устанавливаем делитель, что-бы часы считали секунды.
    // Частота LSI составляет 40кгц.
    RTC_SetPrescaler(40000);  // RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1)
    
    // Включаем RTC часы.
    RCC_RTCCLKCmd(ENABLE);

    // Ждём запуск часов.
    RTC_WaitForSynchro();

    // Wait until last write operation on RTC registers has finished.
    RTC_WaitForLastTask();
    RTC_SetCounter(0);
    RTC_WaitForLastTask();
    
    PWR_BackupAccessCmd(DISABLE);
    
    #pragma endregion
    
    g_waterLevelTask.InitGpioAndClearDisplay();
    g_buzzer.Init();
    g_uartStream.Init();
}

static void EEPROM_Task(void* parm)
{
    g_i2c.vTaskInit(); // Инициализируем шину I2C.

    // Нельзя запускать другие потоки не считав параметры из EEPROM.
    g_eeprom.InitBeforeRTOS();  // Использует шину I2C.
    
    g_heatingTimeLeft.Init(g_properties.WaterTankVolumeLitre, g_properties.WaterHeaterPowerKWatt);
    
    g_rtosHelper.CreateTask(&g_wifiTask, "WI-FI", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_lcdTask, "LCD", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_waterLevelTask, "WaterLevel", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_waterLevelAnimTask, "WaterLevelAnim", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_tempSensorTask, "TempSensor", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_heaterTask, "Heater", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_ledLightTask, "LedLight", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_buttonsTask, "Buttons", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_valveTask, "Valve", tskIDLE_PRIORITY);
    g_rtosHelper.CreateTask(&g_watchDogTask, "WatchDog", tskIDLE_PRIORITY);
    
#if INCLUDE_vTaskDelete
    vTaskDelete(g_eepromTask.taskHandle);
#endif
}

int main(void)
{
    Init();
    
    g_eepromTask.taskHandle = xTaskCreateStatic(
        (TaskFunction_t)EEPROM_Task, 
        "eeprom", 
        iActiveTask::ulStackDepth,
        0,
        tskIDLE_PRIORITY,
        g_eepromTask.xStack,
        &g_eepromTask.xTaskBuffer);
    
    vTaskStartScheduler();
}