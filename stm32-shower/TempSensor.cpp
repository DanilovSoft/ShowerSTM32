#include "TempSensor.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "Common.h"
#include "Properties.h"
#include "Common.h"
#include "string.h"
#include "Eeprom.h"

TempSensor _tempSensorTask;

// сокращенно ow_buf.
volatile uint8_t _oneWireBuffer[8] = { };

#pragma region Public

void TempSensor::Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
		
	// USART Tx
	GPIO_InitTypeDef gpioInitStruct = 
	{
		.GPIO_Pin = OW_GPIO_Pin_Tx,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_AF_OD,
	};
	
	GPIO_Init(OneWire_GPIO, &gpioInitStruct);

	USART_InitTypeDef usartInitStructure = 
	{
		.USART_BaudRate = 115200,
		.USART_WordLength = USART_WordLength_8b,
		.USART_StopBits = USART_StopBits_1,
		.USART_Parity = USART_Parity_No,
		.USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
		.USART_HardwareFlowControl = USART_HardwareFlowControl_None,
	};

	USART_Init(OneWire_USART, &usartInitStructure);
	USART_Cmd(OneWire_USART, ENABLE);
	USART_HalfDuplexCmd(OneWire_USART, ENABLE);
}

void TempSensor::WaitFirstConversion()
{
	while (!InternalSensorInitialized || !ExternalSensorInitialized) 
	{
		taskYIELD();
	}
}

void TempSensor::Run()
{
	// Дать немного времени на инициализацию.
	vTaskDelay(10 / portTICK_PERIOD_MS);
    
	TryRegisterNewSensors();
	
	// Готовим команду - чтение памяти устройства.
	memcpy(_internalDeviceReadScratchCommand + 1, Properties.InternalTempSensorId, 8);
	memcpy(_externalDeviceReadScratchCommand + 1, Properties.ExternalTempSensorId, 8);
	
	SetResolution(DS18B20_Resolution_9_bit, Properties.InternalTempSensorId); // Для 9 бит время измерения = 750 msec / 16 = 93.75 msec.
	SetResolution(DS18B20_Resolution_9_bit, Properties.ExternalTempSensorId);
	
	while (!TryGetFirstTemps())
	{
		Pause();
	}
    
	while (true)
	{
		Pause();
		TryUpdateTemp();   
	}
}

#pragma endregion

bool TempSensor::SetResolution(DS18B20_Resolution resolution, uint8_t* deviceId)
{
	uint8_t scratchpad[9] = {};
	
	uint8_t readScratchCommand[10] = { MATCH_ROM };
	readScratchCommand[9] = READ_SCRATCHPAD;
	memcpy(readScratchCommand + 1, deviceId, 8);
	
	// Читаем память устройства с проверкой контрольной суммы.
	if(!OneWire_Send(readScratchCommand, 10, scratchpad, 9, true))
	{
		return false;
	}
	
	uint8_t configuration = scratchpad[4];
	
	switch (resolution) 
	{
	case DS18B20_Resolution_9_bit:
		// bit 6 = 0 и bit 5 = 0.
		if(BIT_IS_NOT_SET(configuration, 6) && BIT_IS_NOT_SET(configuration, 5))
		{
			return true;	
		}
		break;
	case DS18B20_Resolution_10_bit:
		// bit 6 = 0 и bit 5 = 1.
		if(BIT_IS_NOT_SET(configuration, 6) && BIT_IS_SET(configuration, 5))
		{
			return true;
		}
		break;
	case DS18B20_Resolution_11_bit:
		// bit 6 = 1 и bit 5 = 0.
		if(BIT_IS_SET(configuration, 6) && BIT_IS_NOT_SET(configuration, 5))
		{
			return true;	
		}
		break;
	case DS18B20_Resolution_12_bit:
		// bit 6 = 1 и bit 5 = 1.
		if(BIT_IS_SET(configuration, 6) && BIT_IS_SET(configuration, 5))
		{
			return true;
		}
		break;
	}
	
	if (!SaveResolution(resolution, deviceId))
	{
		return false;
	}
	
	return true;
}

