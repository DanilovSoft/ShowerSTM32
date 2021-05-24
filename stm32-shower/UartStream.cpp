#include "stdint.h"
#include "UartStream.h"
#include "misc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stdint.h"
#include "string.h"
#include "UartBuffer.h"
#include "Common.h"
#include "Timeout.h"
#include "ShowerCode.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "FreeRTOS.h"
#include "task.h"

UartStream uartStream;
volatile uint8_t RX_FIFO_BUF[RX_FIFO_SZ];

bool UartStream::CopyRingBuf()
{
	/* Возвращает кол. байт сколько осталось до 0 */
	/* После 0 сбрасывается на RX_FIFO_SZ */
	uint16_t dma_left = DMA_GetCurrDataCounter(WIFI_DMA_CH_RX);	
	while (_head2 != dma_left)
	{
		_buf[_tail] = RX_FIFO_BUF[_tail];
		_tail = (_tail + 1) % RX_FIFO_SZ;
				
		if (_count < RX_FIFO_SZ)
			_count++;
    			
		--_head2;
		if (_head2 == 0)
			_head2 = RX_FIFO_SZ;		
	}
	return _count;
}
    
void UartStream::LineReceived(const char* str, const uint8_t strLength)
{
	if (strLength > 1) // Нет смысла рассматривать строки короче 2 символов
	{
    	if (streql(str + 1, ",CONNECT"))
    	{
        	uint8_t connection_id = ctoi(str[0]);
        	_connectionBuffer.Clear(connection_id);
    	}
    	else if (streql(str + 1, ",CLOSED"))
    	{
        	uint8_t connection_id = ctoi(str[0]);
        	_connectionBuffer.Clear(connection_id);
    	}
    	else if (streql(str + 1, ",CONNECT FAIL"))
    	{
        	uint8_t connection_id = ctoi(str[0]);
        	_connectionBuffer.Clear(connection_id);
    	}
		else if (streql(str, "WIFI CONNECTED"))
		{
			_connectionBuffer.ClearAll();
		}
		else if (streql(str, "WIFI DISCONNECT"))
		{
			_connectionBuffer.ClearAll();
		}
	}
}

bool UartStream::HandleIpd()
{
	if (IsIpd())
	{
		uint8_t connection_id;
		ReadIpd(connection_id);
		return true;
	}
	return false;
}

bool UartStream::HandleLine()
{
	uint16_t big_count;
	if (GetLineLength(big_count))
	{
		uint8_t count = CopyLine(_str_buf, big_count);
		LineReceived(_str_buf, count);
		return true;
	}
	return false;
}

uint16_t UartStream::GetAbsoluteOffset(uint16_t offset)
{
	uint16_t head = _head;  // Копируем
	while (offset--)
		head = (head + 1) % RX_FIFO_SZ;
    
	return head;
}

bool UartStream::GetLineLength(uint16_t &count)
{
	char prev = '\0';
	uint16_t head = _head;
	for (uint16_t i = 0; i < _count; i++)
	{
		char ch = _buf[head];
		if (prev == '\r' && ch == '\n')
		{
			count = i - 1;
			return true;
		}
		prev = ch;
		head = (head + 1) % RX_FIFO_SZ;
	}
	return false;
}

uint8_t UartStream::CopyLine(char* str, const uint16_t count)
{
	uint16_t tmp_count = count;
    
	// Строка может быть не больше UART_MAX_STR_LEN
	// Следовательно скопировать начиная с конца не больше чем UART_MAX_STR_LEN
    
	if (count >= UART_MAX_STR_LEN)
	{
		tmp_count = (UART_MAX_STR_LEN - 1);
		uint16_t overflowCount = (count - (UART_MAX_STR_LEN - 1)); // -1 для учета нуль-терминатора
		/* Пропустить начало строки которое не вмещается в буффер str */
		_head = GetAbsoluteOffset(overflowCount);
	}
    
	for (uint16_t i = 0; i < tmp_count; i++)
	{
		*str++ = _buf[_head];
		_head = (_head + 1) % RX_FIFO_SZ;
	}
	*str = 0; // Нуль-терминатор в конце строки
    
	_head = (_head + 1) % RX_FIFO_SZ; // учесть \r
	_head = (_head + 1) % RX_FIFO_SZ; // учесть \n
	_count -= (count + 2); // Уменьшить счетчик с учетом \r\n
	return tmp_count;
}

