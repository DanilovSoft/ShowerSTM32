#pragma once
#include "stm32f10x_gpio.h"
#include "string.h"

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
#define WIFI_DMA_FLAG				(DMA1_FLAG_TC2)  // Transmit Complete
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

#define BIT_IS_SET(var,pos) ((var) & (1<<(pos)))
#define BIT_IS_NOT_SET(var,pos) (!BIT_IS_SET(var,pos))

static constexpr auto kUartRxFifoSize = 1024;
static constexpr auto kUartMaxStrLen = 200;                     // Максимальная длина строки получаемая через UART.
static constexpr auto kWiFiUartSpeed = 115200;                  // Скорость на которую настроен ESP8266 модуль.
static constexpr uint8_t kWaterLevelAvgFilterMaxSize = 129;     // Максимально допустимый размер фильтра 'скользящее среднее' для уровня воды.
static constexpr uint8_t kInternalTempLimit = 42;               // Максимальная температура воды в баке.
static constexpr auto kWaterLevelMedianMaxSize = 255;           // Максимально допустимый размер медианного фильтра для уровня воды.
static constexpr uint8_t kAirTempLowerBound = 15;               // Минимальная температура на улице.
static constexpr uint8_t kAirTempUpperBound = 40;               // Максимальная температура на улице.
static constexpr uint8_t kInternalTempAvgFilterSize = 16;       // Максимально допустимый размер фильтра 'скользящее среднее' для температуры в баке.
static constexpr uint8_t kAirTempAvgFilterSize = 1;         // Максимально допустимый размер фильтра 'скользящее среднее' для температуры окружающего воздуха.
static constexpr auto kAirTempSteps = kAirTempUpperBound - kAirTempLowerBound;   // Размер таблицы температур делаем исходя из возможных значений температур окружаюшего воздуха.

static_assert(kAirTempUpperBound > kAirTempLowerBound, "kAirTempLowerBound should be lower than kAirTempUpperBound");

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

class Common
{
public:

    // Включен ли автомат нагревателя.
    inline static bool CircuitBreakerIsOn()
    {
        return GPIO_ReadInputDataBit(GPIO_MainPower, GPIO_Pin_MainPower) == RESET;
    }

    // Включен ли нагреватель (реле).
    inline static bool HeaterIsOn()
    {
        return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
    }

    // Открыт ли водяной клапан.
    inline static bool ValveIsOpen()
    {
        return GPIO_ReadInputDataBit(Valve_GPIO, Valve_Pin);
    }
    
    inline static bool GetIsHeaterEnabled()
    {
        return GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET;
    }
    
    inline static void DelayUs(uint16_t usec)
    {
        _delay_loops((unsigned int)((double)usec * (SystemCoreClock / 3000000.0)));
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

    static char itoa(uint8_t value)
    {
        const char digits[] = "0123456789";

        char ch = '0';
        if (value <= 9)
        {
            ch = digits[value];
        }
        return ch;
    }

    static char* itoa(uint16_t number, char* str)
    {
        const char digit[] = "0123456789";
        char* p = str;
        //	if (i < 0)
        //	{
        //		*p++ = '-';
        //		i *= -1;
        //	}
            int32_t shifter = number;
        do
        {
            //Move to where representation ends
       ++p;
            shifter = shifter / 10;
        } while (shifter);
        *p = '\0';
        do
        {
            //Move back, inserting digits as u go
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

    static unsigned char ctoi(const char ch)
    {
        return ch - 48;
    }

    static bool ArrayEquals(uint8_t* array1, uint8_t arr1Size, uint8_t* array2, uint8_t arr2Size)
    {
        if (arr1Size != arr2Size) 
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
};

// http://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
// If you want full control among compiler optimizations, the best 1µs delay can be reached using this macro fully written in assembler.
// Doing tests with the scope, I found that 1µs delay can be obtained when the MCU execute this loop 16 times at 84MHZ. 
// However, this macro has to be rearranged if you processor speed is lower, and keep in mind 
// that being a macro, it is "expanded" every time you use it, causing the increase of firmware size.
// These are the best solution to obtain 1µs delay with the STM32 platform.
//
// (10*us) // Установлено экспериментальным путем с помощью осцилографа где SystemCoreClock = 72мгц
// (8*us) // Может быть более точным значением но меандр выглядит не пропорциональным

//#define Delay_us(us) do {\
//	asm volatile (	"MOV R0,%[loops]\n\t"\
//			"1: \n\t"\
//			"SUB R0, #1\n\t"\
//			"CMP R0, #0\n\t"\
//			"BNE 1b \n\t" : : [loops] "r" (10*us) : "memory"\
//		      );\
//} while(0)