bool TempSensor::SaveResolution(DS18B20_Resolution resolution, uint8_t* deviceId)
{
	// Нужно записать в устройство новые параметры.

	uint8_t setResolutionCommand[] =
	{ 
		MATCH_ROM,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		WRITE_SCRATCHPAD,
		TH_REGISTER,
		TL_REGISTER,
		(uint8_t)resolution,
	};
		
	memcpy(setResolutionCommand + 1, deviceId, 8);
		
	if (!OneWire_Send(setResolutionCommand, 13))
	{
		return false;
	}
		
//	// Восстанавливаем из EEPROM.
//	uint8_t recallCommand[] = 
//	{
//		MATCH_ROM,
//		0,
//		0,
//		0,
//		0,
//		0,
//		0,
//		0,
//		0,
//		RECALL_E2
//	};
//			
//	memcpy(recallCommand + 1, deviceId, 8);
//			
//	if (!OneWire_Send(recallCommand, 10))
//	{
//		return false;
//	}
//			
//	vTaskDelay(1000 / portTICK_PERIOD_MS);
//			
//	// Читаем память устройства с проверкой контрольной суммы.
//	if(!OneWire_Send(, 10, scratchpad, 9, true))
//	{
//		return false;
//	}
		
	// Теперь скопировать в EEPROM устройства.
	uint8_t copyScratchpadCommand[] = 
	{ 
		MATCH_ROM,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		COPY_SCRATCHPAD
	};
		
	memcpy(copyScratchpadCommand + 1, deviceId, 8);

	if (!OneWire_Send(copyScratchpadCommand, 10))
	{
		return false;
	}
		
	// Не уверен что это нужно но лучше подождать запись в EEPROM.
	vTaskDelay(20 / portTICK_PERIOD_MS);

	return true;
}

