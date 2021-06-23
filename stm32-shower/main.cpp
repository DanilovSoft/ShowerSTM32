#include "WiFiTask.h"
#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "LedLightTask.h"
#include "FreeRTOS.h"
#include "task.h"
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
#include "WaterLevelAnimationTask.h"
#include "PropertyWrapper.h"

PropertyWrapper g_properties;

// Эта структура накапливает изменения и затем сохраняет их в EEPROM и уже после
// перезугрузки устройства, изменения попадут в структуру g_properties.
PropertyStruct g_writeProperties;

// Принимает команды по WiFi и выполняет их.
WiFiTask g_wifiTask;

// Отображает информацию на LCD дисплее.
LcdTask* g_lcdTask;

// Снимает показания датчика расстояния.
WaterLevelTask g_waterLevelTask;

// Снимает показания с температурных датчиков.
TempSensorTask* g_tempSensorTask;

// Включает и выключает ТЭН.
HeaterTask* g_heaterTask;

// Включает или выключает свет в соответствии с положением автомата.
LedLightTask* g_ledLightTask;

// Бесконечно опрашивает кнопки и сенсорную панель.
ButtonsTask* g_buttonsTask;

// По запросу набирает воду в бак.
ValveTask* g_valveTask;

// Просто сбрасывает сторожевой таймер каждую секунду.
WatchDogTask* const g_watchDogTask = new WatchDogTask();

// Крутит анимацию символа '%' когда датчик уровня не может получить данные.
WaterLevelAnimationTask* g_wlAnimationTask;

Buzzer* const g_buzzer = new Buzzer();

EepromHelper* g_eepromHelper;

HeaterTempLimit* g_heaterTempLimit;

HeatingTimeLeft* g_heatingTimeLeft;

UartStream* const g_uartStream = new UartStream();

void PreInitPeripheral()
{
#pragma region RCC
    
     // Включить прерывания.
    __enable_irq();
    
    // Включаем тактирование портов.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);		// Часы.
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    RCC_LSICmd(ENABLE);
    
    // Выключаем JTAG (он занимает нужные нам ноги).
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // Делитель АЦП.
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);   // 72 / 6 = 12 мгц. Не должно превышать 14 мгц.
    
    // Наличие 230в для нагревателя.
    GPIO_InitTypeDef gpio_init =
    {
        GPIO_Pin_MainPower,
        GPIO_Speed_2MHz,
        GPIO_Mode_IPU
    };
    GPIO_Init(GPIO_MainPower, &gpio_init);
    
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
    
    Common::InitWaterLevelPeripheral();
    WaterLevelTask::ClearDisplay();
    Common::InitBeeperPeripheral();
    Common::InitUartPeripheral(g_uartStream->GetUartBufferAddress());
}

// Поток который инициализарует остальные потоки.
static void InitialThread(void* parm)
{
    I2CHelper i2c_helper;
    i2c_helper.InitI2C();   // Инициализируем шину I2C.
    
    g_eepromHelper = new EepromHelper(&i2c_helper);
    
    
    // Параметры прочитанные из EEPROM.
    g_properties = g_eepromHelper->DeserializeProperties();     // Использует шину I2C.
    g_properties.Initialized = true;
    
    // Инициализируем структуру актуальными значениями.
    //g_heatingTimeLeft = new HeatingTimeLeft(properties.WaterTankVolumeLitre, properties.WaterHeaterPowerKWatt);
    
    // Инициализируем периферию потоков.
    //Common::InitPeripheral(&properties);
    
    g_waterLevelTask.Init();
    g_wifiTask.Init();
//    g_tempSensorTask = new TempSensorTask(&properties);
//    g_heaterTask = new HeaterTask(&properties);
//    g_wlAnimationTask = new WaterLevelAnimationTask();
//    g_lcdTask = new LcdTask(&properties, &i2c_helper);
//    g_ledLightTask = new LedLightTask();
//    g_buttonsTask = new ButtonsTask(&properties);
//    g_valveTask = new ValveTask(&properties);
//    
    g_waterLevelTask.StartTask("WaterLevel");
    g_tempSensorTask->StartTask("TempSensor");
    g_heaterTask->StartTask("Heater"); 
    g_wlAnimationTask->StartTask("WaterLevelAnim");         
    g_wifiTask.StartTask("WiFi");                          
    g_lcdTask->StartTask("LCD");                            
    g_ledLightTask->StartTask("LedLight");                  
    g_buttonsTask->StartTask("Buttons");                    
    g_valveTask->StartTask("Valve");                        

}

// Инициализирует i2c и записывает параметры из EEPROM в Property и завершается.
InitializationTask initialization_task(InitialThread);

int main(void)
{
    PreInitPeripheral();
    
    initialization_task.StartTask("Initial");
    
    g_watchDogTask->Init(); // Запускает сторожевой таймер.
    g_watchDogTask->StartTask("WatchDog");  
    
    vTaskStartScheduler();
}

#if DEBUG

extern "C"
{
    void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
    {
        __asm("bkpt 255");
    }
}

#endif