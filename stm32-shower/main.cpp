#include "WiFi.h"
#include "HeaterTask.h"
#include "WaterLevelTask.h"
#include "LedLightTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "RTOSwrapper.h"
#include "stm32f10x.h"
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
#include "Dwt.h"
#include "TaskHelper.h"
#include "MedianFilter.h"

#define uint unsigned int

void Init()
{

#pragma region RCC
	
	 /* включить прерывания */
    __enable_irq();
	
    /* Включаем тактирование портов */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);		// часы
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
    RCC_LSICmd(ENABLE);
	
    /* Выключаем JTAG (он занимает ноги нужные нам) */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
    /* Делитель АЦП */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);		// 72 / 6 = 12 мгц. Не должно превышать 14 мгц
	
    /* Наличие 220v для нагревателя */
    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_MainPower;	
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIO_MainPower, &gpio_init);
	
#pragma endregion	
	
#pragma region RTC
				  
    PWR_BackupAccessCmd(ENABLE);		//Разрешить доступ к Backup
    BKP_DeInit();						//Сбросить Backup область
	
    /* STM32L– Система тактирования
    http://we.easyelectronics.ru/STM32/stm32l-sistema-taktirovaniya-obzor.html
    */
    
    /* Wait until LSI is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {} // LSI RC – внутренний низкочастотный RC-генератор (40 кГц)
	
    /* Enable LSE */
    RCC_LSEConfig(RCC_LSE_ON);
    
    /* Wait until LSI is ready */
    //while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {} // LSE crystal – внешний низкочастотный кварцевый генератор
    
    // Выбрать LSI источник для часов
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    
    // Выбрать LSE источник для часов
    //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

    // Устанавливаем делитель, чтобы часы считали секунды
    // частота LSI составляет 40кгц
    RTC_SetPrescaler(40000);  /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
    
    //  Enable RTC Clock
    RCC_RTCCLKCmd(ENABLE);

    //  Wait for RTC registers synchronization
    RTC_WaitForSynchro();

    //  Wait until last write operation on RTC registers has finished *
    RTC_WaitForLastTask();
    RTC_SetCounter(0);
    RTC_WaitForLastTask();
    
    PWR_BackupAccessCmd(DISABLE);
	
#pragma endregion
	
    _waterLevelTask.InitGPIO_ClearDisplay();
    buzzer.Init();
    uartStream.Init();
}


static void EEPROM_Task(void* parm)
{
	i2c.vTaskInit(); // Инициализируем шину I2C.

	// Нельзя запускать другие потоки не считав переменные из EEPROM.
	_eeprom.InitBeforeRTOS();  // Использует шину I2C.
	
    RTOSwrapper.CreateTask(&_wifiTask, "WI-FI", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_lcdTask, "LCD", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_waterLevelTask, "WaterLevel", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_waterLevelAnimTask, "WaterLevelAnim", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_tempSensorTask, "TempSensor", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_heaterTask, "Heater", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_ledLightTask, "LedLight", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_buttonsTask, "Buttons", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_valveTask, "Valve", tskIDLE_PRIORITY);
    RTOSwrapper.CreateTask(&_watchDogTask, "WatchDog", tskIDLE_PRIORITY);
    
#if INCLUDE_vTaskDelete
	vTaskDelete(eepromTask.taskHandle);
#endif
}


int main(void)
{
	Init();
    
    eepromTask.taskHandle = xTaskCreateStatic(
        (TaskFunction_t)EEPROM_Task, 
        "eeprom", 
        iActiveTask::ulStackDepth,
        0,
        tskIDLE_PRIORITY,
        eepromTask.xStack,
        &eepromTask.xTaskBuffer);
    
    vTaskStartScheduler();
}