void TempSensor::TryRegisterNewSensors()
{
	uint8_t oneDevRepeatCount = 0;
	uint8_t twoDevRepeatCount = 0;
	uint8_t newInternalDevice[8] = {};
	uint8_t newExternalDevice[8] = {};
	
repeat:
	
	uint8_t devices[16] = { };
	uint8_t devCount = OneWire_Scan(devices, 2);
	
	// Если в списке только одно устройство и оно новое то зарегистрировать как датчик для бака.
	if(devCount == 1)
	{
		bool isInternalSensor = ArrayEquals(devices, 8, Properties.InternalTempSensorId, 8);
		
		if (!isInternalSensor)
		{
			bool isExternalSensor = ArrayEquals(devices, 8, Properties.ExternalTempSensorId, 8);
			
			if (!isExternalSensor)
			{
				// На шине обнаружено только одно устройство и оно новое.
				// Значит нужно зарегистрировать его как основной датчик бака.
				// Но ситуация может быть ошибочной поэтому уведомляем пользователя 
				// и выдерживаем паузу что-бы пользователь мог прервать перезапись идентификатора.
				
				if(oneDevRepeatCount == 0)
				{
					oneDevRepeatCount++;
					
					memcpy(newInternalDevice, devices, 8);     // Запомним ид устройства что-бы сравнить при повторной попытке.
					
					RegisteringSensors = true;
				
					vTaskDelay(10000 / portTICK_PERIOD_MS);
					
					// Повторим поиск устройств что-бы дополнительно убедиться что ситуация не ошибочная.
					goto repeat;
				}
				else
				{
					if (!ArrayEquals(devices, 8, newInternalDevice, 8))
					{
						// Идентификатор изменился - что-то пошло не так. Не будем переписывать значение.
						return;
					}
					
					memcpy(_writeOnlyPropertiesStruct.InternalTempSensorId, newInternalDevice, 8);
				
					if (ArrayEquals(_writeOnlyPropertiesStruct.InternalTempSensorId, 8, _writeOnlyPropertiesStruct.ExternalTempSensorId, 8))
					{
						memset(_writeOnlyPropertiesStruct.ExternalTempSensorId, 0, 8);
					}
					
					// Перезаписываем идентификатор датчика.
					_eeprom.Save();
					
					// Пока никто не работает с этим идентификатором можем безопасно перезаписать (не атомарно).
					memcpy(Properties.InternalTempSensorId, newInternalDevice, 8);
				}
			}
		}
	}
	else if(devCount == 2)
	{
		// Если устройств 2, где один это датчик бака, а другое новое то зарегистрировать второй как датчик окружающего воздуха.

		// Является ли первое устройство в массиве датчиком бака.
		bool firstIsInternal = ArrayEquals(devices, 8, Properties.InternalTempSensorId, 8);
		
		bool secondIsInternal;
		
		if (firstIsInternal)
		{
			secondIsInternal = false;
		}
		else
		{
			secondIsInternal = ArrayEquals(devices + 8, 8, Properties.InternalTempSensorId, 8);
		}
		
		if (firstIsInternal || secondIsInternal)
		{
			uint8_t* nextDev = firstIsInternal ? devices + 8 : devices;
			
			if (!ArrayEquals(nextDev, 8, Properties.ExternalTempSensorId, 8))
			{
				// Обнаружен новый датчик. Предупредим пользователя, выдержим паузу, перечитаем идентификатор и зарегистрируем его как датчик воздуха.
				
				if(twoDevRepeatCount == 0)
				{
					twoDevRepeatCount++;
					
					memcpy(newExternalDevice, nextDev, 8);        // Запомним ид устройства что-бы сравнить при повторной попытке.
					
					RegisteringSensors = true;
				
					vTaskDelay(10000 / portTICK_PERIOD_MS);
					
					// Повторим поиск устройств что-бы дополнительно убедиться что ситуация не ошибочная.
					goto repeat;
				}
				else
				{
					if (!ArrayEquals(nextDev, 8, newExternalDevice, 8))
					{
						// Идентификатор изменился - что-то пошло не так. Не будем переписывать значение.
						return;
					}
					
					memcpy(_writeOnlyPropertiesStruct.ExternalTempSensorId, newExternalDevice, 8);
				
					// Перезаписываем идентификатор датчика.
					_eeprom.Save();
					
					// Пока никто не работает с этим идентификатором можем безопасно перезаписать (не атомарно).
					memcpy(Properties.ExternalTempSensorId, newExternalDevice, 8);
				}
			}
		}
	}
}

void TempSensor::OneWire_ToBits(uint8_t ow_byte, volatile uint8_t* ow_bits) 
{
	uint8_t i;
	for (i = 0; i < 8; i++) 
	{
		*ow_bits++ = ow_byte & 0x01 ? OW_1 : OW_0;
		ow_byte >>= 1;
	}
}

uint8_t TempSensor::OneWire_ToByte(volatile uint8_t* ow_bits) 
{
	uint8_t ow_byte = 0;
	for (uint8_t i = 0; i < 8; i++) 
	{
		ow_byte >>= 1;
		if (*ow_bits == OW_R_1) 
		{
			ow_byte |= 0x80;
		}
		ow_bits++;
	}

	return ow_byte;
}

bool TempSensor::OneWire_Reset() 
{
	uint8_t ow_presence;
	USART_InitTypeDef initStructure =
	{
		.USART_BaudRate = 9600,
		.USART_WordLength = USART_WordLength_8b,
		.USART_StopBits = USART_StopBits_1,
		.USART_Parity = USART_Parity_No,
		.USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
		.USART_HardwareFlowControl = USART_HardwareFlowControl_None
	};

	USART_Init(OneWire_USART, &initStructure);

	// Отправляем 0xF0 на скорости 9600
	USART_ClearFlag(OneWire_USART, USART_FLAG_TC);
	USART_SendData(OneWire_USART, 0xF0);
	
	while (USART_GetFlagStatus(OneWire_USART, USART_FLAG_TC) == RESET) 
	{
		taskYIELD();
	}

	ow_presence = USART_ReceiveData(OneWire_USART);

	initStructure = 
	{
		.USART_BaudRate = 115200,
		.USART_WordLength = USART_WordLength_8b,
		.USART_StopBits = USART_StopBits_1,
		.USART_Parity = USART_Parity_No,
		.USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
		.USART_HardwareFlowControl = USART_HardwareFlowControl_None
	};
	
	USART_Init(OneWire_USART, &initStructure);

	if (ow_presence != 0xF0) 
	{
		return true;
	}
	return false;
}

