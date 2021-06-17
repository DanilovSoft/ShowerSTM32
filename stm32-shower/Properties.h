#pragma once
#include "math.h"
#include "Common.h"

struct TempStep
{
public:

    uint8_t GetLimit(uint8_t ext_temp)
    {
        uint8_t index = GetIndex(ext_temp);
        return m_internal[index];
    }

    uint8_t GetLimit(float ext_temp)
    {
        uint8_t index = GetIndexf(ext_temp);
        return m_internal[index];
    }

    // Принимает температуру окружающего воздуха в грудусах, что-бы
    // увеличить температуру при такой температуре окружающей среды.
    bool TempPlus(const uint8_t air_temp)
    {
        const uint8_t index = GetIndex(air_temp);
        uint8_t value = m_internal[index];

        if (value < kInternalTempLimit)
        {
            // Значение в допустимом пределе.
                
            ++value;

            m_internal[index] = value;

            // Сделать точки слева не меньше текущего значения. Массив идет от большего к меньшему: [40, 40, 40, 39, 38, 37, 37, 37, 36, 36].
            if(index > 0)
            {
                uint8_t left_points = index;

                while (left_points--)
                {
                    auto& t = m_internal[left_points];
                    if (t < value)
                    {
                        t = value;
                    }
                }
            }

            uint8_t prev_value = value;

            // Сделать точки справа с шагом не больше 1 градуса.
            for(uint8_t i = (index + 1) ; i < kAirTempSteps ; i++) // От текущей точки не включительно к началу массива.
            {
                auto& t = m_internal[i];

                auto nextT = (prev_value - 1);     // Не должно быть меньше.

                if(t < nextT)
                {
                    t = nextT;
                }

                prev_value = t;
            }
            return true;
        }
        return false;
    }

    // Принимает температуру окружающего воздуха в грудусах, что-бы
    // уменьшить температуру при такой температуре окружающей среды.
    bool TempMinus(const uint8_t air_temp)
    {
        const uint8_t index = GetIndex(air_temp);

        uint8_t value = m_internal[index];

        // проверка на всякий случай.
        if(value > 0)
        {
            --value;     // Уменьшить на 1 градус.

            if(value <= kInternalTempLimit)
            {
                // Значение в допустимом пределе.
                    
                m_internal[index] = value;     // Установить новое значение.

                // Сделать точки справа не больше текущего значени¤.  [40, 40, 40, 39, 38, 37, 37, 37, 36, 36].
                for(uint8_t i = (index + 1) ; i < kAirTempSteps ; i++) // От следующей точки к концу массива.
                {
                    auto& t = m_internal[i];
                    if (t > value)
                    {
                        t = value;
                    }
                }

                if (index > 0)
                {
                    auto prev_value = value;
                    uint8_t left_points = index;

                    // Сделать точки слева с шагом не больше 1 градуса.
                    while(left_points--)
                    {
                        auto& t = m_internal[left_points];
                        auto nextT = (prev_value + 1);   // Не должно превышать.

                        if(t > nextT)
                        {
                            t = nextT;
                        }

                        prev_value = t;
                    }
                }
                return true;
            }
        }
        return false;
    }

    void Parse(const uint8_t* data)
    {
        uint8_t prev = m_internal[0];
        
        for (uint8_t i = 0; i < kAirTempSteps; i++)
        {
            uint8_t value = data[i];

            if (i > 0)
            {
                if (value > prev)
                {
                    value = prev;
                }
            }

            m_internal[i] = value;
            prev = value;
        }
    }

    void SelfFix()
    {
        uint8_t prevSpot = m_internal[0];
        
        for (int i = 0; i < kAirTempSteps; i++)
        {
            uint8_t& internal_temp = m_internal[i];

            if (internal_temp == 0 || internal_temp > kInternalTempLimit)
            {
                internal_temp = 36;
            }

            if (i > 0)
            {
                if (internal_temp > prevSpot)
                {
                    internal_temp = prevSpot;
                }
            }

            prevSpot = internal_temp;
        }
    }
    
private:
    
