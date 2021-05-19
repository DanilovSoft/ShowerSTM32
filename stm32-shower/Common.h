#pragma once
#include "stdint.h"
#include "stm32f10x_gpio.h"

#define GPIO_MainPower			(GPIOA)
#define GPIO_Pin_MainPower		(GPIO_Pin_1)
#define RX_FIFO_SZ              (1024)
#define UART_MAX_STR_LEN        (200)
#define WIFI_GPIO				(GPIOB)
#define WIFI_GPIO_CH_PD_Pin		(GPIO_Pin_1)
#define WIFI_USART				(USART3)
#define GPIO_WIFI_USART		    (GPIOB)
#define GPIO_WIFI_Pin_Rx		(GPIO_Pin_11)
#define GPIO_WIFI_Pin_Tx		(GPIO_Pin_10)
#define WIFI_DMA_CH_RX		    (DMA1_Channel3)
#define WIFI_DMA_CH_TX		    (DMA1_Channel2)
#define WIFI_DMA_FLAG		    (DMA1_FLAG_TC2)  // Transmit Complete
#define GPIO_WPS				(GPIOB)
#define GPIO_WPS_Pin			(GPIO_Pin_12)
#define WIFI_UART_Speed         (115200)
//#define WIFI_UART_Speed         (57600)

//#pragma GCC push_options
//#pragma GCC optimize ("O3")
inline void _delay_loops(unsigned int loops)
{
	 asm volatile (
			 "1: \n"
			 " SUBS %[loops], %[loops], #1 \n"
			 " BNE 1b \n"
			 : [loops] "+r"(loops)
	 );
}
//#pragma GCC pop_options

/*
http://www.carminenoviello.com/2015/09/04/precisely-measure-microseconds-stm32/
If you want full control among compiler optimizations, the best 1µs delay can be reached using this macro fully written in assembler.
Doing tests with the scope, I found that 1µs delay can be obtained when the MCU execute this loop 16 times at 84MHZ. 
However, this macro has to be rearranged if you processor speed is lower, and keep in mind 
that being a macro, it is "expanded" every time you use it, causing the increase of firmware size.
These are the best solution to obtain 1µs delay with the STM32 platform.

(10*us) // Установлено экспериментальным путем с помощью осцилографа где SystemCoreClock = 72мгц
(8*us) // Может быть более точным значением но меандр выглядит не пропорциональным

*/
//#define Delay_us(us) do {\
//	asm volatile (	"MOV R0,%[loops]\n\t"\
//			"1: \n\t"\
//			"SUB R0, #1\n\t"\
//			"CMP R0, #0\n\t"\
//			"BNE 1b \n\t" : : [loops] "r" (10*us) : "memory"\
//		      );\
//} while(0)

#define Delay_us( US ) _delay_loops( (unsigned int)((double)US * (SystemCoreClock / 3000000.0)) )
//#define Delay( MS ) _delay_loops( (unsigned int)((double)MS * (SystemCoreClock / 3000.0)) )
#define HasMainPower() (GPIO_ReadInputDataBit(GPIO_MainPower, GPIO_Pin_MainPower) == RESET) // Включен ли автомат нагревателя


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