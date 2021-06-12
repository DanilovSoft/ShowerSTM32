#pragma once
#include "stm32f10x_gpio.h"

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
#define Button_Water                (GPIO_Pin_4)
#define Button_SensorSwitch_OUT     (GPIO_Pin_7)
#define GPIO_LED        (GPIOB)
#define GPIO_Pin_LED    (GPIO_Pin_7)
#define LED_TIM         (TIM4)


#define CircuitBreakerIsOn()		(GPIO_ReadInputDataBit(GPIO_MainPower, GPIO_Pin_MainPower) == RESET) // Включен ли автомат нагревателя.
#define HeaterIsOn()				(GPIO_ReadInputDataBit(GPIO_Heater, GPIO_Pin_Heater) == SET) // Включен ли нагреватель.
#define ValveOpened()               (GPIO_ReadInputDataBit(Valve_GPIO, Valve_Pin)) // Включен ли клапан воды.


static const auto UART_RX_FIFO_SZ = 1024;
static const auto UART_MAX_STR_LEN = 200;
static const auto WIFI_UART_Speed = 115200;

inline void _delay_loops(unsigned int loops)
{
	 asm volatile (
			 "1: \n"
			 " SUBS %[loops], %[loops], #1 \n"
			 " BNE 1b \n"
			 : [loops] "+r"(loops)
	 );
}

#define Delay_us( US ) _delay_loops( (unsigned int)((double)US * (SystemCoreClock / 3000000.0)) )

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

bool streql(const char* str1, const char* str2);
uint16_t abs(uint8_t a, uint8_t b);
uint16_t abs(uint16_t a, uint16_t b);
uint32_t abs(uint32_t a, uint32_t b);
uint8_t DigitsCount(uint16_t value);
char itoa(uint8_t value);
char* itoa(uint16_t number, char* str);
//uint16_t atoi(const char* str);
unsigned char ctoi(const char ch);
bool ArrayEquals(uint8_t* array1, uint8_t arr1Size, uint8_t* array2, uint8_t arr2Size);
template <typename T, typename U, int size1, int size2> bool Equal(T(&arr1)[size1], U(&arr2)[size2]);
template <typename T, int size1, int size2> bool Equal(T(&arr1)[size1], T(&arr2)[size2]);