    // Температура в баке.
    uint8_t m_internal[kAirTempSteps];
    
    uint8_t GetIndex(uint8_t externalTemp)
    {
        if (externalTemp <= kAirTempLowerBound)
        {
            return 0;
        }
        
        if (externalTemp >= (kAirTempUpperBound - 1))
        {
            return kAirTempSteps - 1;
        }
        
        return externalTemp - kAirTempLowerBound;
    }
    
    uint8_t GetIndexf(float externalTemp)
    {
        uint8_t t = round(externalTemp);
        return GetIndex(t);
    }
};

struct PropertyStruct
{
public:
    
    // Температурная зависимость.
    TempStep Chart;
    
    // Максимальный уровень воды в микросекундах.
    uint16_t WaterLevelFull;
    
    // Минимальный уровень воды в микросекундах.
    uint16_t WaterLevelEmpty;
    
    // Минимальный уровень воды в баке (%) для включения нагревателя.
    uint8_t MinimumWaterHeatingPercent;
    
    // Абсолютное время нагрева (ч).
    // Если на протяжении заданного времени автомат нагревателя не был 
    // физически выключен, то нагрев прекращается и алгоритм переходит в аварийное состояние.
    uint8_t AbsoluteHeatingTimeLimitHours;
    
    // Максимальное время нагрева в минутах.
    // Если на протяжении заданного времени не была достигнута желаемая температура воды в баке, 
    // то нагрев прекращается и алгоритм переходит в аварийное состояние.
    uint8_t HeatingTimeLimitMin;
    
    // Яркость освещения от 0 до 100 (%).
    uint8_t LightBrightness;

    // Мощность WiFi в единицах по 0.25 dBm.
    // От 40 до 82 (10..20.5 dBm)
    uint8_t WiFiPower;
    
    // Минимальное время зажатой кнопки для её срабатывания (антидребезг).
    uint8_t ButtonPressTimeMsec;
    
    // Минимальное время зажатой кнопки для срабатывания длительного нажатия.
    uint16_t ButtonLongPressTimeMsec;
    
    // 64-битный идентификатор датчика температуры воды внутри бака. (DS18B20)
    uint8_t InternalTempSensorId[8] = {};
    
    // 64-битный идентификатор датчика температуры окружающего воздуха. (DS18B20)
    uint8_t ExternalTempSensorId[8] = {};
    
    // Объём воды полного бака в литрах.
    float WaterTankVolumeLitre;
    
    // Электрическая мощность нагревательного элемента — ТЭНа, кВТ.
    float WaterHeaterPowerKWatt;
    
    // Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
    uint8_t WaterLevelMeasureIntervalMsec;
        
    // Размер буфера медианного фильтра для сырых показаний датчика уровня воды.
    uint8_t WaterLevelMedianFilterSize;
        
    // Размер буфера для усреднения показаний после медианного фильтра.
    uint8_t WaterLevelAvgFilterSize;
        
    // Порог отключения клапана набора воды.
    uint8_t WaterValveCutOffPercent;
        
    // Размер скользящего окна для температуры внутри бака.
    uint8_t InternalTempAvgFilterSize;
        
    // Максимальное время набора воды, если выше уровня 'Cut-Off' в секундах.
    uint8_t WaterValveTimeoutSec;
    
    // Число ошибок определения уровня воды, при достижении которого, на LED дисплее должны отобразиться прочерки.
    uint8_t WaterLevelErrorThreshold;
    
    void SelfFix()
    {
        ButtonPressTimeMsec = FixButtonPressTimeMsec(ButtonPressTimeMsec);
        ButtonLongPressTimeMsec = FixButtonLongPressTimeMsec(ButtonLongPressTimeMsec);
        WaterLevelEmpty = FixWaterLevelEmpty(WaterLevelEmpty);
        WaterLevelFull = FixWaterLevelFull(WaterLevelFull, WaterLevelEmpty); // Минимум 2 сантиметра.
        MinimumWaterHeatingPercent = FixMinimumWaterHeatingPercent(MinimumWaterHeatingPercent);
        
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
            WiFiPower = 60;   // 60 = 15.0 dBm
        }
        
