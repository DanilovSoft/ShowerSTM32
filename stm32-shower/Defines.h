#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"

#define SKIP_EEPROM // Для отладки, разрешает пропустить работу с EEPROM

#define GPIO_MainPower				(GPIOA)
#define GPIO_Pin_MainPower			(GPIO_Pin_1)
#define WIFI_GPIO					(GPIOB)
#define WIFI_GPIO_CH_PD_Pin			(GPIO_Pin_1)
#define WIFI_USART					(USART3)
#define GPIO_WIFI_USART				(GPIOB)
#define GPIO_WIFI_Pin_Rx			(GPIO_Pin_11)
#define GPIO_WIFI_Pin_Tx			(GPIO_Pin_10)
#define WIFI_DMA_CH_RX				(DMA1_Channel3)
#define WIFI_DMA_CH_TX				(DMA1_Channel2)
#define WIFI_DMA_FLAG				(DMA1_FLAG_TC2)  // Transmit Complete.
#define GPIO_WPS					(GPIOB)
#define GPIO_WPS_Pin				(GPIO_Pin_12)
#define GPIO_Heater					(GPIOA)
#define GPIO_Pin_Heater				(GPIO_Pin_10)
#define GPIO_Heater_Led				(GPIOC)
#define GPIO_Heater_Led_Pin			(GPIO_Pin_13)
#define Valve_GPIO                  (GPIOA)
#define Valve_Pin                   (GPIO_Pin_11)
#define SensorSwitch_Power_GPIO     (GPIOA)
#define SensorSwitch_Power_Pin      (GPIO_Pin_6)
#define Buzzer_TIM					(TIM2)
#define GPIO_Buzzer					(GPIOA)
#define GPIO_Buzzer_Pin				(GPIO_Pin_0)
#define WL_SPI                      (SPI2)
#define WL_TIM                      (TIM1)
#define WL_GPIO_Trig                (GPIOA)
#define WL_GPIO_Trig_Pin            (GPIO_Pin_9)
#define WL_GPIO_LATCH               (GPIOB)
#define WL_GPIO_LATCH_Pin           (GPIO_Pin_14)
#define WL_GPIO_SPI                 (GPIOB)
#define WL_GPIO_SPI_SCK_Pin         (GPIO_Pin_13)
#define WL_GPIO_SPI_MOSI_Pin        (GPIO_Pin_15)
#define WL_GPIO_TIM                 (GPIOA)
#define WL_GPIO_TIM_Pin             (GPIO_Pin_8)
#define I2C_EE_LCD					(I2C1)
#define GPIO_I2C_SCL_Pin			(GPIO_Pin_8)
#define GPIO_I2C_SDA_Pin			(GPIO_Pin_9)
#define OneWire_USART				(USART1)
#define OneWire_GPIO				(GPIOB)
#define OW_GPIO_Pin_Tx				(GPIO_Pin_6)
#define OW_DMA_CH_RX				(DMA1_Channel5)
#define OW_DMA_CH_TX				(DMA1_Channel4)
#define OW_DMA_FLAG					(DMA1_FLAG_TC5)
#define WIFI_GPIO					(GPIOB)
#define WIFI_GPIO_CH_PD_Pin			(GPIO_Pin_1) // Chip power-down.
#define GPIO_WPS					(GPIOB)
#define GPIO_WPS_Pin				(GPIO_Pin_12)
#define Button_GPIO                 (GPIOA)
#define Button_Temp_Minus           (GPIO_Pin_2)
#define Button_Temp_Plus            (GPIO_Pin_3)
#define Button_Water_Pin            (GPIO_Pin_4)
#define Button_SensorSwitch_OUT     (GPIO_Pin_7)
#define GPIO_LED					(GPIOB)
#define GPIO_Pin_LED				(GPIO_Pin_7)
#define LED_TIM						(TIM4)

