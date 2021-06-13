#pragma once
#include "ConnectionsBuffer.h"
#include "Common.h"
#include "stdint.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stdint.h"
#include "string.h"
#include "UartBuffer.h"
#include "TaskTimeout.h"
#include "ShowerCode.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "FreeRTOS.h"
#include "task.h"

typedef enum { NOTHING = -1, TIMEOUT = 0, STR1 = 1, STR2 = 2, STR3 = 3 } WaitStatus;

class UartStream final
{
public:
    
    void Init()
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        
        // Настраиваем ногу TxD как выход push-pull c альтернативной функцией.
        GPIO_InitTypeDef gpio_init = 
        {
            .GPIO_Pin = GPIO_WIFI_Pin_Tx,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_AF_PP
        };
        GPIO_Init(GPIO_WIFI_USART, &gpio_init);

        // Настраиваем ногу как вход UARTа (RxD).
        gpio_init = 
        {
            .GPIO_Pin = GPIO_WIFI_Pin_Rx,
            .GPIO_Speed = GPIO_Speed_50MHz,
            .GPIO_Mode = GPIO_Mode_IN_FLOATING
        };
        GPIO_Init(GPIO_WIFI_USART, &gpio_init);
        
        // Заполняем структуру настройками UARTa.
        USART_InitTypeDef uart_struct = 
        {
            .USART_BaudRate = kWiFiUartSpeed,
            .USART_WordLength = USART_WordLength_8b,
            .USART_StopBits = USART_StopBits_1,
            .USART_Parity = USART_Parity_No,
            .USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
            .USART_HardwareFlowControl = USART_HardwareFlowControl_None
        };

        // В методе USART_Init есть ошибка, подробности по ссылке
        // http://we.easyelectronics.ru/STM32/nastroyka-uart-v-stm32-i-problemy-dvoichno-desyatichnoy-arifmetiki.html
        USART_Init(WIFI_USART, &uart_struct);   // Инициализируем UART.
        
        DMA_DeInit(WIFI_DMA_CH_RX);
        DMA_DeInit(WIFI_DMA_CH_TX);
        
        DMA_InitTypeDef dma_init_struct = 
        {
            .DMA_PeripheralBaseAddr = (uint32_t)&(WIFI_USART->DR),
            .DMA_MemoryBaseAddr = (uint32_t) m_rxFifoBuf,
            .DMA_DIR = DMA_DIR_PeripheralSRC,
            .DMA_BufferSize = kUartRxFifoSize,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
            .DMA_Mode = DMA_Mode_Circular,
            .DMA_Priority = DMA_Priority_Low,
            .DMA_M2M = DMA_M2M_Disable
        };
        DMA_Init(WIFI_DMA_CH_RX, &dma_init_struct);
        
        // Разрешить DMA для USART.
        USART_DMACmd(WIFI_USART, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
        
        // Включаем UART.
        USART_Cmd(WIFI_USART, ENABLE);
        
        // Старт приема через DMA.
        DMA_Cmd(WIFI_DMA_CH_RX, ENABLE);
    }
    
    WaitStatus WaitLine(const char* str1, const char* str2, const char* str3, uint16_t timeout_msec)
    {
        TaskTimeout tim(timeout_msec);
        WaitStatus result = WaitStatus::TIMEOUT;
        
        while (true)
        {
            if (CopyRingBuf())
            {
                if (!HandleIpd())
                {
                    result = ReadLine(str1, str2, str3);

                    if (result != WaitStatus::NOTHING)
                    {
                        return result;
                    }
                }
            }

            if (tim.TimeIsUp())
            {
                return WaitStatus::TIMEOUT;
            }
            else
            {
                taskYIELD();
            }
        }
    }

    WaitStatus WaitLine(const char* str1, const char* str2, uint16_t timeout_msec)
    {
        return WaitLine(str1, str2, 0, timeout_msec);
    }

    WaitStatus WaitLine(const char* str1, uint16_t timeout_msec)
    {
        return WaitLine(str1, 0, 0, timeout_msec);
    }
  
    void WriteLine(const char* str)
    {	
        DMAWriteData(str, strlen(str));
    }
    