bool UartStream::Find(const char ch, const uint16_t offset, uint16_t &position)
{
	uint16_t absOffset = GetAbsoluteOffset(offset);

    if (offset >= _count)
        return false;
    
	// сколько байт доступно
    for (uint16_t i = 0; i < (_count - offset); i++)
	{
		char c = _buf[absOffset];
		if (c == ch)
		{
			position = i;
			return true;
		}
		absOffset = (absOffset + 1) % RX_FIFO_SZ;
	}
	return false;
}

bool UartStream::IsIpd()
{
	const static char ipdHeader[] = "+IPD";
	if (_count < 4)
	{
		return false;
	}
	else
	{
		uint16_t head = _head;
		for (uint8_t i = 0; i < 4; i++)
		{
			if (_buf[head] != ipdHeader[i])
				return false;

			head = (head + 1) % RX_FIFO_SZ;
		}

		return true;
	}
}

bool UartStream::StartWith(const char* data, const uint16_t count)
{
	// Если в кольцевом буффере байт меньше чем запрошено то нет смысла проверять вхождение
	if (_count >= count)
	{
		uint16_t head = _head;
		for (uint16_t i = 0; i < count; i++)
		{
			if (_buf[head] != data[i])
				return false;

			head = (head + 1) % RX_FIFO_SZ;
		}
		return true;
	}
	return false;
}

bool UartStream::WaitSpecific(const char* data, const uint16_t count, const uint16_t timeoutMsec)
{
	TaskTimeout tim(timeoutMsec);
	while (1)
	{
		if (CopyRingBuf())
		{
			if (!HandleIpd())
			{
				if (!HandleLine())
				{
					if (StartWith(data, count))
					{
						_head = GetAbsoluteOffset(count);
						_count -= count;
						return true;
					}
				}
			}
		}

		if (tim.TimeIsUp())
			return false;
		else
			taskYIELD();
	}
}
    
WaitStatus UartStream::ReadLine(const char* str1, const char* str2, const char* str3)
{
    if (HandleLine())
    {
        if (streql(_str_buf, str1))
            return WaitStatus::STR1;

        if (str2 && streql(_str_buf, str2))
            return WaitStatus::STR2;

        if (str3 && streql(_str_buf, str3))
            return WaitStatus::STR3;
    }
	return WaitStatus::NOTHING;
}

uint8_t UartStream::ReadIpd(uint8_t &connection_id)
{
	// минимум должно быть 9 символов
	if (_count >= 9)
	{
		connection_id = ctoi(_buf[GetAbsoluteOffset(5)]);
		uint16_t pos;
		if (Find(':', 8, pos)) // Находим символ ':' начиная со смещения 8
		{
			uint16_t dataStrLen = pos + 1; // Позиция начинается от 0 -> нужно прибавить 1
    			
			//////////////////////////////////////////////
			// Преобразуем строковое число в uint8_t
			uint8_t length = 0;
			uint16_t absOffset = GetAbsoluteOffset(7);
			for (uint8_t i = 0; i < dataStrLen; i++)
			{
				length = length * 10 + ctoi(_buf[absOffset]);
				absOffset = (absOffset + 1) % RX_FIFO_SZ;
			}
			//////////////////////////////////////////////

			// сколько есть уже считанных данных?
			uint16_t count = _count - 8 - dataStrLen;
			if (count >= length)
			{	
				IpdBuffer* ipdBuf = _connectionBuffer[connection_id];
				absOffset = GetAbsoluteOffset(dataStrLen + 8);
    			for (uint8_t i = 0; i < length; i++)
    			{
        			ipdBuf->WriteByte(_buf[absOffset]);
        			absOffset = (absOffset + 1) % RX_FIFO_SZ;
    			}
    				
				///////////////////////////////////////////////////
				// Сместить заголовок на колличество считанных байт
				uint16_t sz = (8 + dataStrLen + length);
				_count -= sz;
				_head = GetAbsoluteOffset(sz);
				///////////////////////////////////////////////////
    				
				return ipdBuf->GetRequestSize();
			}
		}
	}
	return 0;
}
	
void UartStream::DMAWriteData(const char* data, const uint16_t count)
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
		.DMA_Mode = DMA_Mode_Normal,   // Отправить буффер однократно
		.DMA_Priority = DMA_Priority_Low,
		.DMA_M2M = DMA_M2M_Disable
	};
	
	DMA_Init(WIFI_DMA_CH_TX, &DMA_InitStructure);
		
	DMA_Cmd(WIFI_DMA_CH_TX, ENABLE);	// Старт отправки
		
	// Ждем, пока отправляются данные
	while (DMA_GetFlagStatus(WIFI_DMA_FLAG) == RESET) 
	{
		taskYIELD();
	}
    	
	/* Необходимо выключить DMA */
	DMA_Cmd(WIFI_DMA_CH_TX, DISABLE);
		
	/* Необходимо снять флаг завершения операции */
	DMA_ClearFlag(WIFI_DMA_FLAG);
}

