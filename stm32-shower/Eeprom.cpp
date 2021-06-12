#include "Eeprom.h"
#include "stm32f10x_crc.h"
#include "stm32f10x_i2c.h"
#include "Properties.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2C.h"

Eeprom g_eeprom;

bool Eeprom::SafeBufferRead(uint8_t* pBuffer, uint8_t ReadAddr, uint8_t NumByteToRead)
{
    while (NumByteToRead--)
    {
	    if (!g_i2c.EE_ByteRead(ReadAddr, *pBuffer))
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
	
	if (!g_i2c.EE_ByteRead(0, dataIndex))
	{
		return false;
	}
		
    m_curPageAddr = dataIndex == 0 ? EE_DataAddr1 : EE_DataAddr2;
		
	if (!SafeBufferRead((uint8_t*)&g_writeProperties, m_curPageAddr, sizeof(g_writeProperties)))
	{
		return false;
	}
		
    return true;
}

bool Eeprom::SafeBufferCRC32(uint8_t ReadAddr, uint8_t NumByteToRead, uint32_t &crc32)
{
    uint8_t buf[4];
    CRC_ResetDR();
		
    for (uint8_t i = 0; i < NumByteToRead; i++)
    {
        uint8_t bufIndex = i % 4;
        uint8_t& bufAddr = buf[bufIndex];
			
        if (!g_i2c.EE_ByteRead(ReadAddr, bufAddr))
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
    uint16_t pageAddress = (m_curPageAddr == EE_DataAddr1 ? EE_DataAddr2 : EE_DataAddr1);

	if (!g_i2c.EE_BufferWrite((uint8_t*)&g_writeProperties, pageAddress, sizeof(g_writeProperties)))
	{
		return false;
	}
		
    uint32_t eeprom_crc32;
	
	if (!SafeBufferCRC32(pageAddress, sizeof(g_writeProperties), eeprom_crc32))
	{
		return false;
	}
		
    CRC_ResetDR();

	if (eeprom_crc32 != CRC_CalcBlockCRC((uint32_t*)&g_writeProperties, sizeof(g_writeProperties) / 4))
	{
		return false;
	}
		
    uint8_t newIndex = m_curPageAddr == EE_DataAddr1 ? 1 : 0;

	if (!g_i2c.EE_ByteWrite(0, newIndex))
	{
		return false;
	}
						
    uint8_t indexTest;
	
	if (!g_i2c.EE_ByteRead(0, indexTest))
	{
		return false;
	}
							
	if (newIndex != indexTest)
	{
		return false;
	}
								
    m_curPageAddr = pageAddress;
    return true;
}

void Eeprom::InitBeforeRTOS()
{
	while (!InitProps())
	{
		// тут недоступен taskYIELD.
	}
		
    g_writeProperties.SelfFix();
	
    g_properties = g_writeProperties; // Копируем всю структуру.
}

void Eeprom::Save()
{
    while (!InnerSave())
    {
        taskYIELD();
    }
}