    void WriteData(const char* data, const uint16_t count)
    {
        DMAWriteData(data, count);
    }
   
    WaitStatus SendResponse(const uint8_t connection_id, const char* data, const uint16_t count)
    {	
        uint8_t data_str_len = Common::DigitsCount(count);
        char cipsend[18];
        memcpy(cipsend, "AT+CIPSEND=", 11);
    
        cipsend[11] = Common::itoa(connection_id);  // Converts an integer value to a null-terminated string.
        cipsend[12] = ',';
        Common::itoa(count, cipsend + 13);  		// Будет добавлен null-терминатор.

        cipsend[13 + data_str_len] = '\r';
        cipsend[14 + data_str_len] = '\n';
        cipsend[15 + data_str_len] = '\0';
        
        WriteData(cipsend, 15 + data_str_len);
        if (WaitLine("OK", "ERROR", 2000) == WaitStatus::STR1)
        {
            if (WaitSpecific("> ", 2, 2000))
            {
                WriteData(data, count);
                return WaitLine("SEND OK", "SEND FAIL", "ERROR", 2000);
            }
        }
        return WaitStatus::TIMEOUT;
    }

    uint8_t GetRequestSize(uint8_t &connection_id)
    {
        return m_connectionBuffer.GetRequestSize(connection_id);
    }

    ShowerCode ReadRequest(const uint8_t connection_id, uint8_t* buf)
    {
        return m_connectionBuffer[connection_id]->Take(buf);	
    }

    uint8_t GetRequest(uint8_t &connection_id)
    {
        if (CopyRingBuf())
        {
            // Сначала попытаться считать как структуру IPD.
            if(IsIpd())
            {
                uint8_t length = ReadIpd(connection_id);
                if (length)
                {
                    return length;
                }
            }
            else
            {
                HandleLine();
            }
        }
        return 0;
    }

private:
    
    // Буффер в который копируем данные из RX_FIFO_BUF.
    uint8_t m_rxBuf[kUartRxFifoSize] = {0};
    char m_rxStrBuf[kUartMaxStrLen] = {0};  // Буфер для записи строк из кольцевого буффера m_rxBuf.
    ConnectionsBuffer m_connectionBuffer;
    uint16_t m_count = 0;
    uint16_t m_head = 0;
    uint16_t m_head2 = kUartRxFifoSize;
    uint16_t m_tail = 0;
    volatile uint8_t m_rxFifoBuf[kUartRxFifoSize];

    bool CopyRingBuf()
    {
        // Возвращает кол. байт сколько осталось до 0.
        // После 0 сбрасывается на RX_FIFO_SZ.
        uint16_t dma_left = DMA_GetCurrDataCounter(WIFI_DMA_CH_RX);	
        while (m_head2 != dma_left)
        {
            m_rxBuf[m_tail] = m_rxFifoBuf[m_tail];
            m_tail = (m_tail + 1) % kUartRxFifoSize;
                
            if (m_count < kUartRxFifoSize)
            {
                m_count++;
            }
                
            --m_head2;
            if (m_head2 == 0)
            {
                m_head2 = kUartRxFifoSize;		
            }
        }
        return m_count;
    }
    
    void LineReceived(const char* str, const uint8_t str_length)
    {
        if (str_length > 1) // Нет смысла рассматривать строки короче 2 символов.
            {
                if (Common::streql(str + 1, ",CONNECT"))
                {
                    uint8_t connection_id = Common::ctoi(str[0]);
                    m_connectionBuffer.Clear(connection_id);
                }
                else if (Common::streql(str + 1, ",CLOSED"))
                {
                    uint8_t connection_id = Common::ctoi(str[0]);
                    m_connectionBuffer.Clear(connection_id);
                }
                else if (Common::streql(str + 1, ",CONNECT FAIL"))
                {
                    uint8_t connection_id = Common::ctoi(str[0]);
                    m_connectionBuffer.Clear(connection_id);
                }
                else if (Common::streql(str, "WIFI CONNECTED"))
                {
                    m_connectionBuffer.ClearAll();
                }
                else if (Common::streql(str, "WIFI DISCONNECT"))
                {
                    m_connectionBuffer.ClearAll();
                }
            }
    }

