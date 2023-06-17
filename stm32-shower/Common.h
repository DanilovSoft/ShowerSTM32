#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Properties.h"
#include "Defines.h"


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

//#ifdef  NDEBUG
//# define assert_break(__e) ((void)0)
//#else
//
//__attribute__((noreturn)) void __break_func(const char * file_name, int line);
//        
//# define assert_break(__e) ((__e) ? (void)0 : __break_func (__FILE__, __LINE__))
//#endif

class Common final
{
public:

    static void InitPeripheral(const PropertyStruct* const properties)
    {
        InnerInitPeripheral(properties);
    }
    
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
    
    static bool ButtonWPSPressed()
    {
        return GPIO_ReadInputDataBit(GPIO_WPS, GPIO_WPS_Pin) == RESET;
    }
    
    // Включает лог 1 на ногу CH_PD.
    static void EnableEsp8266()
    {
        GPIO_SetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
    }
    
    // Лог 0 на ногу CH_PD.
    static void DisableEsp8266()
    {
        GPIO_ResetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
    }
    
    // Включает питание сенсорной кнопки.
    static void PowerOnSensorSwitch()
    {
        GPIO_SetBits(SensorSwitch_Power_GPIO, SensorSwitch_Power_Pin);
    }
    
    // Выключает питание сенсорной кнопки.
    static void PowerOffSensorSwitch()
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
    
    // Лог 1 на ноге Trig.
    static void WaterLevelEnableTrig()
    {
        GPIO_SetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    }
    
    // Лог 0 на ноге Trig.
    static void WaterLevelDisableTrig()
    {
        GPIO_ResetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
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
    
    // Отключает ногу бипера от таймера.
    static void DisableBeeper()
    {
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_Buzzer_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_IN_FLOATING
        };
        GPIO_Init(GPIO_Buzzer, &gpio_init_struct);
    }
    
    // Подключает ногу бипера к таймеру.
    static void EnableBeeper()
    {
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_Buzzer_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_AF_PP
        };
        GPIO_Init(GPIO_Buzzer, &gpio_init_struct);
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

    static bool ArrayEquals(const uint8_t* array1, const uint8_t arr1Size, const uint8_t* array2, const uint8_t arr2_size)
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

    static void InitWaterLevelPeripheral();
    
    static void InitBeeperPeripheral()
    {
        DisableBeeper();
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        
        TIM_TimeBaseInitTypeDef tim_time_base_init_struct = 
        {
            .TIM_Prescaler = (uint16_t)(SystemCoreClock / 100000 - 1), 
              // 100 KHz timebase.
            .TIM_CounterMode = TIM_CounterMode_Up,
            .TIM_Period = (uint16_t)(SystemCoreClock / 10000 - 1), 
                  // Arbitary placeholder 100 Hz.
            .TIM_ClockDivision = TIM_CKD_DIV1, 
            							// Без  делителя.
            .TIM_RepetitionCounter = 0
        };
        TIM_TimeBaseInit(Buzzer_TIM, &tim_time_base_init_struct);

        TIM_OCInitTypeDef tim_oc_init_struct = 
        {
            .TIM_OCMode = TIM_OCMode_PWM1,
            .TIM_OutputState = TIM_OutputState_Enable,
            .TIM_OutputNState = TIM_OutputNState_Disable,
            .TIM_Pulse = (uint16_t)((tim_time_base_init_struct.TIM_Period + 1) / 2), 
                   // 50% Duty.
            .TIM_OCPolarity = TIM_OCPolarity_High,
            .TIM_OCNPolarity = TIM_OCPolarity_High,
            .TIM_OCIdleState = TIM_OCIdleState_Reset,
            .TIM_OCNIdleState = TIM_OCNIdleState_Reset
        };
        TIM_OC1Init(Buzzer_TIM, &tim_oc_init_struct);
        
        TIM_OC1PreloadConfig(Buzzer_TIM, TIM_OCPreload_Enable);
        TIM_ARRPreloadConfig(Buzzer_TIM, ENABLE);
        
        static constexpr uint16_t period = 100000 / 1000;    // 1000 гц.
        Buzzer_TIM->ARR = period - 1;
        Buzzer_TIM->CCR1 = period / 2;
        
        DisableBeeper();
        TIM_Cmd(Buzzer_TIM, ENABLE);
    }
    
    static void InitUartPeripheral(const uint32_t memory_base_addr);
    
    static void AssertAllTasksInitialized();
    
    // Отправляет данные в шину 1-wire через интерфейс UART с помощью DMA.
    // PS. UART должен быть заранее настроен на совместимый с 1-wire режим.
    static void OneWireSendBits(const uint32_t memory_base_address, const uint8_t num_bits) 
    {
        // Сбрасываем канал DMA.
        DMA_DeInit(OW_DMA_CH_RX);
    
        // Канал DMA будет читать данные из UART устройства в память.
        DMA_InitTypeDef dma_rx_init_struct = 
        {
            .DMA_PeripheralBaseAddr = (uint32_t) &(OneWire_USART->DR),
            .DMA_MemoryBaseAddr = memory_base_address,
            .DMA_DIR = DMA_DIR_PeripheralSRC,
            .DMA_BufferSize = num_bits,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
            .DMA_Mode = DMA_Mode_Normal,
            .DMA_Priority = DMA_Priority_Low,
            .DMA_M2M = DMA_M2M_Disable,
        };
        DMA_Init(OW_DMA_CH_RX, &dma_rx_init_struct);
    
        // Сбрасываем канал DMA.
        DMA_DeInit(OW_DMA_CH_TX);
    
        // Канал DMA будет отправлять данные из памяти в UART устройство.
        DMA_InitTypeDef dma_tx_init_struct = 
        {
            .DMA_PeripheralBaseAddr = (uint32_t) &(OneWire_USART->DR),
            .DMA_MemoryBaseAddr = memory_base_address,
            .DMA_DIR = DMA_DIR_PeripheralDST,
            .DMA_BufferSize = num_bits,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
            .DMA_Mode = DMA_Mode_Normal,
            .DMA_Priority = DMA_Priority_Low,
            .DMA_M2M = DMA_M2M_Disable,
        };
        DMA_Init(OW_DMA_CH_TX, &dma_tx_init_struct);

        USART_ClearFlag(OneWire_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
        USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
        
        DMA_Cmd(OW_DMA_CH_RX, ENABLE);
        DMA_Cmd(OW_DMA_CH_TX, ENABLE);

        // Ждём завершение отправки.
        while(DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) 
        {
            taskYIELD();
        }

        DMA_Cmd(OW_DMA_CH_TX, DISABLE);
        DMA_Cmd(OW_DMA_CH_RX, DISABLE);
        USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);
    }
    
    // Включает I²C.
    static void InitI2C();
    
private:
    
    static void InnerInitPeripheral(const PropertyStruct* const properties)
    {
        InitWaterLevel(properties);
        InitWiFi();
        InitTempSensor();
        InitLedLight(properties);
        InitHeater();
        InitButtons();
        InitValve();
    }
    
    static void InitHeater();
    static void InitWaterLevel(const PropertyStruct* const properties);
    static void InitWiFi();
    static void InitTempSensor();
    static void InitLedLight(const PropertyStruct* const properties);
    static void InitButtons();
    static void InitValve();
};