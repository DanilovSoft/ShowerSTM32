#pragma once
#include "stm32f10x_gpio.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"

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
#define WIFI_GPIO_CH_PD_Pin			(GPIO_Pin_1)
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

typedef bool(*ButtonPressedFunc)();

static constexpr uint8_t kWiFiTryInitLimit = 3;
static constexpr auto kUartRxFifoSize = 1024;                   // Размер кольцевого буфера UART.
static constexpr auto kUartMaxStrLen = 200;                     // Максимальная длина строки получаемая через UART.
static constexpr auto kWiFiUartSpeed = 115200;                  // Скорость на которую настроен ESP8266 модуль.
static constexpr uint8_t kWaterLevelAvgFilterMaxSize = 129;     // Максимально допустимый размер фильтра 'скользящее среднее' для уровня воды.
static constexpr uint8_t kInternalTempLimit = 42;               // Максимальная температура воды в баке.
static constexpr uint8_t kWaterLevelMedianMaxSize = 255;        // Максимально допустимый размер медианного фильтра для уровня воды.
static constexpr uint8_t kAirTempLowerBound = 15;               // Минимальная температура на улице.
static constexpr uint8_t kAirTempUpperBound = 40;               // Максимальная температура на улице.
static constexpr uint8_t kInternalTempAvgFilterSize = 16;       // Максимально допустимый размер фильтра 'скользящее среднее' для температуры в баке.
static constexpr uint8_t kAirTempAvgFilterSize = 1;             // Максимально допустимый размер фильтра 'скользящее среднее' для температуры окружающего воздуха.
static constexpr auto kAirTempSteps = kAirTempUpperBound - kAirTempLowerBound;   // Размер таблицы температур делаем исходя из возможных значений температур окружаюшего воздуха.
static constexpr auto kTankMinimumHeightCm = 30;                // Минимально возможная высота бака, см.
static constexpr auto kTankMaximumHeightCm = 50;                // Максимально возможная высота бака, см.
static constexpr auto kDefaultFullTankDistanceCm = 45.1379;               
static constexpr auto kDefaultEmptyTankDistanceCm = 15.8965;    
static constexpr uint16_t kTempSensorPauseMsec = 2000;          // Пауза между измерениями температуры.

static_assert(kAirTempUpperBound > kAirTempLowerBound, "kAirTempLowerBound should be lower than kAirTempUpperBound");


// http://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
inline void _delay_loops(unsigned int loops)
{
     asm volatile (
             "1: \n"
             " SUBS %[loops], %[loops], #1 \n"
             " BNE 1b \n"
             : [loops] "+r"(loops)
     );
}

#define DELAY_US( US ) _delay_loops( (unsigned int)((double)US * (SystemCoreClock / 3000000.0)) )

class Common final
{
public:

    // Включен ли автомат нагревателя.
    static bool CircuitBreakerIsOn()
    {
        return GPIO_ReadInputDataBit(GPIO_MainPower, GPIO_Pin_MainPower) == RESET;
    }

    // Включен ли ТЭН (реле).
    static bool HeaterIsOn()
    {
        return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
    }

    // Открыт ли водяной клапан.
    static bool ValveIsOpen()
    {
        return GPIO_ReadInputDataBit(Valve_GPIO, Valve_Pin);
    }
    
    static bool ButtonTempPlussPressed()
    {
        return GPIO_ReadInputDataBit(Button_GPIO, Button_Temp_Plus);
    }
    
    static bool ButtonTempMinusPressed()
    {
        return GPIO_ReadInputDataBit(Button_GPIO, Button_Temp_Minus);
    }
    
    static bool ButtonValvePressed()
    {
        return GPIO_ReadInputDataBit(Button_GPIO, Button_Water_Pin);
    }
    
    static bool ButtonSensorSwitchIsOn()
    {
        return GPIO_ReadInputDataBit(Button_GPIO, Button_SensorSwitch_OUT);
    }
    
    // Включает питание сенсорной кнопки.
    static void TurnOnSensorSwitch()
    {
        GPIO_SetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
    }
    
