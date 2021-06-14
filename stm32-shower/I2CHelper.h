#pragma once
#include "FreeRTOS.h"
#include "semphr.h"
#include "Properties.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_gpio.h"
#include "TaskTimeout.h"
#include "task.h"
#include "Common.h"
#include "stm32f10x_rcc.h"

// Всего 8 блоков памяти по 256 байт (по 16 страниц).
// Каждый блок имеет свой уникальный адрес как отдельное устройство,
// в зависимости от битов b3, b2, b1
//
// Используем только 1 блок памяти.
// Используем первый байт, первой страницы для хранения флага 0 или 1
// указывающего по какому адресу находятся данные.
// На хранение 2 блоков данных приходится по 7 страниц или 112 байт

// Адрес первого блока памяти на 256 байт из 8 (для 24—16).
constexpr auto EE_HW_ADDRESS =          0xA0;   // b3 = b2 = b1 = 0
constexpr auto LCD_HW_ADDRESS =         0x7E;
constexpr auto EE_FLASH_PAGESIZE =      16;      // 16-byte Page.
constexpr auto EE_BlockSize =           256;
constexpr auto EE_DataAddr1 =           0x0000 + 16; // Адрес блока в EEPROM где храняться параметры PropertyStruct.
constexpr auto EE_DataAddr2 =           0x0000 + 128;  // Адрес блока в EEPROM где храняться параметры PropertyStruct.
constexpr auto EE_AvailableDataSize = EE_BlockSize / 2 - EE_FLASH_PAGESIZE;   // 112 байт.
static_assert(sizeof(PropertyStruct) <= EE_AvailableDataSize, "size of PropertyStruct struct greater than available in eeprom");

// Синхронизирует доступ к Eeprom и LCD устройствами.
class I2CHelper final
{
public:

    void InitI2C()
    {
        // http://www.freertos.org/xSemaphoreCreateBinaryStatic.html
        m_xLockSemaphore = xSemaphoreCreateBinaryStatic(&m_xLockSemaphoreBuffer);
        xSemaphoreGive(m_xLockSemaphore);
        
        InitGpio();
        GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
        
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
        
        I2C_DeInit(I2C_EE_LCD);
        InitI2cPeriph();
        
        // Включить i2c.
        I2C_Cmd(I2C_EE_LCD, ENABLE);
        I2C_AcknowledgeConfig(I2C1, DISABLE);
        
        while (!WaitFlag(I2C_FLAG_BUSY, kI2CTimeoutMsec))
        {
            ResetBus();
        }
    }
    
    bool EE_BufferWrite(uint8_t* pBuffer, uint8_t write_addr, uint8_t num_bytes_to_write)
    {
        LockI2c();
        bool result = EE_BufferWriteInternal(pBuffer, write_addr, num_bytes_to_write);
        WaitWhileBusy();
        UnlockI2c();
        return result;
    }
        
    bool EE_ByteRead(uint8_t read_addr, uint8_t &data)
    {
        LockI2c();
        bool result = EE_ByteReadInternal(read_addr, data);
        WaitWhileBusy();
        UnlockI2c();
        return result;
    }
    
    bool EE_ByteWrite(uint8_t write_addr, uint8_t data)
    {
        LockI2c();
        bool result = EE_ByteWriteInternal(write_addr, data);
        WaitWhileBusy();
        UnlockI2c();
        return result;
    }
    
    bool LCD_ExpanderWrite(uint8_t data) 
    {
        LockI2c();
        bool result = LCD_InnerExpanderWrite(data);
        WaitWhileBusy();
        UnlockI2c();
        return result;
    }

private:
    
    static constexpr uint16_t kI2CTimeoutMsec = 2000;
    StaticSemaphore_t m_xLockSemaphoreBuffer;
    SemaphoreHandle_t m_xLockSemaphore;
    
    void InitI2cPeriph()
    {   
        I2C_InitTypeDef i2c_init_struct = 
        {
            // 100kHz.
            .I2C_ClockSpeed = 100000, 
            .I2C_Mode = I2C_Mode_I2C,
            .I2C_DutyCycle = I2C_DutyCycle_2,
            .I2C_OwnAddress1 = 0x15,
            .I2C_Ack = I2C_Ack_Enable,
            .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit
        };

        I2C_Init(I2C_EE_LCD, &i2c_init_struct);
    }
    
