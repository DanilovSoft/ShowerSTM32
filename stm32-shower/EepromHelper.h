#pragma once
#include "stm32f10x_crc.h"
#include "stm32f10x_i2c.h"
#include "Properties.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2CHelper.h"
#include "Common.h"
#include "Debug.h"
#include "PropertyWrapper.h"

class EepromHelper final
{	
public:

    // Полезно использовать для отладки когда EEPROM не подключен
    void InitDefaults(PropertyWrapper& out_properties)
    {
        g_writeProperties.SelfFix();
    
        // Копируем всю структуру.
        out_properties = g_writeProperties;
    }
    
    void ReadProperties(PropertyWrapper& out_properties)
    {
        Debug::Assert(g_i2cHelper.IsInitialized());
        
        while (!TryInitProperties())
        {
            taskYIELD();
        }
        
        g_writeProperties.SelfFix();
    
        // Копируем всю структуру.
        out_properties = g_writeProperties;
    }

    // Сохраняет в EEPROM значение структуры g_writeProperties.
    void Save()
    {
        while (!InnerSave())
        {
            taskYIELD();
        }
    }
    
private:
    
    uint8_t m_curPageAddr;
    
    bool SafeBufferRead(uint8_t* buffer, uint8_t read_addr, uint8_t num_bytes_to_read)
    {
        while (num_bytes_to_read--)
        {
            if (!g_i2cHelper.EE_ByteRead(read_addr, *buffer))
            {
                return false;
            }
            
            buffer++;
            read_addr++;
        }
        
        return true;
    }

    bool TryInitProperties()
    {
        uint8_t data_index;
    
        if (!g_i2cHelper.EE_ByteRead(0, data_index))
        {
            return false;
        }
        
        m_curPageAddr = data_index == 0 ? EE_DataAddr1 : EE_DataAddr2;
        
        if (!SafeBufferRead((uint8_t*)&g_writeProperties, m_curPageAddr, sizeof(g_writeProperties)))
        {
            return false;
        }
        
        return true;
    }

    bool SafeBufferCrc32(uint8_t read_addr, uint8_t num_bytes_to_read, uint32_t& crc32)
    {
        uint8_t crc32_buffer[4];
        CRC_ResetDR();
        
        for (uint8_t i = 0; i < num_bytes_to_read; i++)
        {
            uint8_t bufIndex = i % 4;
            uint8_t& bufAddr = crc32_buffer[bufIndex];
            
            if (!g_i2cHelper.EE_ByteRead(read_addr, bufAddr))
            {
                return false;
            }
            
            read_addr++;

            if (bufIndex == 3)
            {
                CRC_CalcCRC(*((uint32_t*)&crc32_buffer));
            }
        }
        
        crc32 = CRC_GetCRC();
        return true;
    }

    bool InnerSave()
    {	
        // Используем соседнюю половину памяти.
        uint16_t page_address = (m_curPageAddr == EE_DataAddr1 ? EE_DataAddr2 : EE_DataAddr1);

        if (!g_i2cHelper.EE_BufferWrite((uint8_t*)&g_writeProperties, page_address, sizeof(g_writeProperties)))
        {
            return false;
        }
        
        uint32_t eeprom_crc32;
    
        if (!SafeBufferCrc32(page_address, sizeof(g_writeProperties), eeprom_crc32))
        {
            return false;
        }
        
        CRC_ResetDR();

        if (eeprom_crc32 != CRC_CalcBlockCRC((uint32_t*)&g_writeProperties, sizeof(g_writeProperties) / 4))
        {
            return false;
        }
        
        uint8_t new_index = m_curPageAddr == EE_DataAddr1 ? 1 : 0;

        if (!g_i2cHelper.EE_ByteWrite(0, new_index))
        {
            return false;
        }
                        
        uint8_t index_test;
    
        if (!g_i2cHelper.EE_ByteRead(0, index_test))
        {
            return false;
        }
                            
        if (new_index != index_test)
        {
            return false;
        }
                                
        m_curPageAddr = page_address;
        return true;
    }
};

extern EepromHelper g_eepromHelper;
