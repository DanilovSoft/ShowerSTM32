#pragma once
#include "stdint.h"
#include "math.h"
#include "MedianFilter.h"
#include "TempSensor.h"

#define WL_AVG_BUF_MAX_SIZE         (129) // Максимально допустимый размер фильтра 'скользящее среднее' для уровня воды.
constexpr auto InternalTempLimit = (42); // максимальная температура воды в баке.
const uint8_t LOWER_BOUND = 15; // Минимальная температура на улице.
const uint8_t UPPER_BOUND = 40; // Максимальная температура на улице.
#define STEPS_COUNT     (UPPER_BOUND - LOWER_BOUND) // Размер таблицы температур делаем исходя из возможных значений температур окружаюшего воздуха.

typedef uint8_t byte;
typedef uint16_t ushort;

struct TempStep
{
private:
	
	// Температура в баке
	uint8_t _internal[STEPS_COUNT];
	
	uint8_t GetIndex(uint8_t externalTemp)
	{
		if (externalTemp <= LOWER_BOUND)
			return 0;
		
		if (externalTemp >= (UPPER_BOUND - 1))
			return STEPS_COUNT - 1;
		
		return externalTemp - LOWER_BOUND;
	}
	
	uint8_t GetIndexf(float externalTemp)
	{
		uint8_t t = round(externalTemp);
		return GetIndex(t);
	}
	
public:
	
	uint8_t GetLimit(uint8_t extTemp)
	{
		uint8_t index = GetIndex(extTemp);
		return _internal[index];
	}
	
	uint8_t GetLimit(float extTemp)
	{
		uint8_t index = GetIndexf(extTemp);
		return _internal[index];
	}
	
	// Принимает температуру окружающего воздуха в грудусах, что-бы
	// увеличить температуру при такой температуре окружающей среды.
	bool TempPlus(uint8_t extTemp)
	{
		const uint8_t index = GetIndex(extTemp);	
		uint8_t value = _internal[index];

    	if (value < InternalTempLimit)
        // значение в допустимом пределе
		{
    		++value;
    		
			_internal[index] = value;
    		
    		// Сделать точки слева не меньше текущего значения. массив идет от большего к меньшему: [40][40][40][39][38][37][37][37][36][36]
            if(index > 0)
    		{
        		uint8_t leftPoints = index;
                	
        		while (leftPoints--)
        		{
            		auto& t = _internal[leftPoints];
	        		if (t < value)
	        		{
		        		t = value;
	        		}
        		}
    		}
    		
    		uint8_t prevValue = value;
            	
    		// сделать точки справа с шагом не больше 1 градуса
    		for(uint8_t i = (index + 1); i < STEPS_COUNT ; i++) // от текущей точки не включительно к началу массива
    		{
        		auto& t = _internal[i];
                	
        		auto nextT = (prevValue - 1);   // не должно быть меньше
                	
        		if(t < nextT)
	    		{
		    		t = nextT;
	    		}
        		
        		prevValue = t;
    		}
			return true;
		}
		return false;
	}
	
	// Принимает температуру окружающего воздуха в грудусах, что-бы
	// уменьшить температуру при такой температуре окружающей среды.
	bool TempMinus(uint8_t extTemp)
	{
		const uint8_t index = GetIndex(extTemp);	
    	
		uint8_t value = _internal[index];
    	
    	if (value > 0) // проверка на вс¤кий случай
    	{
        	--value; // уменьшить на 1 градус
    	
        	if(value <= InternalTempLimit)
            // значение в допустимом пределе
        	{
            	_internal[index] = value; // установить новое значение
            	
            	// сделать точки справа не больше текущего значени¤.  [40][40][40][37][37][35][37][37][37][37]
            	for(uint8_t i = (index + 1); i < STEPS_COUNT; i++) // от следующей точки к концу массива
            	{
                	auto& t = _internal[i];
                	if (t > value)
                    	t = value;
            	}
            	
            	if (index > 0)
            	{
            	    auto prevValue = value;
                	uint8_t leftPoints = index;
                	
                	// сделать точки слева с шагом не больше 1 градуса
                	while (leftPoints--)
                	{
                    	auto& t = _internal[leftPoints];
                    	auto nextT = (prevValue + 1);      // не должно превышать
                	
                    	if(t > nextT)
                        	t = nextT;
                    	
                    	prevValue = t;
                	}
            	}
            	return true;
        	}
    	}
		return false;
	}
	
