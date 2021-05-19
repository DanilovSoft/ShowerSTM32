#include "I2C.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_gpio.h"
#include "Properties.h"
#include "Timeout.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "Common.h"

I2C i2c;

void I2C::LockI2c()
{
	xSemaphoreTake(xLockSemaphore, portMAX_DELAY);
}
	

void I2C::UnlockI2c()
{
	xSemaphoreGive(xLockSemaphore);
}
    

/* Ожидание окончания записи (Write Cycle Polling using ACK) */
bool I2C::I2C_EE_WaitEepromStandbyState()
{
	TaskTimeout timeout(I2C_TimeOutMs);
		
	__IO uint16_t SR1_Tmp = 0;

	do
	{
		if (timeout.TimeIsUp())
			return false;
			
        /* Send START condition */
		I2C_GenerateSTART(I2C_EE_LCD, ENABLE);

		/* Read SR1 register to clear pending flags */
		SR1_Tmp = I2C_ReadRegister(I2C_EE_LCD, I2C_Register_SR1);

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C_EE_LCD, EE_HW_ADDRESS, I2C_Direction_Transmitter);
			
	} while (!(I2C_ReadRegister(I2C_EE_LCD, I2C_Register_SR1) & 0x0002));

	/* Clear AF flag */
	I2C_ClearFlag(I2C_EE_LCD, I2C_FLAG_AF);

	/* STOP condition */
	I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);

	return true;
}
	

void I2C::InitGPIO()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
		
	GPIO_InitStructure.GPIO_Pin =  GPIO_I2C_SCL_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
    	
	GPIO_InitStructure.GPIO_Pin =  GPIO_I2C_SDA_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}
    

bool I2C::WaitFlag(uint32_t I2C_FLAG, uint16_t timeoutMsec)
{
	TaskTimeout timeout(timeoutMsec);
	while (I2C_GetFlagStatus(I2C_EE_LCD, I2C_FLAG))
	{
		if (timeout.TimeIsUp())
			return false;
		else
			taskYIELD();
	}
	return true;
}
    

bool I2C::WaitFlag(uint32_t I2C_FLAG)
{
	TaskTimeout timeout(I2C_TimeOutMs);
	while (I2C_GetFlagStatus(I2C_EE_LCD, I2C_FLAG))
	{
		if (timeout.TimeIsUp())
			return false;
		else
			taskYIELD();
	}
	return true;
}
    

bool I2C::WaitEvent(uint32_t I2C_EVENT)
{
	TaskTimeout timeout(I2C_TimeOutMs);
	while (!I2C_CheckEvent(I2C_EE_LCD, I2C_EVENT))
	{
		if (timeout.TimeIsUp())
			return false;
		else
			taskYIELD();
	}
		
	return true;
}
    

void I2C::WaitWhileBusy()
{
	do
	{
		if (WaitFlag(I2C_FLAG_BUSY))
		{
			break;
		}
		else
		{
		    /* Остановить переключение контекста */
			taskENTER_CRITICAL();
			
			//break;
			ResetBus();
    		
			/* Возобновить переключение контекста */
			taskEXIT_CRITICAL();
                
			taskYIELD();
		}
    		
	} while (1);
}
    