bool TempSensor::ValidateCrc32(uint8_t* data, uint8_t length)
{
	uint8_t crc = 0;
	while (length--)
	{
		CalcCrc32_ibutton_update(crc, *data++);
	}
		
	return crc == 0;
}

void TempSensor::CalcCrc32_ibutton_update(uint8_t &crc, uint8_t data)
{
	crc = crc ^ data;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (crc & 0x01)
		{
			crc = (crc >> 1) ^ 0x8C;
		}
		else 
		{
			crc >>= 1;
		}
	}
}

bool TempSensor::OneWire_Send(const uint8_t* command, uint8_t cLen, uint8_t* data, uint8_t dLen, bool calcCrc) 
{	
	if (OneWire_Reset())
	{
		uint8_t* dataStart = data;
		const uint8_t dataLen = dLen;
		
		uint8_t readStart = dLen ? cLen : 255;
		cLen += dLen;
			
		while (cLen--) 
		{
			uint8_t byte = cLen < dLen ? 0xFF : *command++;
			OneWire_ToBits(byte, _oneWireBuffer);

			// Сбросить читающий канал DMA на значения по умолчанию, перед установкой новых значений (опционально).
			DMA_DeInit(OW_DMA_CH_RX);
			
			DMA_InitTypeDef dmaInitStructure = 
			{
				.DMA_PeripheralBaseAddr = (uint32_t)&(OneWire_USART->DR),
				.DMA_MemoryBaseAddr = (uint32_t)_oneWireBuffer,
				.DMA_DIR = DMA_DIR_PeripheralSRC,
				.DMA_BufferSize = 8,  // 8 байт которые интерпретируем как 8 бит.
				.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
				.DMA_MemoryInc = DMA_MemoryInc_Enable,
				.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
				.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
				.DMA_Mode = DMA_Mode_Normal,
				.DMA_Priority = DMA_Priority_Low,
				.DMA_M2M = DMA_M2M_Disable,
			};
			
			// Настроить читающий канал DMA.
			DMA_Init(OW_DMA_CH_RX, &dmaInitStructure);

			// Сбросить записывающий канал DMA на значения по умолчанию, перед установкой новых значений (опционально).
			DMA_DeInit(OW_DMA_CH_TX);
			
			dmaInitStructure = 
			{
				.DMA_PeripheralBaseAddr = (uint32_t)&(OneWire_USART->DR),
				.DMA_MemoryBaseAddr = (uint32_t)_oneWireBuffer,
				.DMA_DIR = DMA_DIR_PeripheralDST,
				.DMA_BufferSize = 8, // 8 байт которые интерпретируем как 8 бит.
				.DMA_PeripheralInc = DMA_PeripheralInc_Disable,
				.DMA_MemoryInc = DMA_MemoryInc_Enable,
				.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
				.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
				.DMA_Mode = DMA_Mode_Normal,
				.DMA_Priority = DMA_Priority_Low,
				.DMA_M2M = DMA_M2M_Disable,
			};
			
			// Настроить записывающий канал DMA.
			DMA_Init(OW_DMA_CH_TX, &dmaInitStructure);

			// Старт цикла отправки.
			USART_ClearFlag(OneWire_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
			USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE); // Запустили UART.
			DMA_Cmd(OW_DMA_CH_RX, ENABLE); // Запустили DMA на чтение.
			DMA_Cmd(OW_DMA_CH_TX, ENABLE); // Запустили DMA на запись.

			// Ждем, пока не примем 8 байт (которые сконвертируем в 8 бит).
			while (DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) 
			{
				taskYIELD();
			}

			// Отключаем DMA.
			DMA_Cmd(OW_DMA_CH_TX, DISABLE);
			DMA_Cmd(OW_DMA_CH_RX, DISABLE);
			USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);

			// Если прочитанные данные кому-то нужны - запишем их в выходной буфер data.
			if (readStart != 255)
			{
				if (readStart == 0 && dLen > 0) 
				{
					*data++ = OneWire_ToByte(_oneWireBuffer);
					dLen--;
				}
				else 
				{
					readStart--;
				}
			}
		}
			
		if (calcCrc)
		{
			return ValidateCrc32(dataStart, dataLen);
		}
		return true;
	}
	return false;
}