	void Parse(const uint8_t* data)
	{
		uint8_t prev = _internal[0];
		for (uint8_t i = 0; i < STEPS_COUNT; i++)
		{
			uint8_t value = data[i];
			
			if (i > 0)
			{
				if (value > prev)
				{
					value = prev;
				}
			}
			
			_internal[i] = value;
			prev = value;
		}
	}
	
	void SelfTest()
	{
		uint8_t prevSpot = _internal[0];
		for (int i = 0; i < STEPS_COUNT; i++)
		{
			uint8_t& internalTemp = _internal[i];

    		if (internalTemp == 0 || internalTemp > InternalTempLimit)
			{
				internalTemp = 36;
			}
			
			if (i > 0)
			{
				if (internalTemp > prevSpot)
				{
					internalTemp = prevSpot;
				}
			}
			
			prevSpot = internalTemp;
		}
	}
};

struct PropertyStruct
{
	// Температурная зависимость.
	TempStep Chart;
	
	// Максимальный уровень воды в микросекундах.
	ushort WaterLevelFull;
	
	// Минимальный уровень воды в микросекундах.
	ushort WaterLevelEmpty;

	// Минимальный уровень воды в баке (%) для включения нагревателя.
	byte MinimumWaterHeatingPercent;
	
	// Абсолютное время нагрева (ч).
	// Если на протяжении заданного времени автомат нагревателя не был 
	// физически выключен, то нагрев прекращается и алгоритм переходит в аварийное состояние.
	byte AbsoluteHeatingTimeLimitHours;
	
	// Максимальное время нагрева в минутах.
	// Если на протяжении заданного времени не была достигнута желаемая температура воды в баке, 
	// то нагрев прекращается и алгоритм переходит в аварийное состояние.
	byte HeatingTimeLimitMin;
	
	// Яркость освещения от 0 до 100 (%).
	byte LightBrightness;

	// Мощность WiFi в единицах по 0.25 dBm.
	// От 40 до 82 (10..20.5 dBm)
	uint8_t WiFiPower;
	
	// Минимальное время зажатой кнопки для её срабатывания.
    uint8_t ButtonPressTimeMs = 40;
    
	// Минимальное время зажатой кнопки для срабатывания длительного нажатия.
	uint16_t ButtonLongPressTimeMs = 3000;
    
	// 64-битный идентификатор датчика температуры внутри бака. (DS18B20)
	uint8_t InternalTempSensorId[8] = {};
	
	// 64-битный идентификатор датчика температуры окружающего воздуха. (DS18B20)
	byte ExternalTempSensorId[8] = { };
	
	// Объём воды полного бака в литрах.
	float WaterTankVolumeLitre;
	
    // Электрическая мощность нагревательного элемента — ТЭНа, кВТ.
    float WaterHeaterPowerKWatt;
	