    bool HandleIpd()
    {
        if (IsIpd())
        {
            uint8_t connection_id;
            ReadIpd(connection_id);
            return true;
        }
        return false;
    }

    bool HandleLine()
    {
        uint16_t big_count;
        if (GetLineLength(big_count))
        {
            uint8_t count = CopyLine(m_rxStrBuf, big_count);
            LineReceived(m_rxStrBuf, count);
            return true;
        }
        return false;
    }

    uint16_t GetAbsoluteOffset(uint16_t offset)
    {
        uint16_t head = m_head;   // Копируем

        while(offset--)
        {
            head = (head + 1) % kUartRxFifoSize;
        }
    
        return head;
    }

    // Возвращает количество байт до символов \r\n.
    bool GetLineLength(uint16_t &count)
    {
        char prev = '\0';
        uint16_t head = m_head;
        for (uint16_t i = 0; i < m_count; i++)
        {
            char ch = m_rxBuf[head];
            if (prev == '\r' && ch == '\n')
            {
                count = i - 1;
                return true;
            }
            prev = ch;
            head = (head + 1) % kUartRxFifoSize;
        }
        return false;
    }

    // Копирует в строку str данные из кольцевого буффера m_buf и смещает указатель m_head.
    uint8_t CopyLine(char* str, const uint16_t count)
    {
        uint16_t tmp_count = count;
    
        // Строка может быть не больше UART_MAX_STR_LEN
        // Следовательно скопировать начиная с конца не больше чем UART_MAX_STR_LEN
    
        if(count >= kUartMaxStrLen)
        {
            tmp_count = (kUartMaxStrLen - 1);
            uint16_t overflowCount = (count - (kUartMaxStrLen - 1));  // -1 для учета нуль-терминатора

            // Пропустить начало строки которое не вмещается в буффер str.
            m_head = GetAbsoluteOffset(overflowCount);
        }
    
        for (uint16_t i = 0; i < tmp_count; i++)
        {
            *str++ = m_rxBuf[m_head];
            m_head = (m_head + 1) % kUartRxFifoSize;
        }
        *str = 0;  // Нуль-терминатор в конце строки
    
        m_head = (m_head + 1) % kUartRxFifoSize;  // учесть \r
        m_head = (m_head + 1) % kUartRxFifoSize;  // учесть \n
        m_count -= (count + 2);  // Уменьшить счетчик с учетом \r\n
        return tmp_count;
    }

    // Возвращает индекс первого вхождения строки начиная от 0.
    bool Find(const char ch, const uint16_t offset, uint16_t &position)
    {
        uint16_t absOffset = GetAbsoluteOffset(offset);

        if (offset >= m_count)
        {
            return false;
        }
    
        // сколько байт доступно
        for(uint16_t i = 0 ; i < (m_count - offset) ; i++)
        {
            char c = m_rxBuf[absOffset];
            if (c == ch)
            {
                position = i;
                return true;
            }
            absOffset = (absOffset + 1) % kUartRxFifoSize;
        }
        return false;
    }