        if (WaterTankVolumeLitre < 1 || WaterTankVolumeLitre > 100 || isnan(WaterTankVolumeLitre))
        {
            WaterTankVolumeLitre = 37.32212;
        }
        
        if (WaterHeaterPowerKWatt < 0.1f || WaterHeaterPowerKWatt > 5 || isnan(WaterHeaterPowerKWatt))
        {
            WaterHeaterPowerKWatt = 1.247616; // Полтора-киловатный ТЭН с учётом КПД.
        }
        
        if (WaterLevelMeasureIntervalMsec < 10 || WaterLevelMeasureIntervalMsec > 200)
        {
            WaterLevelMeasureIntervalMsec = 60;
        }
        
        if (WaterLevelMedianFilterSize == 0 || WaterLevelMedianFilterSize > kWaterLevelMedianMaxSize)
        {
            WaterLevelMedianFilterSize = 191;  // Лучше не чётное число.
        }
            
        if (WaterLevelAvgFilterSize == 0 || WaterLevelAvgFilterSize > kWaterLevelAvgFilterMaxSize)
        {
            WaterLevelAvgFilterSize = 32;
        }
            
        if (WaterValveCutOffPercent < 90 || WaterValveCutOffPercent > 99)
        {
            WaterValveCutOffPercent = 90;
        }
            
        if (InternalTempAvgFilterSize == 0 || InternalTempAvgFilterSize > kInternalTempAvgFilterSize)
        {
            InternalTempAvgFilterSize = 4;
        }
            
        if (WaterValveTimeoutSec < 5)
        {
            WaterValveTimeoutSec = 5;	
        }
        
        if (WaterLevelErrorThreshold == 0)
        {
            WaterLevelErrorThreshold = 255;
        }
        
        Chart.SelfFix();
    }
    
    // От 20 до 80 мсек.
    static uint8_t FixButtonPressTimeMsec(const uint8_t timeMsec)
    {
        // Обычно для антидребезга задают 50 мс.
        if(timeMsec < 20 || timeMsec > 80)
        {
            return 40;
        }
        return timeMsec;
    }
    
    // От 1 до 10 секунд.
    static uint16_t FixButtonLongPressTimeMsec(const uint16_t timeMsec)
    {
        if (timeMsec < 1000 || timeMsec > 10000)
        {
            return 4000;
        }
        return timeMsec;
    }
    
    static uint16_t FixWaterLevelEmpty(const uint16_t waterLevelEmpty)
    {
        // В одном сантиметре - 58 микросекунд.
        static const auto minimum = kTankMinimumHeightCm * 58;
        static const auto maximum = kTankMaximumHeightCm * 58;
        
        if(waterLevelEmpty < minimum || waterLevelEmpty > maximum) 
        {
            return round(kDefaultFullTankDistanceCm * 58);
        }
        return waterLevelEmpty;
    }
    
    static uint16_t FixWaterLevelFull(const uint16_t waterLevelFull, const uint16_t waterLevelEmpty)
    {
        // 2 сантиметра.
        if(waterLevelFull < 116 || waterLevelFull > waterLevelEmpty)
        {
            return round(kDefaultEmptyTankDistanceCm * 58);
        }
        return waterLevelFull;
    }
    
    static uint8_t FixMinimumWaterHeatingPercent(const uint8_t minimumWaterHeatingPercent)
    {
        if (minimumWaterHeatingPercent > 100)
        {
            return 25;
        }
        return minimumWaterHeatingPercent;
    }
    
} __attribute__((aligned(16)));		// Размер структуры должен быть кратен 4 для удобства подсчета CRC32 или 16 для удобства хранения в странице EEPROM.

extern PropertyStruct g_properties, g_writeProperties;