void UartStream::Init()
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
		
    //Заполняем структуру настройками UARTa.
	USART_InitTypeDef uart_struct;
	USART_StructInit(&uart_struct);
	uart_struct = 
	{
		.USART_BaudRate = WIFI_UART_Speed,
		.USART_WordLength = USART_WordLength_8b,
		.USART_StopBits = USART_StopBits_1,
		.USART_Parity = USART_Parity_No,
		.USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
		.USART_HardwareFlowControl = USART_HardwareFlowControl_None
	};

    // В методе USART_Init есть ошибка, подробности по ссылке
    // http://we.easyelectronics.ru/STM32/nastroyka-uart-v-stm32-i-problemy-dvoichno-desyatichnoy-arifmetiki.html
	USART_Init(WIFI_USART, &uart_struct);  // Инициализируем UART
		
	DMA_DeInit(WIFI_DMA_CH_RX);
	DMA_DeInit(WIFI_DMA_CH_TX);
        
	DMA_InitTypeDef DMA_InitStructure = 
	{
		.DMA_PeripheralBaseAddr = (uint32_t)&(WIFI_USART->DR),
		.DMA_MemoryBaseAddr = (uint32_t) RX_FIFO_BUF,
		.DMA_DIR = DMA_DIR_PeripheralSRC,
		.DMA_BufferSize = RX_FIFO_SZ,
		.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
		.DMA_MemoryInc = DMA_MemoryInc_Enable,
		.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
		.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
		.DMA_Mode = DMA_Mode_Circular,
		.DMA_Priority = DMA_Priority_Low,
		.DMA_M2M = DMA_M2M_Disable
	};
	DMA_Init(WIFI_DMA_CH_RX, &DMA_InitStructure);
    	
	// Разрешить DMA для USART.
	USART_DMACmd(WIFI_USART, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
        
    // Включаем UART.
	USART_Cmd(WIFI_USART, ENABLE);
    	
	// Старт приема через DMA.
	DMA_Cmd(WIFI_DMA_CH_RX, ENABLE);
}
    
WaitStatus UartStream::WaitLine(const char* str1, const char* str2, const char* str3, uint16_t timeoutMsec)
{
	TaskTimeout tim(timeoutMsec);
	WaitStatus result = WaitStatus::TIMEOUT;
		
	while (true)
	{
		if (CopyRingBuf())
		{
			if (!HandleIpd())
			{
				result = ReadLine(str1, str2, str3);
				if (result != WaitStatus::NOTHING)
					return result;
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

WaitStatus UartStream::WaitLine(const char* str1, const char* str2, uint16_t timeoutMsec)
{
	return WaitLine(str1, str2, 0, timeoutMsec);
}

WaitStatus UartStream::WaitLine(const char* str1, uint16_t timeoutMsec)
{
	return WaitLine(str1, 0, 0, timeoutMsec);
}
	
void UartStream::WriteLine(const char* str)
{	
	DMAWriteData(str, strlen(str));
}
	
void UartStream::WriteData(const char* data, const uint16_t count)
{
	DMAWriteData(data, count);
}
    
WaitStatus UartStream::SendResponse(const uint8_t connectionId, const char* data, const uint16_t count)
{	
	uint8_t dataStrLen = DigitsCount(count);
	char cipsend[18];
	memcpy(cipsend, "AT+CIPSEND=", 11);
	
	cipsend[11] = itoa(connectionId);     // Converts an integer value to a null-terminated string
	cipsend[12] = ',';
	itoa(count, cipsend + 13);		// будет добавлен null-терминатор

	cipsend[13 + dataStrLen] = '\r';
	cipsend[14 + dataStrLen] = '\n';
	cipsend[15 + dataStrLen] = '\0';
		
	WriteData(cipsend, 15 + dataStrLen);
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

uint8_t UartStream::GetRequestSize(uint8_t &connection_id)
{
	return _connectionBuffer.GetRequestSize(connection_id);
}

ShowerCode UartStream::ReadRequest(const uint8_t connection_id, uint8_t* buf)
{
	return _connectionBuffer[connection_id]->Take(buf);	
}
	
uint8_t UartStream::GetRequest(uint8_t &connection_id)
{
	if (CopyRingBuf())
	{
		// сначала попытаться считать как структуру IPD
		if (IsIpd())
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
