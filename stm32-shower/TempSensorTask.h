#pragma once

/////////////////////////////////////////   Общие коменды   //////////////////////////////////////////////////////////////////
/*

0xF0 —
            Перечисление ID устройств (Поиск ROM (Search ROM))
            Команда выдается управляющим микроконтроллером для определения 
            числа и типа термодатчиков, подключенных к одной линии.

0x33 — 
            Чтение ID единственного подключенного устройства		(Чтение ROM (Read ROM))
            Данная команда инициализирует термодатчик для генерации в линию 
            идентификационного номера. Эту команду нельзя посылать, если к одной линии 
            связи подключено несколько термодатчиков. Прежде чем подключить 
            несколько датчиков на одну линию, необходимо для каждого датчика определить 
            его личный номер с использованием данной команды.

0x55 — 
            Поиск устройства по ID (Идентификация ROM (Match ROM))
            Команда выдается перед 64-битным идентификационным номером и подтверждает 
            обращение именно к этому термодатчику. Все последующие команды будут 
            восприниматься только одним датчиком до команды обнуления линии.

0xCC — 
            Обращение ко всем устройствам (пропуск ID) (Пропуск ROM (Skip ROM))
            Команда может использоваться, когда необходимо обратиться ко всем датчикам, 
            расположенным на одной линии, или когда к линии подключен только один датчик. 
            Общей для многих датчиков может быть команда начала преобразования температуры. 
            При обращении к одному термодатчику команда позволяет 
            упростить программу (следовательно, и время цикла) за счет того, что пропускается 
            громоздкая подпрограмма идентификации кода и вычисления кода четности.

0xEC — 
            Поиск устройств, установивших флаг 'тревога' (Поиск аварии (Alarm Search))
            Действие команды аналогично команде «Поиск ROM», но отвечает на нее термодатчик, 
            если измеренная температура выходит за пределы предварительных установок 
            по максимуму и минимуму.

*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////// Команды DS18S20 //////////////////////////////////////////////////////////////////////
/*

0x44 —
            Начало преобразования температуры (Convert Т)
            Команда разрешает преобразование температуры и запись результата в блокнот.
            От подачи этой команды до считывания необходимо выдержать паузу, 
            необходимую для преобразования с установленной точностью.

0xBE —
            Чтение блокнота (Read Scratchpad)
            В блокноте содержится 8 байт информации. Если нужна информация только о температуре, то 
            считывается 9 бит. Термодатчик будет выдавать информацию до тех пор, пока управляющий 
            микроконтроллер не выдаст в линию нулевой импульс.

0x4E —
            Запись в блокнот (Write Scratchpad)
            После этой команды управляющий микроконтроллер должен послать два байта для записи в блокнот 
            максимальной ТН и минимальной TL температуры ограничения по максимуму и минимуму. 
            Все 16 бит необходимо передавать непрерывно без обнуления линии.

0x48 —
            Копирование блокнота (Copy Scratchpad)
            После этой команды минимальная (TL) и максимальная (ТН) установленные значения температур 
            переписываются в энергонезависимую память (EEPROM). После отключения напряжения 
            питания записанные значения сохранятся в памяти.

0xB8 —
            Восстановление (Recall Е2)
            Эта команда необходима для копирования значений температуры из EEPROM в рабочую зону 
            блокнота. При выполнении восстановления термодатчик выдает в линию низкий 
            уровень, а после окончания записи — высокий.

0xB4 —
            Питание от линии (Read Power Supply)
            После этой команды термодатчик переходит к питанию от линии. 
            В составе термодатчика имеется конденсатор, который заряжается от высокого 
            уровня линии. Перед опросом термодатчика управляющим микроконтроллером 
            необходимо выдержать время, необходимое для заряда конденсатора.
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TaskBase.h"
#include "Properties.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "Common.h"
#include "Properties.h"
#include "Common.h"
#include "string.h"
#include "EepromHelper.h"
#include "InitializationTask.h"

class TempSensorTask final : public TaskBase
{
public:

    TempSensorTask(PropertyStruct* properties)
        : m_properties(properties)
    {
        Debug::Assert(properties != NULL);
        
        // Готовим команду — чтение памяти устройства.
        memcpy(m_internalDeviceReadScratchCommand + 1, properties->InternalTempSensorId, 8);
        memcpy(m_externalDeviceReadScratchCommand + 1, properties->ExternalTempSensorId, 8);
    }
    
    volatile bool InternalSensorInitialized = false;
    volatile bool ExternalSensorInitialized = false;
    volatile bool RegisteringSensors;
    // Усреднённое показание с датчика.
    volatile float AverageInternalTemp = 0;
    // Усреднённое показание с датчика.
    volatile float AverageExternalTemp = 0;
    // Последнее показание с датчика в баке.
    volatile float InternalTemp = 0;
    // Последнее показание с датчика окружающего воздуха.
    volatile float ExternalTemp = 0;
    
    void WaitFirstConversion()
    {
        while (!InternalSensorInitialized || !ExternalSensorInitialized) 
        {
            taskYIELD();
        }
    }

    bool TryGetAirTemp(float& air_temp)
    {
        if (ExternalSensorInitialized)
        {	
            float air_temp = AverageExternalTemp;
            return true;
        }
        return false;
    }
    
private:
    
    typedef enum 
    {
        MATCH_ROM        = 0x55,
        SEARCH_ROM       = 0xF0,
        SKIP_ROM         = 0xCC,
        CONVERT_T        = 0x44,
        READ_SCRATCHPAD  = 0xBE,
        WRITE_SCRATCHPAD = 0x4E,
        TH_REGISTER      = 0x4B,
        TL_REGISTER      = 0x46,
        COPY_SCRATCHPAD  = 0x48,
        RECALL_E2        = 0xB8
    } DS18B20_Commands;

    typedef enum
    {
        DS18B20_Resolution_9_bit  = 0x1F,
        DS18B20_Resolution_10_bit = 0x3F,
        DS18B20_Resolution_11_bit = 0x5F,
        DS18B20_Resolution_12_bit = 0x7F
    } DS18B20_Resolution;
    
    const uint8_t AllDevicessStartConvert[2] = { DS18B20_Commands::SKIP_ROM, DS18B20_Commands::CONVERT_T };
    static constexpr bool kOneWireNoCRC = false;
    static constexpr uint8_t kOneWire0 = 0x00;
    static constexpr uint8_t kOneWire1 = 0xFF;
    static constexpr uint8_t kOneWireR1 = 0xFF;
    static constexpr uint8_t kOneWireNoRead = 0xFF;
    static constexpr uint8_t kOneWireReadSlot = 0xFF;
    static constexpr uint8_t kCountPerC = 16;
    static constexpr uint16_t kMinimumDelayMsec = 200;
    
    PropertyStruct* m_properties;
    
    uint8_t m_internalDeviceReadScratchCommand[10] = { MATCH_ROM, 0, 0, 0, 0, 0, 0, 0, 0, READ_SCRATCHPAD };
    uint8_t m_externalDeviceReadScratchCommand[10] = { MATCH_ROM, 0, 0, 0, 0, 0, 0, 0, 0, READ_SCRATCHPAD };
    
    // Буфер скользящее окно.
    float m_intTempBuf[kInternalTempAvgFilterSize] = { };
    double m_intTempSum = 0;
    uint16_t m_intTempHead = 0;
    float m_extTempBuf[kAirTempAvgFilterSize] = {};
    double m_extTempSum = 0;
    uint16_t m_extTempHead = 0;
    // Сокращенно ow_buf.
    volatile uint8_t m_oneWireBuffer[8] = {};
    
    static void OneWireToBits(uint8_t ow_byte, volatile uint8_t* ow_bits) 
    {
        uint8_t i;
        for (i = 0; i < 8; i++)
        {
            *ow_bits++ = ow_byte & 0x01 ? kOneWire1 : kOneWire0;
            ow_byte >>= 1;
        }
    }

    // Конвертирует массив из 8 бит в 1 байт (каждый элемент массива должен иметь значение только 0 или 1).
    static uint8_t OneWire_ToByte(volatile uint8_t* ow_bits) 
    {
        uint8_t ow_byte = 0;
        for (uint8_t i = 0; i < 8; i++) 
        {
            ow_byte >>= 1;
            if (*ow_bits == kOneWireR1) 
            {
                ow_byte |= 0x80;
            }
            ow_bits++;
        }

        return ow_byte;
    }
    
    static float Decode(uint8_t* scratch_pad)
    {
        // Температуру мы получаем из двух байт: 0-3 биты это значения после запятой,
        // 4-12 собственно значение температуры, 12-16 знак. Если разрешение 12бит, 
        // то биты значащие, если 11бит, то нулевой не используется, если 9бит, 
        // то не используются с 0-2. Если температура отрицательная, то нужно инвертировать 
        // биты, добавить 1 и вычесть из нуля, чтобы получить отрицательное значение.
    
        const uint8_t TEMP_LSB = 0;
        const uint8_t TEMP_MSB = 1;
    
        uint8_t LSB = scratch_pad[TEMP_LSB];
        uint8_t MSB = scratch_pad[TEMP_MSB];
    
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

    void Run()
    {
        Common::AssertAllTasksInitialized();
        
        // Дать немного времени на инициализацию.
        vTaskDelay(10 / portTICK_PERIOD_MS);
    
        TryRegisterNewSensors();
    
        // Готовим команду — чтение памяти устройства.
        memcpy(m_internalDeviceReadScratchCommand + 1, m_properties->InternalTempSensorId, 8);
        memcpy(m_externalDeviceReadScratchCommand + 1, m_properties->ExternalTempSensorId, 8);
    
        SetResolution(DS18B20_Resolution_9_bit, m_properties->InternalTempSensorId);  // Для 9 бит время измерения = 750 msec / 16 = 93.75 msec.
        SetResolution(DS18B20_Resolution_9_bit, m_properties->ExternalTempSensorId);
    
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
    
    bool SetResolution(const DS18B20_Resolution resolution, const uint8_t* device_id)
    {
        uint8_t scratchpad[9] = { };
    
        uint8_t readScratchCommand[10] = { MATCH_ROM };
        readScratchCommand[9] = READ_SCRATCHPAD;
        memcpy(readScratchCommand + 1, device_id, 8);
    
        // Читаем память устройства с проверкой контрольной суммы.
        if(!OneWireSend(readScratchCommand, 10, scratchpad, 9, true))
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
    
        if (!SaveResolution(resolution, device_id))
        {
            return false;
        }
    
        return true;
    }

    bool SaveResolution(const DS18B20_Resolution resolution, const uint8_t* device_id)
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
        
        memcpy(setResolutionCommand + 1, device_id, 8);
        
        if (!OneWireSend(setResolutionCommand, 13))
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
        
        memcpy(copyScratchpadCommand + 1, device_id, 8);

        if (!OneWireSend(copyScratchpadCommand, 10))
        {
            return false;
        }
        
        // Не уверен что это нужно но лучше подождать запись в EEPROM.
        vTaskDelay(20 / portTICK_PERIOD_MS);

        return true;
    }

    // Если датчики заменить на новые то потребуется зарегистрировать их идентификаторы.
    void TryRegisterNewSensors()
    {
        uint8_t one_dev_repeat_count = 0;
        uint8_t two_dev_repeat_count = 0;
        uint8_t new_internal_sensor[8] = { };
        uint8_t new_air_sensor[8] = { };
    
repeat:
    
        uint8_t devices[16] = {};
        uint8_t dev_count = OneWireScan(devices, 2);
    
        // Если в списке только одно устройство и оно новое то зарегистрировать как датчик для бака.
        if(dev_count == 1)
        {
            bool is_internal_sensor = Common::ArrayEquals(devices, 8, m_properties->InternalTempSensorId, 8);
        
            if (!is_internal_sensor)
            {
                bool is_air_sensor = Common::ArrayEquals(devices, 8, m_properties->ExternalTempSensorId, 8);
            
                if (!is_air_sensor)
                {
                    // На шине обнаружено только одно устройство и оно новое.
                    // Значит нужно зарегистрировать его как основной датчик бака.
                    // Но ситуация может быть ошибочной поэтому уведомляем пользователя 
                    // и выдерживаем паузу что-бы пользователь мог прервать перезапись идентификатора.
                
                    if(one_dev_repeat_count == 0)
                    {
                        one_dev_repeat_count++;
                    
                        memcpy(new_internal_sensor, devices, 8);      // Запомним ид устройства что-бы сравнить при повторной попытке.
                    
                        RegisteringSensors = true;
                
                        vTaskDelay(10000 / portTICK_PERIOD_MS);
                    
                        // Повторим поиск устройств что-бы дополнительно убедиться что ситуация не ошибочная.
                        goto repeat;
                    }
                    else
                    {
                        if (!Common::ArrayEquals(devices, 8, new_internal_sensor, 8))
                        {
                            // Идентификатор изменился - что-то пошло не так. Не будем переписывать значение.
                            return;
                        }
                    
                        memcpy(g_writeProperties.InternalTempSensorId, new_internal_sensor, 8);
                
                        if (Common::ArrayEquals(g_writeProperties.InternalTempSensorId, 8, g_writeProperties.ExternalTempSensorId, 8))
                        {
                            memset(g_writeProperties.ExternalTempSensorId, 0, 8);
                        }
                    
                        // Перезаписываем идентификатор датчика.
                        g_eepromHelper->Save();
                    
                        // Пока никто не работает с этим идентификатором можем безопасно перезаписать (не атомарно).
                        // TODO не очень хорошо модифицировать здесь.
                        memcpy(m_properties->InternalTempSensorId, new_internal_sensor, 8);
                    }
                }
            }
        }
        else if(dev_count == 2)
        {
            // Если устройств 2, где один это датчик бака, а другое новое то зарегистрировать второй как датчик окружающего воздуха.

            // Является ли первое устройство в массиве датчиком бака.
            bool firstIsInternal = Common::ArrayEquals(devices, 8, m_properties->InternalTempSensorId, 8);
        
            bool secondIsInternal;
        
            if (firstIsInternal)
            {
                secondIsInternal = false;
            }
            else
            {
                secondIsInternal = Common::ArrayEquals(devices + 8, 8, m_properties->InternalTempSensorId, 8);
            }
        
            if (firstIsInternal || secondIsInternal)
            {
                uint8_t* nextDev = firstIsInternal ? devices + 8 : devices;
            
                if (!Common::ArrayEquals(nextDev, 8, m_properties->ExternalTempSensorId, 8))
                {
                    // Обнаружен новый датчик. Предупредим пользователя, выдержим паузу, перечитаем идентификатор и зарегистрируем его как датчик воздуха.
                
                    if(two_dev_repeat_count == 0)
                    {
                        two_dev_repeat_count++;
                    
                        memcpy(new_air_sensor, nextDev, 8);         // Запомним ид устройства что-бы сравнить при повторной попытке.
                    
                        RegisteringSensors = true;
                
                        vTaskDelay(10000 / portTICK_PERIOD_MS);
                    
                        // Повторим поиск устройств что-бы дополнительно убедиться что ситуация не ошибочная.
                        goto repeat;
                    }
                    else
                    {
                        if (!Common::ArrayEquals(nextDev, 8, new_air_sensor, 8))
                        {
                            // Идентификатор изменился - что-то пошло не так. Не будем переписывать значение.
                            return;
                        }
                    
                        memcpy(g_writeProperties.ExternalTempSensorId, new_air_sensor, 8);
                
                        // Перезаписываем идентификатор датчика.
                        g_eepromHelper->Save();
                    
                        // Пока никто не работает с этим идентификатором можем безопасно перезаписать (не атомарно).
                        memcpy(m_properties->ExternalTempSensorId, new_air_sensor, 8);
                    }
                }
            }
        }
    }

    // Отправляет команду 0xF0.
    bool OneWireReset() 
    {
        USART_InitTypeDef init_struct =
        {
            .USART_BaudRate = 9600,
            .USART_WordLength = USART_WordLength_8b,
            .USART_StopBits = USART_StopBits_1,
            .USART_Parity = USART_Parity_No,
            .USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
            .USART_HardwareFlowControl = USART_HardwareFlowControl_None
        };
        USART_Init(OneWire_USART, &init_struct);

        // Отправляем 0xF0 на скорости 9600
        USART_ClearFlag(OneWire_USART, USART_FLAG_TC);
        USART_SendData(OneWire_USART, 0xF0);
    
        while (USART_GetFlagStatus(OneWire_USART, USART_FLAG_TC) == RESET) 
        {
            taskYIELD();
        }

        uint8_t ow_presence = USART_ReceiveData(OneWire_USART);

        init_struct = 
        {
            .USART_BaudRate = 115200,
            .USART_WordLength = USART_WordLength_8b,
            .USART_StopBits = USART_StopBits_1,
            .USART_Parity = USART_Parity_No,
            .USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
            .USART_HardwareFlowControl = USART_HardwareFlowControl_None
        };
        USART_Init(OneWire_USART, &init_struct);

        if (ow_presence != 0xF0) 
        {
            return true;
        }
        return false;
    }

    bool ValidateCrc32(uint8_t* data, uint8_t length)
    {
        uint8_t crc = 0;
        while (length--)
        {
            CalcCrc32_ibutton_update(crc, *data++);
        }
        
        return crc == 0;
    }

    // Polynomial: x^8 + x^5 + x^4 + 1 (0x8C)
    void CalcCrc32_ibutton_update(uint8_t &crc, uint8_t data)
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

    //-----------------------------------------------------------------------------
    // Процедура общения с шиной 1-wire.
    // Выполняет Reset, затем отправляет команду, затем читает 8 бит (по факту 8 байт) в глобальный
    // буфер _oneWireBuffer. Затем, опционально, копирует полученный байт в массив data.
    // command - массив байт, отсылаемых в шину
    // cLen - длина строки command, столько байт отошлётся в шину.
    // data - если требуется чтение, то ссылка на буфер для чтения, иначе 0.
    // dLen - длина буфера для чтения. Прочитается не более этой длины. Фактически сколько раз будет повторяться вся процедура.
    //-----------------------------------------------------------------------------
    bool OneWireSend(const uint8_t* command, uint8_t cLen, uint8_t* data = 0, uint8_t dLen = 0, bool calcCrc = true) 
    {	
        if (OneWireReset())
        {
            uint8_t* dataStart = data;
            const uint8_t dataLen = dLen;
        
            uint8_t readStart = dLen ? cLen : 255;
            cLen += dLen;
            
            while (cLen--) 
            {
                uint8_t byte = cLen < dLen ? 0xFF : *command++;
                OneWireToBits(byte, m_oneWireBuffer);

                // Сбросить читающий канал DMA на значения по умолчанию, перед установкой новых значений (опционально).
                DMA_DeInit(OW_DMA_CH_RX);
            
                DMA_InitTypeDef dmaInitStructure = 
                {
                    .DMA_PeripheralBaseAddr = (uint32_t)&(OneWire_USART->DR),
                    .DMA_MemoryBaseAddr = (uint32_t)m_oneWireBuffer,
                    .DMA_DIR = DMA_DIR_PeripheralSRC,
                    .DMA_BufferSize = 8,
                       // 8 байт которые интерпретируем как 8 бит.
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
                    .DMA_MemoryBaseAddr = (uint32_t)m_oneWireBuffer,
                    .DMA_DIR = DMA_DIR_PeripheralDST,
                    .DMA_BufferSize = 8, 
                     // 8 байт которые интерпретируем как 8 бит.
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
                USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);  // Запустили UART.
                DMA_Cmd(OW_DMA_CH_RX, ENABLE);  // Запустили DMA на чтение.
                DMA_Cmd(OW_DMA_CH_TX, ENABLE);  // Запустили DMA на запись.

                // Ждем, пока не примем 8 байт (которые сконвертируем в 8 бит).
                while(DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) 
                {
                    taskYIELD();
                }

                // Отключаем DMA.
                DMA_Cmd(OW_DMA_CH_TX, DISABLE);
                DMA_Cmd(OW_DMA_CH_RX, DISABLE);
                USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);

                // Если прочитанные данные кому-то нужны - запишем их в выходной буфер data.
                if(readStart != 255)
                {
                    if (readStart == 0 && dLen > 0) 
                    {
                        *data++ = OneWire_ToByte(m_oneWireBuffer);
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

    void Pause()
    {
        vTaskDelay(kTempSensorPauseMsec / portTICK_PERIOD_MS);
    }

    // Если датчик не подключен к шине UART, scratchpad будет заполнен значениями 0 из-за подтягивающего резистора.
    // В этом случае контрольная сумма будет пройдена успешно, а расчет температуры будет не верным.
    // Что бы это предотвратить следует проверять значение COUNT_PER_°C которое не должно равняться нулю.
    bool TryUpdateTemp()
    {
        uint8_t scratchpad[9];
        bool internal_success = false;
        
        if (OneWireSend(AllDevicessStartConvert, 2))
        {
            vTaskDelay(kMinimumDelayMsec / portTICK_PERIOD_MS);	
        
            float internal_temp;
            if (TryGetInternalTemp(internal_temp))
            {
                InternalTemp = internal_temp;
            
                // Записать в скользящее окно.
                m_intTempSum -= m_intTempBuf[m_intTempHead];
                m_intTempSum += InternalTemp;
                m_intTempBuf[m_intTempHead] = InternalTemp;
                m_intTempHead = (m_intTempHead + 1) % m_properties->InternalTempAvgFilterSize;
                AverageInternalTemp = m_intTempSum / m_properties->InternalTempAvgFilterSize;
            }
        
            float air_temp;
            if (TryGetExternalTemp(air_temp))
            {
                ExternalTemp = air_temp;
                // Записать в скользящее окно.
                m_extTempSum -= m_extTempBuf[m_extTempHead];
                m_extTempSum += ExternalTemp;
                m_extTempBuf[m_extTempHead] = ExternalTemp;
                m_extTempHead = (m_extTempHead + 1) % kAirTempAvgFilterSize;
                AverageExternalTemp = m_extTempSum / kAirTempAvgFilterSize;
                return internal_success;
            }
        }
        return false;
    }

    bool TryGetFirstTemps()
    {
        uint8_t scratchpad[9];
        bool internalSuccess = false;
    
        // PS. У датчиков по умолчанию значение температуры = 85 градусов С.
    
        if(OneWireSend(AllDevicessStartConvert, 2))
        {
            vTaskDelay(kMinimumDelayMsec / portTICK_PERIOD_MS);	
        
            float internal_temp;
            if (TryGetInternalTemp(internal_temp))
            {
                InternalTemp = internal_temp;
                InitAverageInternalTemp(internal_temp);  // Заполнить скользящий буфер первым измерением.
                AverageInternalTemp = internal_temp;
                InternalSensorInitialized = true;
            }
        
            float externalTemp;
            if (TryGetExternalTemp(externalTemp))
            {
                ExternalTemp = externalTemp;
                InitAverageExternalTemp(externalTemp);  // Заполнить скользящий буфер первым измерением.
                AverageExternalTemp = externalTemp;
                ExternalSensorInitialized = true;
            }
        }
        return InternalSensorInitialized && ExternalSensorInitialized;
    }

    bool TryGetInternalTemp(float& internal_temp)
    {
        uint8_t scratchpad[9];
        
        // Читаем блокнот конкретного устройства.
        if(OneWireSend(m_internalDeviceReadScratchCommand, 10, scratchpad, 9, true))
        {
            // Значение COUNT_PER_°C должно равняться заданной разрядности.
            uint8_t count_per_c = scratchpad[7];
            if (count_per_c == kCountPerC)
            {
                internal_temp = Decode(scratchpad);
                return true;
            }
        }
        return false;
    }

    bool TryGetExternalTemp(float& air_temp)
    {
        uint8_t scratchpad[9];
    
        if (OneWireSend(m_externalDeviceReadScratchCommand, 10, scratchpad, 9, true))
        {
            uint8_t count_per_c = scratchpad[7];
            if (count_per_c == kCountPerC)
            {
                air_temp = Decode(scratchpad);
                return true;
            }
        }
        return false;
    }

    // Заполняет весь скользящий буфер одним значением.
    void InitAverageInternalTemp(const float internal_temp)
    {
        m_intTempSum = internal_temp * m_properties->InternalTempAvgFilterSize;
        for (size_t i = 0; i < m_properties->InternalTempAvgFilterSize; i++)
        {
            m_intTempBuf[i] = internal_temp;
        }
        m_intTempHead = 0;
    }

    // Заполняет весь скользящий буфер одним значением.
    void InitAverageExternalTemp(const float external_temp)
    {
        m_extTempSum = external_temp * kAirTempAvgFilterSize;
        for (size_t i = 0; i < kAirTempAvgFilterSize; i++)
        {
            m_extTempBuf[i] = external_temp;
        }
        m_extTempHead = 0;
    }
    
    // Данная функция осуществляет сканирование сети 1-wire и записывает найденные
    // ID устройств в массив buf, по 8 байт на каждое устройство.
    // @param num Переменная num ограничивает количество находимых устройств, чтобы не переполнить буфер.
    uint8_t OneWireScan(uint8_t* devices, uint8_t num) 
    {
        uint8_t found = 0;
        uint8_t* last_device;
        uint8_t* cur_device = devices;
        uint8_t num_bit, last_collision, current_collision, current_selection;

        static constexpr uint8_t search_rom_command[1] = { SEARCH_ROM };
    
        last_collision = 0;
        while (found < num) 
        {
            num_bit = 1;
            current_collision = 0;

            // Посылаем команду на поиск устройств.
            OneWireSend(search_rom_command, 1, 0, 0);
        
            for (num_bit = 1; num_bit <= 64; num_bit++) 
            {
                // Читаем два бита. Основной и комплементарный.
                OneWireToBits(kOneWireReadSlot, m_oneWireBuffer);
                OneWireSendBits(2);

                if (m_oneWireBuffer[0] == kOneWireR1) 
                {
                    if (m_oneWireBuffer[1] == kOneWireR1) 
                    {
                        // Две единицы, где-то провтыкали и заканчиваем поиск.
                        return found;
                    }
                    else 
                    {
                        // 10 - на данном этапе только 1.
                        current_selection = 1;
                    }
                }
                else 
                {
                    if (m_oneWireBuffer[1] == kOneWireR1) 
                    {
                        // 01 - на данном этапе только 0.
                        current_selection = 0;
                    }
                    else 
                    {
                        // 00 - коллизия.
                        if(num_bit < last_collision) 
                        {
                            // идем по дереву, не дошли до развилки.
                            if(last_device[(num_bit - 1) >> 3] & 1 << ((num_bit - 1) & 0x07)) 
                            {
                                // (numBit-1)>>3 - номер байта.
                                // (numBit-1)&0x07 - номер бита в байте.
                                current_selection = 1;

                                // если пошли по правой ветке, запоминаем номер бита.
                                if(current_collision < num_bit) 
                                {
                                    current_collision = num_bit;
                                }
                            }
                            else 
                            {
                                current_selection = 0;
                            }
                        }
                        else 
                        {
                            if (num_bit == last_collision) 
                            {
                                current_selection = 0;
                            }
                            else 
                            {
                                // идем по правой ветке.
                                current_selection = 1;

                                // если пошли по правой ветке, запоминаем номер бита.
                                if(current_collision < num_bit) 
                                {
                                    current_collision = num_bit;
                                }
                            }
                        }
                    }
                }

                if (current_selection == 1) 
                {
                    cur_device[(num_bit - 1) >> 3] |= 1 << ((num_bit - 1) & 0x07);
                    OneWireToBits(0x01, m_oneWireBuffer);
                }
                else 
                {
                    cur_device[(num_bit - 1) >> 3] &= ~(1 << ((num_bit - 1) & 0x07));
                    OneWireToBits(0x00, m_oneWireBuffer);
                }
                OneWireSendBits(1);
            }
            found++;
            last_device = cur_device;
            cur_device += 8;
            if (current_collision == 0)
            {
                return found;
            }

            last_collision = current_collision;
        }
        return found;
    }

    void OneWireSendBits(uint8_t num_bits) 
    {
        DMA_DeInit(OW_DMA_CH_RX);
    
        DMA_InitTypeDef dmaInitStructure = 
        {
            .DMA_PeripheralBaseAddr = (uint32_t) &(OneWire_USART->DR),
            .DMA_MemoryBaseAddr = (uint32_t) m_oneWireBuffer,
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
            .DMA_MemoryBaseAddr = (uint32_t) m_oneWireBuffer,
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

        while (DMA_GetFlagStatus(OW_DMA_FLAG) == RESET) 
        {
            taskYIELD();
        }

        DMA_Cmd(OW_DMA_CH_TX, DISABLE);
        DMA_Cmd(OW_DMA_CH_RX, DISABLE);
        USART_DMACmd(OneWire_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);
    }

    uint8_t GetDevider(DS18B20_Resolution resolution) 
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
};

extern TempSensorTask* g_tempSensorTask;
