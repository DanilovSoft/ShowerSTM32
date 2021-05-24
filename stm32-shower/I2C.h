#pragma once
#include "FreeRTOS.h"
#include "semphr.h"
#include "Properties.h"

#define I2C_EE_LCD              (I2C1)
#define GPIO_I2C_SCL_Pin        (GPIO_Pin_8)
#define GPIO_I2C_SDA_Pin        (GPIO_Pin_9)
#define I2C_TimeOutMs           (2000)

/*
	Всего 8 блоков памяти по 256 байт (по 16 страниц).
	Каждый блок имеет свой уникальный адрес как отдельное устройство,
	в зависимости от битов b3, b2, b1
	
	Используем только 1 блок памяти.
	Используем первый байт 1 страницы для хранения 0 или 1
	указывающий по какому адресу находятся данные.
	На хранение 2 блоков данных приходится по 7 страниц или 112 байт
*/

/* Адрес первого блока памяти на 256 байт из 8 (для 24—16) */
#define EE_HW_ADDRESS                   (0xA0)   /* b3 = b2 = b1 = 0 */
#define LCD_HW_ADDRESS                  (0x7E)	
#define EE_FLASH_PAGESIZE               (16)      /* 16-byte Page */
#define EE_BlockSize					(256)
#define EE_DataAddr1					(0x0000 + 16)
#define EE_DataAddr2					(0x0000 + 128)
#define EE_AvailableDataSize			(EE_BlockSize / 2 - EE_FLASH_PAGESIZE)   // 112 байт
static_assert(sizeof(PropertyStruct) <= EE_AvailableDataSize, "size of SettingsData struct greater than available in eeprom");

class I2C
{
	StaticSemaphore_t xLockSemaphoreBuffer;
	SemaphoreHandle_t xLockSemaphore;
	
    void LockI2c();
    void UnlockI2c();
    bool I2C_EE_WaitEepromStandbyState(); /* ожидание окончания записи (Write Cycle Polling using ACK) */
    void InitGPIO();
    bool WaitFlag(uint32_t I2C_FLAG, uint16_t timeoutMsec);
    bool WaitFlag(uint32_t I2C_FLAG);
    bool WaitEvent(uint32_t I2C_EVENT);
    void WaitWhileBusy();
    bool I2C_WriteData(uint8_t data);
    bool LCD_expanderWriteInternal(uint8_t data);
    bool StartTransmission(uint8_t HWAddress);
    bool StartReceive(uint8_t HWAddress);
    bool EE_ByteReadInternal(uint8_t ReadAddr, uint8_t &data);
    bool EE_BufferWriteInternal(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite);
    bool EE_PageWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite);
    bool EE_ByteWriteInternal(uint8_t WriteAddr, uint8_t data);
    void ResetBus();    // ‘ормирует сигнал STOP на ногах i2c в ручном режиме
public:
    void vTaskInit();
    void InitI2C();
    bool EE_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite);
    bool EE_ByteRead(uint8_t ReadAddr, uint8_t &data);
    bool EE_ByteWrite(uint8_t WriteAddr, uint8_t data);
    bool LCD_expanderWrite(uint8_t data);
};

extern I2C i2c;