    void LockI2c()
    {
        xSemaphoreTake(m_xLockSemaphore, portMAX_DELAY);
    }
    
    void UnlockI2c()
    {
        xSemaphoreGive(m_xLockSemaphore);
    }

    // Ожидание окончания записи (Write Cycle Polling using ACK).
    bool I2C_EE_WaitEepromStandbyState()
    {
        TaskTimeout taskTimeout(kI2CTimeoutMsec);
        
        __IO uint16_t SR1_Tmp = 0;

        do
        {
            if (taskTimeout.TimeIsUp())
                return false;
            
            // Send START condition.
            I2C_GenerateSTART(I2C_EE_LCD, ENABLE);

            // Read SR1 register to clear pending flags.
            SR1_Tmp = I2C_ReadRegister(I2C_EE_LCD, I2C_Register_SR1);

            // Send EEPROM address for write.
            I2C_Send7bitAddress(I2C_EE_LCD, EE_HW_ADDRESS, I2C_Direction_Transmitter);
            
        } while (!(I2C_ReadRegister(I2C_EE_LCD, I2C_Register_SR1) & 0x0002));

        // Clear AF flag.
        I2C_ClearFlag(I2C_EE_LCD, I2C_FLAG_AF);

        // STOP condition.
        I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);

        return true;
    }
    
    void InitGpio()
    {
        GPIO_InitTypeDef  GPIO_InitStructure = 
        {
            .GPIO_Pin = GPIO_I2C_SCL_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_AF_OD
        };
        GPIO_Init(GPIOB, &GPIO_InitStructure);
        
        GPIO_InitStructure = 
        {
            .GPIO_Pin = GPIO_I2C_SDA_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_AF_OD	
        };
        GPIO_Init(GPIOB, &GPIO_InitStructure);
    }

    bool WaitFlag(uint32_t i2c_flag, uint16_t timeout_msec)
    {
        TaskTimeout timeout(timeout_msec);
    
        while (I2C_GetFlagStatus(I2C_EE_LCD, i2c_flag))
        {
            if (timeout.TimeIsUp())
            {
                return false;
            }
            else
            {
                taskYIELD();
            }
        }
        return true;
    }
    
    bool WaitFlag(uint32_t i2c_flag)
    {
        TaskTimeout timeout(kI2CTimeoutMsec);
    
        while (I2C_GetFlagStatus(I2C_EE_LCD, i2c_flag))
        {
            if (timeout.TimeIsUp())
            {
                return false;
            }
            else
            {
                taskYIELD();
            }
        }
        return true;
    }
    
    bool WaitEvent(uint32_t i2c_event)
    {
        TaskTimeout timeout(kI2CTimeoutMsec);
    
        while (!I2C_CheckEvent(I2C_EE_LCD, i2c_event))
        {
            if (timeout.TimeIsUp())
            {
                return false;
            }
            else
            {
                taskYIELD();
            }
        }
        
        return true;
    }
    
    void WaitWhileBusy()
    {
        do
        {
            if (WaitFlag(I2C_FLAG_BUSY))
            {
                break;
            }
            else
            {
                // Остановить переключение контекста.
                taskENTER_CRITICAL();
            
                ResetBus();
            
                // Возобновить переключение контекста.
                taskEXIT_CRITICAL();
                
                taskYIELD();
            }
            
        } while (1);
    }
    
    bool I2C_WriteData(uint8_t data)
    {
        I2C_SendData(I2C_EE_LCD, data);
        
        // Test on EV8 and clear it.
        if(!WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        {
            return false;
        }

        return true;
    }
    
    bool LCD_InnerExpanderWrite(uint8_t data) 
    {
        if (!StartTransmission(LCD_HW_ADDRESS))
        {
            return false;
        }
            
        if (!I2C_WriteData(data))
        {
            return false;
        }
        
        I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
        
        return true;
    }
    
    bool StartTransmission(uint8_t hw_address)
    {
        // While the bus is busy.
        if(!WaitFlag(I2C_FLAG_BUSY))
        {
            return false;
        }
        
        // Send STRAT condition.
        I2C_GenerateSTART(I2C_EE_LCD, ENABLE);
        
        // Test on EV5 and clear it.
        if(!WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        {
            return false;
        }
        
        // Send address for write.
        I2C_Send7bitAddress(I2C_EE_LCD, hw_address, I2C_Direction_Transmitter);
        
        // Test on EV6 and clear it.
        if(!WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        {
            return false;
        }
        
        return true;
    }
    
    bool StartReceive(uint8_t hw_address)
    {
        // Send STRAT condition.
        I2C_GenerateSTART(I2C_EE_LCD, ENABLE);
        
        // Test on EV5 and clear it.
        if(!WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        {
            return false;
        }
        
        // Send address for read.
        I2C_Send7bitAddress(I2C_EE_LCD, hw_address, I2C_Direction_Receiver);

        // Test on EV6 and clear it.
        if(!WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
        {
            return false;
        }
        
        return true;
    }
    
    bool EE_ByteReadInternal(uint8_t read_addr, uint8_t &data)
    {
        if (!StartTransmission(EE_HW_ADDRESS))
        {
            return false;
        }
        
        if (!I2C_WriteData(read_addr))
        {
            return false;
        }
        
        if (!StartReceive(EE_HW_ADDRESS))
        {
            return false;
        }
        
        if (!WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            return false;
        }
        
        data = I2C_ReceiveData(I2C_EE_LCD);
        
        I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
        
        return true;
    }
    
    bool EE_BufferWriteInternal(uint8_t* pBuffer, uint8_t write_addr, uint8_t num_bytes_to_write)
    {
        uint8_t NumOfPage = 0;
        uint8_t NumOfSingle = 0;
        uint8_t count = 0;
        uint8_t Addr = 0;

        Addr = write_addr % EE_FLASH_PAGESIZE;
        count = EE_FLASH_PAGESIZE - Addr;
        NumOfPage =  num_bytes_to_write / EE_FLASH_PAGESIZE;
        NumOfSingle = num_bytes_to_write % EE_FLASH_PAGESIZE;

        // If WriteAddr is I2C_FLASH_PAGESIZE aligned.
        if(Addr == 0)
        {
            // If NumByteToWrite < I2C_FLASH_PAGESIZE.
            if(NumOfPage == 0)
            {
                if (!EE_PageWrite(pBuffer, write_addr, NumOfSingle))
                {
                    return false;
                }
                    
                if (!I2C_EE_WaitEepromStandbyState())
                {
                    return false;
                }
            }
            // If NumByteToWrite > I2C_FLASH_PAGESIZE.
            else
            {
                while (NumOfPage--)
                {
                    if (!EE_PageWrite(pBuffer, write_addr, EE_FLASH_PAGESIZE))
                    {
                        return false;
                    }
                    
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                        
                    write_addr +=  EE_FLASH_PAGESIZE;
                    pBuffer += EE_FLASH_PAGESIZE;
                }

                if (NumOfSingle != 0)
                {
                    if (!EE_PageWrite(pBuffer, write_addr, NumOfSingle))
                    {
                        return false;
                    }
                        
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                }
            }
        }
        // If WriteAddr is not I2C_FLASH_PAGESIZE aligned.
        else
        {
            // If NumByteToWrite < I2C_FLASH_PAGESIZE.
            if(NumOfPage == 0)
            {
                // If the number of data to be written is more than the remaining space in the current page:
                if(num_bytes_to_write > count)
                {
                    // Write the data conained in same page.
                    if(!EE_PageWrite(pBuffer, write_addr, count))
                    {
                        return false;
                    }
                        
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }

                    // Write the remaining data in the following page.
                    if(!EE_PageWrite((uint8_t*)(pBuffer + count), (write_addr + count), (num_bytes_to_write - count)))
                    {
                        return false;
                    }
                        
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                }
                else
                {
                    if (!EE_PageWrite(pBuffer, write_addr, NumOfSingle))
                    {
                        return false;
                    }
                    
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                }
            }
            // If NumByteToWrite > I2C_FLASH_PAGESIZE.
            else
            {
                num_bytes_to_write -= count;
                NumOfPage =  num_bytes_to_write / EE_FLASH_PAGESIZE;
                NumOfSingle = num_bytes_to_write % EE_FLASH_PAGESIZE;

                if (count != 0)
                {
                    if (!EE_PageWrite(pBuffer, write_addr, count))
                    {
                        return false;
                    }
                        
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                        
                    write_addr += count;
                    pBuffer += count;
                }

                while (NumOfPage--)
                {
                    if (!EE_PageWrite(pBuffer, write_addr, EE_FLASH_PAGESIZE))
                    {
                        return false;
                    }
                        
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                        
                    write_addr +=  EE_FLASH_PAGESIZE;
                    pBuffer += EE_FLASH_PAGESIZE;
                }
                if (NumOfSingle != 0)
                {
                    if (!EE_PageWrite(pBuffer, write_addr, NumOfSingle))
                    {
                        return false;
                    }
                    
                    if (!I2C_EE_WaitEepromStandbyState())
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }
    
    bool EE_PageWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t num_bytes_to_write)
    {
        if (!StartTransmission(EE_HW_ADDRESS))
        {
            return false;
        }

        if (!I2C_WriteData(WriteAddr))
        {
            return false;
        }

        // While there is data to be written.
        while(num_bytes_to_write--)
        {
            // Send the current byte.
            if(!I2C_WriteData(*pBuffer++))
            {
                return false;
            }
        }

        // Send STOP condition.
        I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
        
        return true;
    }

    bool EE_ByteWriteInternal(uint8_t write_addr, uint8_t data)
    {
        if (!StartTransmission(EE_HW_ADDRESS))
        {
            return false;
        }
        
        if (!I2C_WriteData(write_addr))
        {
            return false;
        }
        
        // Send the byte to be written.
        if(!I2C_WriteData(data))
        {
            return false;
        }

        // Send STOP condition.
        I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
        
        if (!I2C_EE_WaitEepromStandbyState())
        {
            return false;
        }
        
        return true;
    }

    // Формирует сигнал STOP на ногах i2c в ручном режиме.
    void ResetBus()	
    {
        // Starting I2C bus recovery.
        // Try i2c bus recovery at 100kHz = 5uS high, 5uS low.
        
        GPIO_InitTypeDef gpio_init_struct = 
        {
            .GPIO_Pin = GPIO_I2C_SDA_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP
        };
        GPIO_Init(GPIOB, &gpio_init_struct);
        
        GPIO_SetBits(GPIOB, GPIO_I2C_SDA_Pin);

        gpio_init_struct =
        {
            .GPIO_Pin = GPIO_I2C_SCL_Pin,
            .GPIO_Speed = GPIO_Speed_2MHz,
            .GPIO_Mode = GPIO_Mode_Out_PP
        };
        GPIO_Init(GPIOB, &gpio_init_struct);
        
        // 9nth cycle acts as NACK.
        for(int i = 0 ; i < 10 ; i++) 
        { 
            GPIO_SetBits(GPIOB, GPIO_I2C_SCL_Pin);
            DELAY_US(5);
            GPIO_ResetBits(GPIOB, GPIO_I2C_SCL_Pin);
            DELAY_US(5);
        }

        // A STOP signal (SDA from low to high while CLK is high).
        GPIO_ResetBits(GPIOB, GPIO_I2C_SDA_Pin);
        DELAY_US(5);
        GPIO_SetBits(GPIOB, GPIO_I2C_SCL_Pin);
        DELAY_US(2);
        GPIO_SetBits(GPIOB, GPIO_I2C_SDA_Pin);
        DELAY_US(2);
        // Bus status is now : FREE.
        
        // Return to power up mode.
        InitGpio();
    
        I2C_Cmd(I2C_EE_LCD, DISABLE);
        I2C_DeInit(I2C_EE_LCD);
        InitI2cPeriph();
        I2C_Cmd(I2C_EE_LCD, ENABLE);
        I2C_AcknowledgeConfig(I2C1, DISABLE);
    }  
};

extern I2CHelper g_i2cHelper;