    // Выключает питание сенсорной кнопки.
    static void TurnOffSensorSwitch()
    {
        // Выключить сенсор.
        GPIO_ResetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
    }
    
    // Открывает водяной клапан.
    static void OpenValve()
    {
        GPIO_SetBits(Valve_GPIO, Valve_Pin);
    }
    
    // Закрывает водяной клапан.
    static void CloseValve()
    {
        GPIO_ResetBits(Valve_GPIO, Valve_Pin);
    }
    
    // Отключает светодиод освещения.
    static void TurnOffLight()
    {
        // Отключает GPIO от таймера.
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_Pin_LED,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_IN_FLOATING
        };
        GPIO_Init(GPIO_LED, &gpio_init_struct);
    }
    
    // Включает светодиод освещения.
    static void TurnOnLight()
    {
        // Подключает GPIO к таймеру.
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_Pin_LED,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_AF_PP
        };
        GPIO_Init(GPIO_LED, &gpio_init_struct);
    }
    
    // Включает питание ТЭНа и зажигает светодиод.
    static void TurnOnHeater()
    {
        GPIO_SetBits(GPIO_Heater, GPIO_Pin_Heater);
        GPIO_ResetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
    }
    
    // Выключает питание ТЭНа и тушит светодиод.
    static void TurnOffHeater()
    {
        GPIO_ResetBits(GPIO_Heater, GPIO_Pin_Heater);
        GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
    }
    
    static uint8_t DigitsCount(uint16_t value)
    {
        uint8_t count = 0;

        do
        {
            value /= 10;
            count++;
        } while (value);

        return count;
    }

    // Преобразует число в символьное представление.
    static char DigitToChar(uint8_t value)
    {
        const char digits[] = "0123456789";

        char ch = '0';
        if (value <= 9)
        {
            ch = digits[value];
        }
        return ch;
    }

    static char* NumberToString(uint16_t number, char* str)
    {
        const char digit[] = "0123456789";
        char* p = str;
        
        int32_t shifter = number;
        do
        {
            // Move to where representation ends.
            ++p;
            shifter = shifter / 10;
        } while (shifter);
        
        *p = '\0';
        
        do
        {
            // Move back, inserting digits as u go.
            * --p = digit[number % 10];
            number = number / 10;
        } while (number);
        
        return str;
    }

    static bool streql(const char* str1, const char* str2)
    {
        return strcmp(str1, str2) == 0;
    }

    // Возвращает абсолютную разницу между значениями.
    static uint16_t abs(uint8_t a, uint8_t b)
    {
        return a > b ? a - b : b - a;
    }

    // Возвращает абсолютную разницу между значениями.
    static uint16_t abs(uint16_t a, uint16_t b)
    {
        return a > b ? a - b : b - a;
    }

    // Возвращает абсолютную разницу между значениями.
    static uint32_t abs(uint32_t a, uint32_t b)
    {
        return a > b ? a - b : b - a;
    }

    static unsigned char CharToDigit(const char ch)
    {
        return ch - 48;
    }

    static bool ArrayEquals(uint8_t* array1, uint8_t arr1Size, uint8_t* array2, uint8_t arr2_size)
    {
        if (arr1Size != arr2_size) 
        {
            return false;
        }
    
        for (int i = 0; i < arr1Size; ++i) 
        {
            if (array1[i] != array2[i])
            {
                return false;
            }
        }
        return true;
    }

    // Passed arrays store different data types.
    template <typename T, typename U, int size1, int size2> static bool Equal(T(&arr1)[size1], U(&arr2)[size2])
    {
        return false;
    }

    // Passed arrays store SAME data types.
    template <typename T, int size1, int size2> static bool Equal(T(&arr1)[size1], T(&arr2)[size2]) 
    {
        if (size1 == size2) 
        {
            for (int i = 0; i < size1; ++i) 
            {
                if (arr1[i] != arr2[i]) return false;
            }
            return true;
        }
        return false;
    }

private:
    
};