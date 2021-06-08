#pragma once
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_crc.h"
#include "stm32f10x_flash.h"
#include "Properties.h"

///////////////////////////////////////////////////////
#define PAGE_62              (FLASH_BASE + 62 * 1024)
#define PAGE_63              (FLASH_BASE + 63 * 1024)
///////////////////////////////////////////////////////

extern PropertyStruct WriteProperties;

class Settings
{
public:

	void Init()
	{
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);

		_curPageAddr = PAGE_62;
		if (!ReadData(PAGE_62))		// Контрольная сумма страницы не валидна
		{
			_curPageAddr = PAGE_63;
			ReadData(PAGE_63);
		}

		WriteProperties.SelfFix();
		Properties = WriteProperties;
	}

	void Save()
	{
		uint32_t pageAddress = (_curPageAddr == PAGE_62 ? PAGE_63 : PAGE_62);		// Используем соседнюю страницу

		FLASH_Unlock();
		FLASH_ErasePage(pageAddress);													// Оцищаем соседнюю страницу
		WriteData(pageAddress);																// Записываем данные
		FLASH_ErasePage(_curPageAddr);																// Очищаем старую страницу
		FLASH_Lock();
		_curPageAddr = pageAddress;																	// Обновляем указатель на страницу
	}

private:

	uint32_t _curPageAddr;     // Начальный адрес страницы где хранятся настройки

	// Восстановить данные при инициализации
	static bool ReadData(uint32_t pageAddress)
	{
		uint32_t data;
		uint32_t propsAddr = (uint32_t) & WriteProperties;
		uint16_t dataSizeLeft = sizeof(WriteProperties);
		
		CRC_ResetDR();
		do
		{
			data = FlashRead32(pageAddress);
			CRC_CalcCRC(data);
			*((uint32_t*) propsAddr) = data;
			propsAddr += 4;
			pageAddress += 4;
		} while (dataSizeLeft -= 4);

		data = FlashRead32(pageAddress);
		CRC_CalcCRC(data);
		
		bool valid = CRC_GetCRC() == 0;
		return valid;
	}

	static void WriteData(uint32_t address)
	{	
		uint32_t data;
		uint32_t propsAddr = (uint32_t) & WriteProperties;
		uint16_t dataSizeLeft = sizeof(WriteProperties);
		
		CRC_ResetDR();
		do
		{
			data = *((uint32_t*) propsAddr);
			FLASH_ProgramWord(address, data);
			CRC_CalcCRC(data);
			address += 4;
			propsAddr += 4;
		} while (dataSizeLeft -= 4);
		
		// Сохранение контрольной суммы в конце страницы
		FLASH_ProgramWord(address, CRC_GetCRC());
	}
	
	static inline uint32_t FlashRead32(uint32_t address)
	{
		return (*(__IO uint32_t*) address);
	}
};