float TempSensor::Decode(uint8_t* scratchPad)
{
	// Температуру мы получаем из двух байт: 0-3 биты это значения после запятой,
	// 4-12 собственно значение температуры, 12-16 знак. Если разрешение 12бит, 
	// то биты значащие, если 11бит, то нулевой не используется, если 9бит, 
	// то не используются с 0-2. Если температура отрицательная, то нужно инвертировать 
	// биты, добавить 1 и вычесть из нуля, чтобы получить отрицательное значение.
	
	const uint8_t TEMP_LSB = 0;
	const uint8_t TEMP_MSB = 1;
	
	uint8_t LSB = scratchPad[TEMP_LSB];
	uint8_t MSB = scratchPad[TEMP_MSB];
	
	float data;
	uint16_t temperature;
	temperature = LSB | (MSB << 8);
 
	if (temperature & 0x8000) 
	{
		temperature = ~temperature + 1;
		data = 0.0 - (temperature * 0.0625);
		return data;
	}
	
	data = temperature * 0.0625;
	return data;
}

//float TempSensor::Decode(uint8_t* scratchPad)
//{
//	const uint8_t TEMP_LSB = 0;
//	const uint8_t TEMP_MSB = 1;
//
//	/*
//		1. Truncating the 0.5 bit - use a simple & mask: raw & 0xFFFE
//		2. Convert to 12 bit value (1/16 of °C) - shift left: (raw & 0xFFFE)<<3
//		3. Subtracting 0.25 (1/4 °C of 1/16) or 0.25/0.0625 = 4: ((raw & 0xFFFE)<<3)-4
//		4. Add the count (count per c - count remain), count per c is constant of 16, and no need to dived by 16 since we are calculating to the 1/16 of °C: +16 - COUNT_REMAIN 
//	*/
//		
//	// Construct the integer value
//	int16_t rawTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 8) | scratchPad[TEMP_LSB];
//	rawTemperature = ((rawTemperature & 0xFFFE) << 3) - 4 + 16 - scratchPad[6];
//	float temp = rawTemperature * 0.0625f;
//	return temp;
//}

void TempSensor::Pause()
{
	vTaskDelay(_pauseMsec / portTICK_PERIOD_MS);
}

bool TempSensor::TryUpdateTemp()
{
	uint8_t scratchpad[9];
	bool internalSuccess = false;
		
	if (OneWire_Send(AllDevicessStartConvert, 2))
	{
		vTaskDelay(_minimumDelayMsec / portTICK_PERIOD_MS);	
    	
		float internalTemp;
		if (TryGetInternalTemp(internalTemp))
		{
			InternalTemp = internalTemp;
			
			// Записать в скользящее окно.
			_intTempSum -= _intTempBuf[_intTempHead];
			_intTempSum += InternalTemp;
			_intTempBuf[_intTempHead] = InternalTemp;
			_intTempHead = (_intTempHead + 1) % Properties.Customs.InternalTemp_Avg_Size;
			AverageInternalTemp = _intTempSum / Properties.Customs.InternalTemp_Avg_Size;
		}
		
		float externalTemp;
		if (TryGetExternalTemp(externalTemp))
		{
			ExternalTemp = externalTemp;
			// Записать в скользящее окно.
			_extTempSum -= _extTempBuf[_extTempHead];
			_extTempSum += ExternalTemp;
			_extTempBuf[_extTempHead] = ExternalTemp;
			_extTempHead = (_extTempHead + 1) % EXT_AVG_BUF_SZ;
			AverageExternalTemp = _extTempSum / EXT_AVG_BUF_SZ;
			return internalSuccess;
		}
	}
	return false;
}

