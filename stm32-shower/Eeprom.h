#pragma once
#include "stdint.h"

class Eeprom final
{	
public:

	// Выполняется перед запуском диспетчера потоков.
	void InitBeforeRTOS();
	
	// Сохраняет в EEPROM значение структуры WriteProperties.
	void Save();
	
private:
	
	uint8_t m_curPageAddr;
	bool SafeBufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead);
	bool InitProps();
	bool SafeBufferCRC32(uint8_t ReadAddr, uint8_t NumByteToRead, uint32_t &crc32);
	bool InnerSave();
};

extern Eeprom g_eeprom;
