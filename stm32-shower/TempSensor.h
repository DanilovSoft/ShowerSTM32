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

#include "iActiveTask.h"
#include "stdint.h"

typedef enum 
{
	MATCH_ROM        = 0x55,
	SEARCH_ROM		 = 0xF0,
	SKIP_ROM         = 0xCC,
	CONVERT_T        = 0x44,
	READ_SCRATCHPAD  = 0xBE,
	WRITE_SCRATCHPAD = 0x4E,
	TH_REGISTER      = 0x4B,
	TL_REGISTER      = 0x46,
	COPY_SCRATCHPAD  = 0x48,
	RECALL_E2		 = 0xB8
} COMMANDS;

#define OneWire_USART            (USART1)
#define OneWire_GPIO             (GPIOB)
#define OW_GPIO_Pin_Tx      (GPIO_Pin_6)
#define OW_DMA_CH_RX        (DMA1_Channel5)
#define OW_DMA_CH_TX        (DMA1_Channel4)
#define OW_DMA_FLAG         (DMA1_FLAG_TC5)
#define OW_NO_CRC           (false)
#define OW_0                (0x00)
#define OW_1                (0xFF)
#define OW_R_1              (0xFF)
#define OW_NO_READ			(0xFF)
#define OW_READ_SLOT		(0xFF)
#define OW_SEND_RESET		(1)
#define OW_NO_RESET			(2)
#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)
#define BIT_IS_SET(var,pos) ((var) & (1<<(pos)))
#define BIT_IS_NOT_SET(var,pos) (!BIT_IS_SET(var,pos))
// Команды SKIP_ROM, CONVERT_T.
constexpr uint8_t AllDevicessStartConvert[] = { SKIP_ROM, CONVERT_T };
#define COUNT_PER_C     (16)


typedef enum
{
	DS18B20_Resolution_9_bit  = 0x1F,
	DS18B20_Resolution_10_bit = 0x3F,
	DS18B20_Resolution_11_bit = 0x5F,
	DS18B20_Resolution_12_bit = 0x7F
} DS18B20_Resolution;

class TempSensor : public iActiveTask
{
private:
	
    const static uint8_t EXT_BUF_SZ = 1;
    const uint16_t _minimumDelayMsec = 200;
	
	uint8_t InternalDeviceReadScratchCommand[10] = { MATCH_ROM, 0, 0, 0, 0, 0, 0, 0, 0, READ_SCRATCHPAD };
	uint8_t ExternalDeviceReadScratchCommand[10] = { MATCH_ROM, 0, 0, 0, 0, 0, 0, 0, 0, READ_SCRATCHPAD };
	
    // Буфер скользящее окно.
    float _intTempBuf[255] = {};
    double _intTempSum = 0;
    uint16_t _intTempHead = 0;
    //TickType_t _xLastWakeTime;
    float _extTempBuf[EXT_BUF_SZ] = {};
    double _extTempSum = 0;
    uint16_t _extTempHead = 0;
    
	void Run();
	void Init();
	static void OneWire_ToBits(uint8_t ow_byte, volatile uint8_t* ow_bits);

	// Конвертирует массив из 8 бит в 1 байт (каждый элемент массива должен иметь значение только 0 или 1).
	static uint8_t OneWire_ToByte(volatile uint8_t* ow_bits);
	static float Decode(uint8_t* scratchPad);
	
	// Отправляет команду 0xF0.
	bool OneWire_Reset();
	bool ValidateCrc32(uint8_t* data, uint8_t length);
	void CalcCrc32_ibutton_update(uint8_t &crc, uint8_t data); // Polynomial: x^8 + x^5 + x^4 + 1 (0x8C)
	//-----------------------------------------------------------------------------
	// Процедура общения с шиной 1-wire.
	// Выполняет Reset, затем отправляет команду, затем читает 8 бит (по факту 8 байт) в глобальный
	// буфер _oneWireBuffer. Затем, опционально, копирует полученный байт в массив data.
	// command - массив байт, отсылаемых в шину
	// cLen - длина строки command, столько байт отошлётся в шину.
	// data - если требуется чтение, то ссылка на буфер для чтения, иначе 0.
	// dLen - длина буфера для чтения. Прочитается не более этой длины. Фактически сколько раз будет повторяться вся процедура.
	//-----------------------------------------------------------------------------
	bool OneWire_Send(const uint8_t* command, uint8_t cLen, uint8_t* data = 0, uint8_t dLen = 0, bool calcCrc = true);
	float Decode_old(uint8_t* scratchPad);
	void ReadRom();
	// Если датчик не подключен к шине UART, scratchpad будет заполнен значениями 0 из-за подтягивающего резистора.
	// В этом случае контрольная сумма будет пройдена успешно, а расчет температуры будет не верным.
	// Что бы это предотвратить следует проверять значение COUNT_PER_°C которое не должно равняться нулю.
	bool TryUpdateTemp();
	
	bool TryGetFirstTemps();
	
	bool TryGetInternalTemp(float& internalTemp);
	bool TryGetExternalTemp(float& externalTemp);
	
	// Заполняет весь скользящий буфер одним значением.
	void InitAverageInternalTemp(const float internalTemp);
	void InitAverageExternalTemp(const float externalTemp);
    void Setup();
    void Pause();
	//-----------------------------------------------------------------------------
	// Данная функция осуществляет сканирование сети 1-wire и записывает найденные
	//   ID устройств в массив buf, по 8 байт на каждое устройство.
	// переменная num ограничивает количество находимых устройств, чтобы не переполнить
	// буфер.
	//-----------------------------------------------------------------------------
	uint8_t OneWire_Scan(uint8_t* buf, uint8_t num);
	void OneWire_SendBits(uint8_t num_bits);

	// Если датчики заменить на новые то потребуется зарегистрировать их идентификаторы.
	void TryRegisterNewSensors();
	
	uint8_t GetDevider(DS18B20_Resolution resolution);
	bool SetResolution(DS18B20_Resolution resolution, uint8_t* deviceId);
	bool SaveResolution(DS18B20_Resolution resolution, uint8_t* deviceId);

public:

	volatile bool InternalSensorInitialized = false;
	volatile bool ExternalSensorInitialized = false;
	// Усредненое показание с датчика.
	volatile float AverageInternalTemp = 0;
	// Усредненое показание с датчика.
	volatile float AverageExternalTemp = 0;
	// Последнее показание с датчика.
	volatile float InternalTemp = 0;
	// Последнее показание с датчика.
	volatile float ExternalTemp = 0;
	void WaitFirstConversion();
	volatile bool RegisteringSensors;
};

extern TempSensor _tempSensorTask;