bool TempSensor::TryGetFirstTemps()
{
	uint8_t scratchpad[9];
	bool internalSuccess = false;
	
	// PS. У датчиков по умолчанию значение температуры = 85 градусов С.
	
	if (OneWire_Send(AllDevicessStartConvert, 2))
	{
		vTaskDelay(_minimumDelayMsec / portTICK_PERIOD_MS);	
    	
		float internalTemp;
		if (TryGetInternalTemp(internalTemp))
		{
			InternalTemp = internalTemp;
			InitAverageInternalTemp(internalTemp); // Заполнить скользящий буфер первым измерением.
			AverageInternalTemp = internalTemp;
			InternalSensorInitialized = true;
		}
		
		float externalTemp;
		if (TryGetExternalTemp(externalTemp))
		{
			ExternalTemp = externalTemp;
			InitAverageExternalTemp(externalTemp); // Заполнить скользящий буфер первым измерением.
			AverageExternalTemp = externalTemp;
			ExternalSensorInitialized = true;
		}
	}
	return InternalSensorInitialized && ExternalSensorInitialized;
}

bool TempSensor::TryGetInternalTemp(float& internalTemp)
{
	uint8_t scratchpad[9];
		
	// Читаем блокнот конкретного устройства.
	if(OneWire_Send(_internalDeviceReadScratchCommand, 10, scratchpad, 9, true))
	{
		// Значение COUNT_PER_°C должно равняться заданной разрядности.
		uint8_t count_per_c = scratchpad[7];
		if (count_per_c == COUNT_PER_C)
		{
			internalTemp = Decode(scratchpad);
			return true;
		}
	}
	return false;
}

bool TempSensor::TryGetExternalTemp(float& externalTemp)
{
	uint8_t scratchpad[9];
	
	if (OneWire_Send(_externalDeviceReadScratchCommand, 10, scratchpad, 9, true))
	{
		uint8_t count_per_c = scratchpad[7];
		if (count_per_c == COUNT_PER_C)
		{
			externalTemp = Decode(scratchpad);
			return true;
		}
	}
	return false;
}

void TempSensor::InitAverageInternalTemp(const float internalTemp)
{
	_intTempSum = internalTemp * Properties.Customs.InternalTemp_Avg_Size;
	for (size_t i = 0; i < Properties.Customs.InternalTemp_Avg_Size; i++)
	{
		_intTempBuf[i] = internalTemp;
	}
	_intTempHead = 0;
}

void TempSensor::InitAverageExternalTemp(const float externalTemp)
{
	_extTempSum = externalTemp * EXT_AVG_BUF_SZ;
	for (size_t i = 0; i < EXT_AVG_BUF_SZ; i++)
	{
		_extTempBuf[i] = externalTemp;
	}
	_extTempHead = 0;
}
	