bool I2C::I2C_WriteData(uint8_t data)
{
	I2C_SendData(I2C_EE_LCD, data);
		
	/* Test on EV8 and clear it */
	if (!WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
		return false;

	return true;
}
	

bool I2C::LCD_expanderWriteInternal(uint8_t data) 
{
	if (!StartTransmission(LCD_HW_ADDRESS))
		return false;
			
	if (!I2C_WriteData(data))
		return false;
		
	I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
		
	return true;
}
    

bool I2C::StartTransmission(uint8_t HWAddress)
{
	/* While the bus is busy */
	if (!WaitFlag(I2C_FLAG_BUSY))
		return false;
    	
	/* Send STRAT condition */
	I2C_GenerateSTART(I2C_EE_LCD, ENABLE);
		
	/* Test on EV5 and clear it */
	if (!WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
		return false;
		
    /* Send address for write */
	I2C_Send7bitAddress(I2C_EE_LCD, HWAddress, I2C_Direction_Transmitter);
    	
	/* Test on EV6 and clear it */
	if (!WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		return false;
		
	return true;
}
    

bool I2C::StartReceive(uint8_t HWAddress)
{
	/* Send STRAT condition */
	I2C_GenerateSTART(I2C_EE_LCD, ENABLE);
		
	/* Test on EV5 and clear it */
	if (!WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
		return false;
		
    /* Send address for read */
	I2C_Send7bitAddress(I2C_EE_LCD, HWAddress, I2C_Direction_Receiver);

	/* Test on EV6 and clear it */
	if (!WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		return false;
		
	return true;
}
    

bool I2C::EE_ByteReadInternal(uint8_t ReadAddr, uint8_t &data)
{
	if (!StartTransmission(EE_HW_ADDRESS))
		return false;
		
	if (!I2C_WriteData(ReadAddr))
		return false;
		
	if (!StartReceive(EE_HW_ADDRESS))
		return false;
		
	if (!WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED))
		return false;
		
	data = I2C_ReceiveData(I2C_EE_LCD);
		
	I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
		
	return true;
}
	

bool I2C::EE_BufferWriteInternal(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite)
{
	uint8_t NumOfPage = 0;
	uint8_t NumOfSingle = 0;
	uint8_t count = 0;
	uint8_t Addr = 0;

	Addr = WriteAddr % EE_FLASH_PAGESIZE;
	count = EE_FLASH_PAGESIZE - Addr;
	NumOfPage =  NumByteToWrite / EE_FLASH_PAGESIZE;
	NumOfSingle = NumByteToWrite % EE_FLASH_PAGESIZE;

	/* If WriteAddr is I2C_FLASH_PAGESIZE aligned  */
	if (Addr == 0)
	{
	    /* If NumByteToWrite < I2C_FLASH_PAGESIZE */
		if (NumOfPage == 0)
		{
			if (!EE_PageWrite(pBuffer, WriteAddr, NumOfSingle))
				return false;
					
			if (!I2C_EE_WaitEepromStandbyState())
				return false;
		}
		/* If NumByteToWrite > I2C_FLASH_PAGESIZE */
		else
		{
			while (NumOfPage--)
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, EE_FLASH_PAGESIZE))
					return false;
					
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
						
				WriteAddr +=  EE_FLASH_PAGESIZE;
				pBuffer += EE_FLASH_PAGESIZE;
			}

			if (NumOfSingle != 0)
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, NumOfSingle))
					return false;
						
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
			}
		}
	}
	/* If WriteAddr is not I2C_FLASH_PAGESIZE aligned  */
	else
	{
	    /* If NumByteToWrite < I2C_FLASH_PAGESIZE */
		if (NumOfPage == 0)
		{
		    /* If the number of data to be written is more than the remaining space in the current page: */
			if (NumByteToWrite > count)
			{
			    /* Write the data conained in same page */
				if (!EE_PageWrite(pBuffer, WriteAddr, count))
					return false;
						
				if (!I2C_EE_WaitEepromStandbyState())
					return false;

				/* Write the remaining data in the following page */
				if (!EE_PageWrite((uint8_t*)(pBuffer + count), (WriteAddr + count), (NumByteToWrite - count)))
					return false;
						
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
			}
			else
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, NumOfSingle))
					return false;
					
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
			}
		}
		/* If NumByteToWrite > I2C_FLASH_PAGESIZE */
		else
		{
			NumByteToWrite -= count;
			NumOfPage =  NumByteToWrite / EE_FLASH_PAGESIZE;
			NumOfSingle = NumByteToWrite % EE_FLASH_PAGESIZE;

			if (count != 0)
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, count))
					return false;
						
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
						
				WriteAddr += count;
				pBuffer += count;
			}

			while (NumOfPage--)
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, EE_FLASH_PAGESIZE))
					return false;
						
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
						
				WriteAddr +=  EE_FLASH_PAGESIZE;
				pBuffer += EE_FLASH_PAGESIZE;
			}
			if (NumOfSingle != 0)
			{
				if (!EE_PageWrite(pBuffer, WriteAddr, NumOfSingle))
					return false;
					
				if (!I2C_EE_WaitEepromStandbyState())
					return false;
			}
		}
	}

	return true;
}
	

bool I2C::EE_PageWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite)
{
	if (!StartTransmission(EE_HW_ADDRESS))
		return false;

	if (!I2C_WriteData(WriteAddr))
		return false;

	/* While there is data to be written */
	while (NumByteToWrite--)
	{
		/* Send the current byte */
		if (!I2C_WriteData(*pBuffer++))
			return false;
	}

	/* Send STOP condition */
	I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
		
	return true;
}