#define WL_SUCCESS                  (0b10000000)
#define WL_RISING_EDGE              (0b01000000)
#define WL_OVERFLOW                 (0b00100000)

#define BIT_IS_SET(var,pos) ((var) & (1<<(pos)))
#define BIT_IS_NOT_SET(var,pos) (!BIT_IS_SET(var,pos))

// Делегат нажатия на кнопку.
typedef bool(*ButtonPressedFunc)();

static constexpr auto kDefaultWaterTankVolumeLitre = 37.32212;    
static constexpr auto kDefaultWiFiPower = 60;                        // 60 = 15.0 dBm
static constexpr auto kDefaultWaterHeaterPowerKWatt = 1.247616;      // Полтора-киловатный ТЭН с учётом КПД.
static constexpr auto kDefaultMinimumWaterHeatingPercent = 25;

static constexpr auto kAnimSpeedMsec = 300;                      // Скорость анимации символа '%' на LCD.
static constexpr uint8_t kWiFiTryInitLimit = 3;
static constexpr auto kUartRxFifoSize = 1024;                    // Размер кольцевого буфера UART.
static constexpr auto kUartMaxStrLen = 200;                      // Максимальная длина строки получаемая через UART.
static constexpr auto kWiFiUartSpeed = 115200;                   // Скорость на которую настроен ESP8266 модуль.
static constexpr uint8_t kWaterLevelAvgFilterMaxSize = 129;      // Максимально допустимый размер фильтра 'скользящее среднее' для уровня воды.
static constexpr uint8_t kInternalTempLimit = 42;                // Максимальная температура воды в баке.
static constexpr uint8_t kWaterLevelMedianMaxSize = 255;         // Максимально допустимый размер медианного фильтра для уровня воды.
static constexpr uint8_t kAirTempLowerBound = 15;                // Минимальная температура на улице.
static constexpr uint8_t kAirTempUpperBound = 40;                // Максимальная температура на улице.
static constexpr uint8_t kInternalTempAvgFilterSize = 8;         // Максимально допустимый размер фильтра 'скользящее среднее' для температуры в баке.
static constexpr uint8_t kAirTempAvgFilterSize = 1;              // Максимально допустимый размер фильтра 'скользящее среднее' для температуры окружающего воздуха.
static constexpr auto kAirTempSteps = kAirTempUpperBound - kAirTempLowerBound;    // Размер таблицы температур делаем исходя из возможных значений температур окружаюшего воздуха.
static constexpr uint16_t kTempSensorPauseMsec = 2000;           // Пауза между измерениями температуры.

// Всего 8 блоков памяти по 256 байт (по 16 страниц).
// Каждый блок имеет свой уникальный адрес как отдельное устройство,
// в зависимости от битов b3, b2, b1
//
// Используем только 1 блок памяти.
// Используем первый байт, первой страницы для хранения флага 0 или 1
// указывающего по какому адресу находятся данные.
// На хранение 2 блоков данных приходится по 7 страниц или 112 байт

static constexpr auto EE_HW_ADDRESS =          0xA0;   // Адрес первого блока памяти на 256 байт из 8 (для 24—16).  // b3 = b2 = b1 = 0
static constexpr auto LCD_HW_ADDRESS =         0x7E;
static constexpr auto EE_FLASH_PAGESIZE =      16;        // 16-byte Page.
static constexpr auto EE_BlockSize =           256;
static constexpr auto EE_DataAddr1 =           0x0000 + 16;   // Адрес блока в EEPROM где храняться параметры PropertyStruct.
static constexpr auto EE_DataAddr2 =           0x0000 + 128;    // Адрес блока в EEPROM где храняться параметры PropertyStruct.
static constexpr auto EE_AvailableDataSize = EE_BlockSize / 2 - EE_FLASH_PAGESIZE;     // 112 байт.

static_assert(kAirTempUpperBound > kAirTempLowerBound, "kAirTempLowerBound should be lower than kAirTempUpperBound");