	void SelfFix()
	{
		Chart.SelfTest();
		
		// В одном сантиметре - 58 микросекунд.
		if (WaterLevelEmpty < (30 * 58) || WaterLevelEmpty > (50 * 58)) // 30..50 сантиметров (1740..2900 мкс)
		{
			WaterLevelEmpty = 2618;   // 45 см
		}

		if (WaterLevelFull < 116 || WaterLevelFull > WaterLevelEmpty)     // 2 сантиметра.
		{
			WaterLevelFull = 922;   // 16 см
		}

		if (MinimumWaterHeatingPercent > 100)
		{
			MinimumWaterHeatingPercent = 25;
		}
		
		if (HeatingTimeLimitMin < 10 || HeatingTimeLimitMin > 180)
		{
			HeatingTimeLimitMin = 70;
		}
		
		if (LightBrightness > 100)
		{
			LightBrightness = 100;
		}

		if (AbsoluteHeatingTimeLimitHours > 24)
		{
			AbsoluteHeatingTimeLimitHours = 6;
		}
		
		// от 10 до 20.5 dBm.
		if (WiFiPower < 40 || WiFiPower > 82)
		{
			WiFiPower = 56;   // 56 = 14.0 dBm
		}
    	
		if (WaterTankVolumeLitre < 0 || WaterTankVolumeLitre > 100 || isnan(WaterTankVolumeLitre))
		{
			WaterTankVolumeLitre = 37.32212;
		}
		
		if (WaterHeaterPowerKWatt < 0 || WaterHeaterPowerKWatt > 3 || isnan(WaterHeaterPowerKWatt))
		{
			WaterHeaterPowerKWatt = 1.38624; // Полтора-киловатный ТЭН с учётом КПД.
		}
		
		Customs.SelfTest();
	}
	
	// TODO зачем нужна эта структура?
	struct CustomsTypeDef
	{
		// Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
		uint8_t WaterLevel_Measure_IntervalMsec;
		
		// Размер буфера медианного фильтра для сырых показаний датчика уровня воды.
		uint8_t WaterLevel_Median_Buffer_Size;
		
		// Размер буфера для усреднения показаний после медианного фильтра.
		uint8_t WaterLevel_Avg_Buffer_Size;
		
		// Порог отключения клапана набора воды.
		uint8_t WaterValve_Cut_Off_Percent;
		
		// Размер скользящего окна для температуры внутри бака.
		uint8_t InternalTemp_Avg_Size;
		
		// Поправка скорости звука на температуру воздуха.
		uint16_t WaterLevel_Usec_Per_Deg;
		
		// Максимальное время набора воды, если выше уровня 'Cut-Off' в секундах.
		uint8_t WaterValve_TimeoutSec;
    	
		void SelfTest()
		{
			if (WaterLevel_Measure_IntervalMsec < 10 || WaterLevel_Measure_IntervalMsec > 255)
			{
				WaterLevel_Measure_IntervalMsec = 100;
			}
		
			if (WaterLevel_Median_Buffer_Size == 0 || WaterLevel_Median_Buffer_Size > WL_MEDIAN_BUF_MAX_SIZE)
			{
				WaterLevel_Median_Buffer_Size = 33; // Лучше не чётное число.
			}
			
    		if (WaterLevel_Avg_Buffer_Size == 0 || WaterLevel_Avg_Buffer_Size > WL_AVG_BUF_MAX_SIZE)
			{
				WaterLevel_Avg_Buffer_Size = 8;
			}
			
			if (WaterValve_Cut_Off_Percent < 90 || WaterValve_Cut_Off_Percent > 99)
			{
				WaterValve_Cut_Off_Percent = 95;
			}
			
			if (InternalTemp_Avg_Size == 0 || InternalTemp_Avg_Size > INT_AVG_BUF_SZ)
			{
				InternalTemp_Avg_Size = 1;
			}
			
			if (WaterLevel_Usec_Per_Deg > 10000)
			{
				WaterLevel_Usec_Per_Deg = 0;
			}
			
			if (WaterValve_TimeoutSec < 5)
			{
				WaterValve_TimeoutSec = 5;	
			}
		}
	} Customs;
	
} __attribute__((aligned(16)));		// Размер структуры должен быть кратен 4 для удобства подсчета CRC32 или 16 для удобства хранения в странице EEPROM.

extern PropertyStruct Properties, _writeOnlyPropertiesStruct;