    // Проверяет начинается ли кольцевой буффер _buf со строки '+IPD'.
    bool IsIpd()
    {
        const static char ipdHeader[] = "+IPD";
        if (m_count < 4)
        {
            return false;
        }
        else
        {
            uint16_t head = m_head;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (m_rxBuf[head] != ipdHeader[i])
                {
                    return false;
                }

                head = (head + 1) % kUartRxFifoSize;
            }

            return true;
        }
    }

    bool StartWith(const char* data, const uint16_t count)
    {
        // Если в кольцевом буффере байт меньше чем запрошено то нет смысла проверять вхождение
        if(m_count >= count)
        {
            uint16_t head = m_head;
            for (uint16_t i = 0; i < count; i++)
            {
                if (m_rxBuf[head] != data[i])
                {
                    return false;
                }

                head = (head + 1) % kUartRxFifoSize;
            }
            return true;
        }
        return false;
    }

    bool WaitSpecific(const char* data, const uint16_t count, const uint16_t timeout_msec)
    {
        TaskTimeout tim(timeout_msec);
        while (true)
        {
            if (CopyRingBuf())
            {
                if (!HandleIpd())
                {
                    if (!HandleLine())
                    {
                        if (StartWith(data, count))
                        {
                            m_head = GetAbsoluteOffset(count);
                            m_count -= count;
                            return true;
                        }
                    }
                }
            }

            if (tim.TimeIsUp())
            {
                return false;
            }
            else
            {
                taskYIELD();
            }
        }
    }
    
    WaitStatus ReadLine(const char* str1, const char* str2, const char* str3)
    {
        if (HandleLine())
        {
            if (Common::streql(m_rxStrBuf, str1))
            {
                return WaitStatus::STR1;
            }

            if (str2 && Common::streql(m_rxStrBuf, str2))
            {
                return WaitStatus::STR2;
            }

            if (str3 && Common::streql(m_rxStrBuf, str3))
            {
                return WaitStatus::STR3;
            }
        }
        return WaitStatus::NOTHING;
    }

    uint8_t ReadIpd(uint8_t &connection_id)
    {
        // Минимум должно быть 9 символов.
        if(m_count >= 9)
        {
            connection_id = Common::ctoi(m_rxBuf[GetAbsoluteOffset(5)]);
            uint16_t pos;
            if (Find(':', 8, pos)) // Находим символ ':' начиная со смещения 8.
                {
                    uint16_t dataStrLen = pos + 1;  // Позиция начинается от 0 -> нужно прибавить 1.
                
                    //////////////////////////////////////////////
                    // Преобразуем строковое число в uint8_t
                    uint8_t length = 0;
                    uint16_t absOffset = GetAbsoluteOffset(7);
                    for (uint8_t i = 0; i < dataStrLen; i++)
                    {
                        length = length * 10 + Common::ctoi(m_rxBuf[absOffset]);
                        absOffset = (absOffset + 1) % kUartRxFifoSize;
                    }
                    //////////////////////////////////////////////

                    // Сколько есть уже считанных данных?
                    uint16_t count = m_count - 8 - dataStrLen;
                    if (count >= length)
                    {	
                        IpdBuffer* ipdBuf = m_connectionBuffer[connection_id];
                        absOffset = GetAbsoluteOffset(dataStrLen + 8);
                        for (uint8_t i = 0; i < length; i++)
                        {
                            ipdBuf->WriteByte(m_rxBuf[absOffset]);
                            absOffset = (absOffset + 1) % kUartRxFifoSize;
                        }
                    
                        ///////////////////////////////////////////////////
                        // Сместить заголовок на колличество считанных байт
                        uint16_t sz = (8 + dataStrLen + length);
                        m_count -= sz;
                        m_head = GetAbsoluteOffset(sz);
                        ///////////////////////////////////////////////////
                    
                        return ipdBuf->GetRequestSize();
                    }
                }
        }
        return 0;
    }
    
    void DMAWriteData(const char* data, const uint16_t count)
    {
        DMA_InitTypeDef DMA_InitStructure = 
        {
            .DMA_PeripheralBaseAddr = (uint32_t)&(WIFI_USART->DR),
            .DMA_MemoryBaseAddr = (uint32_t) data,
            .DMA_DIR = DMA_DIR_PeripheralDST,
            .DMA_BufferSize = count,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
            .DMA_Mode = DMA_Mode_Normal,
                // Отправить буфер однократно.
            .DMA_Priority = DMA_Priority_Low,
            .DMA_M2M = DMA_M2M_Disable
        };
    
        DMA_Init(WIFI_DMA_CH_TX, &DMA_InitStructure);
        
        DMA_Cmd(WIFI_DMA_CH_TX, ENABLE); 	// Старт отправки.
        
        // Ждем, пока отправляются данные.
        while(DMA_GetFlagStatus(WIFI_DMA_FLAG) == RESET) 
        {
            taskYIELD();
        }
        
        // Необходимо выключить DMA.
        DMA_Cmd(WIFI_DMA_CH_TX, DISABLE);
        
        // Необходимо снять флаг завершения операции.
        DMA_ClearFlag(WIFI_DMA_FLAG);
    }
};

extern UartStream g_uartStream;
