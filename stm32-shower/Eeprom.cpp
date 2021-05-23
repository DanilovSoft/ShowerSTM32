#include "Eeprom.h"
#include "stm32f10x_crc.h"
#include "stm32f10x_i2c.h"
#include "Properties.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2C.h"

Eeprom _eeprom;

bool Eeprom::EE_SafeBufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead)
{
    while (NumByteToRead--)
    {
	    if (!i2c.EE_ByteRead(ReadAddr, *pBuffer))
	    {
		    return false;
	    }
			
        pBuffer++;
        ReadAddr++;
    }
		
    return true;
}

bool Eeprom::InitProps()
{
    uint8_t dataIndex;
	
	if (!i2c.EE_ByteRead(0, dataIndex))
	{
		return false;
	}
		
    _curPageAddr = dataIndex == 0 ? EE_DataAddr1 : EE_DataAddr2;
		
	if (!EE_SafeBufferRead((uint8_t*)&_writeOnlyPropertiesStruct, _curPageAddr, sizeof(_writeOnlyPropertiesStruct)))
	{
		return false;
	}
		
    return true;
}

bool Eeprom::EE_SafeBufferCRC32(uint8_t ReadAddr, uint8_t NumByteToRead, uint32_t &crc32)
{
    uint8_t buf[4];
    CRC_ResetDR();
		
    for (uint8_t i = 0; i < NumByteToRead; i++)
    {
        uint8_t bufIndex = i % 4;
        uint8_t& bufAddr = buf[bufIndex];
			
        if (!i2c.EE_ByteRead(ReadAddr, bufAddr))
            return false;
			
        ReadAddr++;

        if (bufIndex == 3)
        {
            CRC_CalcCRC(*((uint32_t*)&buf));
        }
    }
		
    crc32 = CRC_GetCRC();
    return true;
}

bool Eeprom::InnerSave()
{	
	// Используем соседнюю половину памяти.
    uint16_t pageAddress = (_curPageAddr == EE_DataAddr1 ? EE_DataAddr2 : EE_DataAddr1);

	if (!i2c.EE_BufferWrite((uint8_t*)&_writeOnlyPropertiesStruct, pageAddress, sizeof(_writeOnlyPropertiesStruct)))
	{
		return false;
	}
		
    uint32_t eeprom_crc32;
	
	if (!EE_SafeBufferCRC32(pageAddress, sizeof(_writeOnlyPropertiesStruct), eeprom_crc32))
	{
		return false;
	}
		
    CRC_ResetDR();

	if (eeprom_crc32 != CRC_CalcBlockCRC((uint32_t*)&_writeOnlyPropertiesStruct, sizeof(_writeOnlyPropertiesStruct) / 4))
	{
		return false;
	}
		
    uint8_t newIndex = _curPageAddr == EE_DataAddr1 ? 1 : 0;

	if (!i2c.EE_ByteWrite(0, newIndex))
	{
		return false;
	}
						
    uint8_t indexTest;
	
	if (!i2c.EE_ByteRead(0, indexTest))
	{
		return false;
	}
							
	if (newIndex != indexTest)
	{
		return false;
	}
								
    _curPageAddr = pageAddress;
    return true;
}

void Eeprom::InitBeforeRTOS()
{
	while (!InitProps())
	{
		// тут недоступен taskYIELD.
	}
		
    _writeOnlyPropertiesStruct.SelfFix();
    Properties = _writeOnlyPropertiesStruct;
}

void Eeprom::Save()
{
    while (!InnerSave())
    {
        taskYIELD();
    }
}