bool I2C::EE_ByteWriteInternal(uint8_t WriteAddr, uint8_t data)
{
	if (!StartTransmission(EE_HW_ADDRESS))
		return false;
		
	if (!I2C_WriteData(WriteAddr))
		return false;
		
    /* Send the byte to be written */
	if (!I2C_WriteData(data))
		return false;

	/* Send STOP condition */
	I2C_GenerateSTOP(I2C_EE_LCD, ENABLE);
		
	if (!I2C_EE_WaitEepromStandbyState())
		return false;
		
	return true;
}
	

// Формирует сигнал STOP на ногах i2c в ручном режиме
void I2C::ResetBus()	
{
	// Starting I2C bus recovery
	//try i2c bus recovery at 100kHz = 5uS high, 5uS low
    	
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  GPIO_I2C_SDA_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	GPIO_SetBits(GPIOB, GPIO_I2C_SDA_Pin);

	GPIO_InitStructure.GPIO_Pin =  GPIO_I2C_SCL_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
    	
	//9nth cycle acts as NACK
	for (int i = 0; i < 10; i++) 
	{ 
		GPIO_SetBits(GPIOB, GPIO_I2C_SCL_Pin);
		Delay_us(5);
		GPIO_ResetBits(GPIOB, GPIO_I2C_SCL_Pin);
		Delay_us(5);
	}

	//a STOP signal (SDA from low to high while CLK is high)
	GPIO_ResetBits(GPIOB, GPIO_I2C_SDA_Pin);
	Delay_us(5);
	GPIO_SetBits(GPIOB, GPIO_I2C_SCL_Pin);
	Delay_us(2);
	GPIO_SetBits(GPIOB, GPIO_I2C_SDA_Pin);
	Delay_us(2);
	//bus status is now : FREE
    	
	// Return to power up mode
	InitGPIO();
    
	I2C_Cmd(I2C_EE_LCD, DISABLE);
	I2C_DeInit(I2C_EE_LCD);
	InitI2C();
	I2C_Cmd(I2C_EE_LCD, ENABLE);
	I2C_AcknowledgeConfig(I2C1, DISABLE);
}
	

void I2C::vTaskInit()
{
	/* http://www.freertos.org/xSemaphoreCreateBinaryStatic.html */
	xLockSemaphore = xSemaphoreCreateBinaryStatic(&xLockSemaphoreBuffer);
	xSemaphoreGive(xLockSemaphore);
    	
	InitGPIO();
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
		
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
		
	I2C_DeInit(I2C_EE_LCD);
	InitI2C();
    	
	/* Включить i2c */
	I2C_Cmd(I2C_EE_LCD, ENABLE);
	I2C_AcknowledgeConfig(I2C1, DISABLE);
		
	while (!WaitFlag(I2C_FLAG_BUSY, I2C_TimeOutMs))
	{
		ResetBus();
	}
}
	

void I2C::InitI2C()
{
	I2C_InitTypeDef  I2C_InitStructure;
	I2C_StructInit(&I2C_InitStructure);
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0x15;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 100000;  /* 100kHz */
	I2C_Init(I2C_EE_LCD, &I2C_InitStructure);
}
    

bool I2C::EE_BufferWrite(uint8_t* pBuffer, uint8_t WriteAddr, uint8_t NumByteToWrite)
{
	LockI2c();
	bool result = EE_BufferWriteInternal(pBuffer, WriteAddr, NumByteToWrite);
	WaitWhileBusy();
	UnlockI2c();
	return result;
}
    

bool I2C::EE_ByteRead(uint8_t ReadAddr, uint8_t &data)
{
	LockI2c();
	bool result = EE_ByteReadInternal(ReadAddr, data);
	WaitWhileBusy();
	UnlockI2c();
	return result;
}
	

bool I2C::EE_ByteWrite(uint8_t WriteAddr, uint8_t data)
{
	LockI2c();
	bool result = EE_ByteWriteInternal(WriteAddr, data);
	WaitWhileBusy();
	UnlockI2c();
	return result;
}
	

bool I2C::LCD_expanderWrite(uint8_t data) 
{
	LockI2c();
	bool result = LCD_expanderWriteInternal(data);
	WaitWhileBusy();
	UnlockI2c();
	return result;
}