uint8_t TempSensor::OneWire_Scan(uint8_t* buf, uint8_t num) 
{
	uint8_t found = 0;
	uint8_t* lastDevice;
	uint8_t* curDevice = buf;
	uint8_t numBit, lastCollision, currentCollision, currentSelection;

	const uint8_t searchRomCommand[1] = { SEARCH_ROM };
	
	lastCollision = 0;
	while (found < num) 
	{
		numBit = 1;
		currentCollision = 0;

		// Посылаем команду на поиск устройств.
		OneWire_Send(searchRomCommand, 1, 0, 0);
		
		for (numBit = 1; numBit <= 64; numBit++) 
		{
			// Читаем два бита. Основной и комплементарный.
			OneWire_ToBits(OW_READ_SLOT, _oneWireBuffer);
			OneWire_SendBits(2);

			if (_oneWireBuffer[0] == OW_R_1) 
			{
				if (_oneWireBuffer[1] == OW_R_1) 
				{
					// Две единицы, где-то провтыкали и заканчиваем поиск.
					return found;
				}
				else 
				{
					// 10 - на данном этапе только 1.
					currentSelection = 1;
				}
			}
			else 
			{
				if (_oneWireBuffer[1] == OW_R_1) 
				{
					// 01 - на данном этапе только 0.
					currentSelection = 0;
				}
				else 
				{
					// 00 - коллизия.
					if (numBit < lastCollision) 
					{
						// идем по дереву, не дошли до развилки.
						if (lastDevice[(numBit - 1) >> 3] & 1 << ((numBit - 1) & 0x07)) 
						{
							// (numBit-1)>>3 - номер байта.
							// (numBit-1)&0x07 - номер бита в байте.
							currentSelection = 1;

							// если пошли по правой ветке, запоминаем номер бита.
							if (currentCollision < numBit) 
							{
								currentCollision = numBit;
							}
						}
						else 
						{
							currentSelection = 0;
						}
					}
					else 
					{
						if (numBit == lastCollision) 
						{
							currentSelection = 0;
						}
						else 
						{
							// идем по правой ветке.
							currentSelection = 1;

							// если пошли по правой ветке, запоминаем номер бита.
							if (currentCollision < numBit) 
							{
								currentCollision = numBit;
							}
						}
					}
				}
			}

			if (currentSelection == 1) 
			{
				curDevice[(numBit - 1) >> 3] |= 1 << ((numBit - 1) & 0x07);
				OneWire_ToBits(0x01, _oneWireBuffer);
			}
			else 
			{
				curDevice[(numBit - 1) >> 3] &= ~(1 << ((numBit - 1) & 0x07));
				OneWire_ToBits(0x00, _oneWireBuffer);
			}
			OneWire_SendBits(1);
		}
		found++;
		lastDevice = curDevice;
		curDevice += 8;
		if (currentCollision == 0)
		{
			return found;
		}

		lastCollision = currentCollision;
	}
	return found;
}

void TempSensor::OneWire_SendBits(uint8_t num_bits) 
{
	DMA_DeInit(OW_DMA_CH_RX);
	
	DMA_InitTypeDef dmaInitStructure = 
	{
		.DMA_PeripheralBaseAddr = (uint32_t) &(OneWire_USART->DR),
		.DMA_MemoryBaseAddr = (uint32_t) _oneWireBuffer,
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
	
	DMA_Init(OW_DMA_CH_RX, &dmaInitStructure);
	
	DMA_DeInit(OW_DMA_CH_TX);
	
	dmaInitStructure = 
	{
		.DMA_PeripheralBaseAddr = (uint32_t) &(OneWire_USART->DR),
		.DMA_MemoryBaseAddr = (uint32_t) _oneWireBuffer,
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
	
	DMA_Init(OW_DMA_CH_TX, &dmaInitStructure);

	USART_ClearFlag(OneWire_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
	USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
	DMA_Cmd(OW_DMA_CH_RX, ENABLE);
	DMA_Cmd(OW_DMA_CH_TX, ENABLE);

	while(DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) 
	{
		taskYIELD();
	}

	DMA_Cmd(OW_DMA_CH_TX, DISABLE);
	DMA_Cmd(OW_DMA_CH_RX, DISABLE);
	USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);
}

uint8_t TempSensor::GetDevider(DS18B20_Resolution resolution) 
{
	uint8_t devider;
	switch (resolution) 
	{
	case DS18B20_Resolution_9_bit:
		devider = 8;
		break;
	case DS18B20_Resolution_10_bit:
		devider = 4;
		break;
	case DS18B20_Resolution_11_bit:
		devider = 2;
		break;
	case DS18B20_Resolution_12_bit:
	default:
		devider = 1;
		break;
	}

	return devider;
}
