#pragma once
#include "stdint.h"
#include "math.h"
#include "MedianFilter.h"

#define WL_BUF_MAX_SIZE         (255)
constexpr auto InternalTempLimit = (42); // максимальная температура воды в баке
const uint8_t LOWER_BOUND = 15; // минимальна¤ температура на улице 
const uint8_t UPPER_BOUND = 40; // максимальна¤ температура на улице
#define STEPS_COUNT             (UPPER_BOUND - LOWER_BOUND)

struct TempStep
{
	// Температура в баке
	uint8_t Internal[STEPS_COUNT];
	
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
		return Internal[index];
	}
	
	uint8_t GetLimit(float extTemp)
	{
		uint8_t index = GetIndexf(extTemp);
		return Internal[index];
	}
	
	// Принимает температуру окружающего воздуха в грудусах, что-бы
	// увеличить температуру при такой температуре окружающей среды.
	bool TempPlus(uint8_t extTemp)
	{
		const uint8_t index = GetIndex(extTemp);	
		uint8_t value = Internal[index];

    	if (value < InternalTempLimit)
        // значение в допустимом пределе
		{
    		++value;
    		
			Internal[index] = value;
    		
    		// Сделать точки слева не меньше текущего значения. массив идет от большего к меньшему: [40][40][40][39][38][37][37][37][36][36]
            if(index > 0)
    		{
        		uint8_t leftPoints = index;
                	
        		while (leftPoints--)
        		{
            		auto& t = Internal[leftPoints];
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
        		auto& t = Internal[i];
                	
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
    	
		uint8_t value = Internal[index];
    	
    	if (value > 0) // проверка на вс¤кий случай
    	{
        	--value; // уменьшить на 1 градус
    	
        	if(value <= InternalTempLimit)
            // значение в допустимом пределе
        	{
            	Internal[index] = value; // установить новое значение
            	
            	// сделать точки справа не больше текущего значени¤.  [40][40][40][37][37][35][37][37][37][37]
            	for(uint8_t i = (index + 1); i < STEPS_COUNT; i++) // от следующей точки к концу массива
            	{
                	auto& t = Internal[i];
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
                    	auto& t = Internal[leftPoints];
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
		uint8_t prev = Internal[0];
		for (uint8_t i = 0; i < STEPS_COUNT; i++)
		{
			uint8_t value = data[i];
			
			if (i > 0)
			{
				if (value > prev)
					value = prev;
			}
			
			Internal[i] = value;
			prev = value;
		}
	}
	
	void SelfTest()
	{
		uint8_t prevSpot = Internal[0];
		for (int i = 0; i < STEPS_COUNT; i++)
		{
			uint8_t& t = Internal[i];

    		if (t == 0 || t > InternalTempLimit)
			{
				t = 36;
			}
			
			if (i > 0)
			{
				if (t > prevSpot)
				{
					t = prevSpot;
				}
			}
			
			prevSpot = t;
		}
	}
};

struct SettingsData
{
	// Температурная зависимость.
	TempStep Chart;
	
	// Максимальный уровень воды в микросекундах.
	uint16_t WaterLevelFull;
	
	// Минимальный уровень воды в микросекундах.
	uint16_t WaterLevelEmpty;

	// Минимальный уровень воды в баке (%) для включения нагревателя.
	uint8_t MinimumWaterHeatingPercent;
	
	// Безусловное максимальное время нагрева ч.
	uint8_t AbsoluteHeatingTimeLimitHours;
	
	// Максимальное время нагрева в минутах.
	uint8_t HeatingTimeLimitMin;
	
	// Яркость освещения от 0 до 100 (%).
	uint8_t LightBrightness;

	// Мощность WiFi в единицах по 0.25 dBm.
	// От 40 до 82 (10..20.5 dBm)
	uint8_t WiFiPower;
	
	// Минимальное время зажатой кнопки для её срабатывания.
    uint8_t ButtonPressTimeMs = 40;
    
	uint16_t ButtonLongPressTimeMs = 3000;
    
	// 64-битный идентификатор датчика температуры внутри бака. (DS18B20)
	uint8_t InternalTempSensorId[8] = {};
	
	// 64-битный идентификатор датчика температуры окружающего воздуха. (DS18B20)
	uint8_t ExternalTempSensorId[8] = {};
	
	void SelfFix()
	{
		Chart.SelfTest();
		
		// В одном сантиметре - 58 микросекунд.
		if(WaterLevelEmpty < (30 * 58) || WaterLevelEmpty > (50 * 58)) // 30..50 сантиметров (1740..2900 мкс)
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
			WiFiPower = 56;   // 14 dBm
		}
    	
		Customs.SelfTest();
	}
	
	struct CustomsTypeDef
	{
		// Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
		uint8_t WaterLevel_Measure_IntervalMsec;
		
		// Размер скользящего окна.
		uint8_t WaterLevel_Ring_Buffer_Size;
		
		// Порог отключения клапана набора воды.
		uint8_t WaterValve_Cut_Off_Percent;
		
		// Размер скользящего окна для температуры внутри бака.
		uint8_t TempSensor_InternalTemp_Buffer_Size;
		
		// Поправка скорости звука на температуру воздуха.
		uint16_t WaterLevel_Usec_Per_Deg;
		
		// Максимальное время набора воды, если выше уровня 'Cut-Off' в секундах.
		uint8_t WaterValve_TimeoutSec;
		
    	uint8_t SetWaterLevel_Ring_Buffer_Size(uint8_t value)
    	{
        	if (value > 0)
        	{	
	        	if (value > WL_BUF_MAX_SIZE)
	        	{
		        	value = WL_BUF_MAX_SIZE;
	        	}
        	
            	WaterLevel_Ring_Buffer_Size = value;
        	}
        	
        	return WaterLevel_Ring_Buffer_Size;
    	}
    	
		void SelfTest()
		{
			if (WaterLevel_Measure_IntervalMsec < 10 || WaterLevel_Measure_IntervalMsec > 255)
			{
				WaterLevel_Measure_IntervalMsec = 100;
			}
		
    		if (WaterLevel_Ring_Buffer_Size == 0 || WaterLevel_Ring_Buffer_Size > WL_BUF_MAX_SIZE)
			{
				WaterLevel_Ring_Buffer_Size = 32;
			}
			
			if (WaterValve_Cut_Off_Percent < 90 || WaterValve_Cut_Off_Percent > 99)
			{
				WaterValve_Cut_Off_Percent = 95;
			}
			
			if (TempSensor_InternalTemp_Buffer_Size == 0 || TempSensor_InternalTemp_Buffer_Size > 8)
			{
				TempSensor_InternalTemp_Buffer_Size = 1;
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
	
} __attribute__((aligned(16)));		// Размер структуры должен быть кратен 4 для удобства подсчета CRC32 или 16 для удобства хранения в EEPROM.

extern SettingsData Properties, _writeOnlyPropertiesStruct;