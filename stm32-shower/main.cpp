#include "WiFiTask.h"
#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "LedLightTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "RtosHelper.h"
#include "stm32f10x_bkp.h"
#include "EepromHelper.h"
#include "ButtonsTask.h"
#include "LcdTask.h"
#include "HeatingTimeLeft.h"
#include "WatchDogTask.h"
#include "ValveTask.h"
#include "InitializationTask.h"
#include "Buzzer.h"
#include "I2CHelper.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include "TempSensorTask.h"
#include "MedianFilter.h"

// Эта структура существует только для чтения, потому что с ней 
// работают несколько потоков, а запись ничем не синхронизирована.
PropertyStruct g_properties;

// Эта структура накапливает изменения и затем сохраняет их в EEPROM и уже после
// перезугрузки устройства, изменения попадут в структуру g_properties.
PropertyStruct g_writeProperties;

InitializationTask g_initializationTask;
WiFiTask g_wifiTask;
LcdTask g_lcdTask;
WaterLevelTask g_waterLevelTask;
TempSensorTask g_tempSensorTask;
HeaterTask g_heaterTask;
LedLightTask g_ledLightTask;
ButtonsTask g_buttonsTask;
ValveTask g_valveTask;
WatchDogTask g_watchDogTask;
Buzzer g_buzzer;
EepromHelper g_eepromHelper;
HeaterTempLimit g_heaterTempLimit;
HeatingTimeLeft* g_heatingTimeLeft;
UartStream g_uartStream;
I2CHelper g_i2cHelper;

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
   
//    uint16_t lse_retry_count = 1000;
//    
//    // Ждём запуск LSE.
//    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET && lse_retry_count > 0)
//    {
//        lse_retry_count--;   
//    }
//    
//    if (lse_retry_count == 0)
//    {
//        // Выбрать LSI источник для часов (LSE не всегда запускается).
//        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
//    }
//    else
//    {
//        // Выбрать LSE источник для часов.
//        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
//    }

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

int main(void)
{
    Init();
    
    g_watchDogTask.Init();
    
    RtosHelper::CreateTask(&g_initializationTask, "Initialization");
    RtosHelper::CreateTask(&g_waterLevelTask, "WaterLevel");
    RtosHelper::CreateTask(&g_wifiTask, "WI-FI");
    RtosHelper::CreateTask(&g_lcdTask, "LCD");
    RtosHelper::CreateTask(&g_tempSensorTask, "TempSensor");
    RtosHelper::CreateTask(&g_heaterTask, "Heater");
    RtosHelper::CreateTask(&g_ledLightTask, "LedLight");
    RtosHelper::CreateTask(&g_buttonsTask, "Buttons");
    RtosHelper::CreateTask(&g_valveTask, "Valve");
    RtosHelper::CreateTask(&g_watchDogTask, "WatchDog");
    
    vTaskStartScheduler();
}

//extern "C"
//{
//    void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
//    {
//        __asm("bkpt 255");
//    }
//}