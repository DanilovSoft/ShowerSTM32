#pragma once
#include "stdint.h"

class Eeprom
{	
	uint8_t _curPageAddr;
	bool EE_SafeBufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead);
	bool InitProps();
	bool EE_SafeBufferCRC32(uint8_t ReadAddr, uint8_t NumByteToRead, uint32_t &crc32);
	bool InnerSave();
public:

	// Выполняется перед запуском диспетчера потоков.
	void InitBeforeRTOS();
	
	// Сохраняет в EEPROM значение структуры _writeOnlyPropertiesStruct.
	void Save();
};

extern Eeprom _eeprom;
