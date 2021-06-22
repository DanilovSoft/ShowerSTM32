#pragma once
#include "math.h"
#include "Defines.h"

class TempStep
{
public:

    uint8_t GetLimit(const uint8_t air_temp)
    {
        uint8_t index = GetIndex(air_temp);
        return m_internal[index];
    }

    uint8_t GetLimit(const float air_temp) const
    {
        uint8_t index = GetIndexf(air_temp);
        return m_internal[index];
    }

    // Принимает температуру окружающего воздуха в грудусах, что-бы
    // увеличить температуру нагрева при такой температуре окружающей среды.
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
    // уменьшить температуру нагрева при такой температуре окружающей среды.
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
        uint8_t prev_spot = m_internal[0];
        
        for (auto i = 0; i < kAirTempSteps; i++)
        {
            uint8_t& internal_temp = m_internal[i];

            if (internal_temp == 0 || internal_temp > kInternalTempLimit)
            {
                internal_temp = 36;
            }

            if (i > 0)
            {
                if (internal_temp > prev_spot)
                {
                    internal_temp = prev_spot;
                }
            }

            prev_spot = internal_temp;
        }
    }
    
private:
    
    // Температура в баке.
    uint8_t m_internal[kAirTempSteps];
    
    uint8_t GetIndex(const uint8_t air_temp) const
    {
        if (air_temp <= kAirTempLowerBound)
        {
            return 0;
        }
        
        if (air_temp >= (kAirTempUpperBound - 1))
        {
            return kAirTempSteps - 1;
        }
        
        return air_temp - kAirTempLowerBound;
    }
    
    uint8_t GetIndexf(const float air_temp) const
    {
        uint8_t t = round(air_temp);
        return GetIndex(t);
    }
};

class PropertyStruct
{
public:
    
    // Температурная зависимость.
    TempStep Chart;
    
    // Максимальный уровень воды в микросекундах (полный бак).
    uint16_t WaterLevelFull;
    
    // Минимальный уровень воды в микросекундах (пустой бак).
    uint16_t WaterLevelEmpty;
    
    // Минимальный уровень воды в баке (%) для включения нагревателя.
    uint8_t MinimumWaterHeatingPercent;
    
    // Абсолютное время нагрева, (от 1 до 24ч.)
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
    
    // Число ошибок определения уровня воды, при достижении которого, на LED дисплее должны отобразиться прочерки.
    uint8_t WaterLevelErrorThreshold;
    
    void SelfFix()
    {
        ButtonPressTimeMsec = FixButtonPressTimeMsec(ButtonPressTimeMsec);
        ButtonLongPressTimeMsec = FixButtonLongPressTimeMsec(ButtonLongPressTimeMsec);
        WaterLevelEmpty = FixWaterLevelEmpty(WaterLevelEmpty);
        WaterLevelFull = FixWaterLevelFull(WaterLevelFull, WaterLevelEmpty); 
        MinimumWaterHeatingPercent = FixMinimumWaterHeatingPercent(MinimumWaterHeatingPercent);
        HeatingTimeLimitMin = FixHeatingTimeLimitMin(HeatingTimeLimitMin);
        LightBrightness = FixLightBrightness(LightBrightness);
        AbsoluteHeatingTimeLimitHours = FixAbsoluteHeatingTimeLimitHours(AbsoluteHeatingTimeLimitHours);
        WiFiPower = FixWiFiPower(WiFiPower);
        WaterTankVolumeLitre = FixWaterTankVolumeLitre(WaterTankVolumeLitre);
        WaterHeaterPowerKWatt = FixWaterHeaterPowerKWatt(WaterHeaterPowerKWatt);
        WaterLevelMeasureIntervalMsec = FixWaterLevelMeasureIntervalMsec(WaterLevelMeasureIntervalMsec);
        WaterLevelMedianFilterSize = FixWaterLevelMedianFilterSize(WaterLevelMedianFilterSize);
        WaterLevelAvgFilterSize = FixWaterLevelAvgFilterSize(WaterLevelAvgFilterSize);
        WaterValveCutOffPercent = FixWaterValveCutOffPercent(WaterValveCutOffPercent);
        InternalTempAvgFilterSize = FixInternalTempAvgFilterSize(InternalTempAvgFilterSize);    
        WaterLevelErrorThreshold = FixWaterLevelErrorThreshold(WaterLevelErrorThreshold);
        
        Chart.SelfFix();
    }
    
    // От 20 до 80 мсек.
    static uint8_t FixButtonPressTimeMsec(const uint8_t time_msec)
    {
        // Обычно для антидребезга задают 50 мс.
        if(time_msec < 20 || time_msec > 80)
        {
            return 40;
        }
        return time_msec;
    }
    
    // От 1 до 10 секунд.
    static uint16_t FixButtonLongPressTimeMsec(const uint16_t time_msec)
    {
        if (time_msec < 1000 || time_msec > 10000)
        {
            return 4000;
        }
        return time_msec;
    }
    
    static uint16_t FixWaterLevelEmpty(const uint16_t water_level_empty)
    {
        // В одном сантиметре - 58 микросекунд.
        static const auto minimum = kTankMinimumHeightCm * 58;
        static const auto maximum = kTankMaximumHeightCm * 58;
        
        if(water_level_empty < minimum || water_level_empty > maximum) 
        {
            return round(kDefaultEmptyTankDistanceCm * 58);
        }
        return water_level_empty;
    }
    
    // Минимум 2 сантиметра.
    static uint16_t FixWaterLevelFull(const uint16_t water_level_full, const uint16_t water_level_empty)
    {
        // 2 сантиметра.
        if(water_level_full < 116 || water_level_full > water_level_empty)
        {
            return round(kDefaultFullTankDistanceCm * 58);
        }
        return water_level_full;
    }
    
    static uint8_t FixMinimumWaterHeatingPercent(const uint8_t minimum_water_heating_percent)
    {
        if (minimum_water_heating_percent > 100)
        {
            return kDefaultMinimumWaterHeatingPercent;
        }
        return minimum_water_heating_percent;
    }
    
    static uint8_t FixHeatingTimeLimitMin(const uint8_t heating_time_limit_min)
    {
        if (heating_time_limit_min < 10 || heating_time_limit_min > 180)
        {
            return 70;
        }
        return heating_time_limit_min;
    }
    
    // От 1 до 100
    static uint8_t FixLightBrightness(const uint8_t light_brightness)
    {
        if (light_brightness == 0 || light_brightness > 100)
        {
            return 100;
        }
        return light_brightness;
    }
    
    // От 1 до 24
    static uint8_t FixAbsoluteHeatingTimeLimitHours(const uint8_t absolute_heating_time_limit_hours)
    {
        if (absolute_heating_time_limit_hours == 0 || absolute_heating_time_limit_hours > 24)
        {
            return 6;
        }
        return absolute_heating_time_limit_hours;
    }
    
    // от 10 до 20.5 dBm.
    static uint8_t FixWiFiPower(const uint8_t wifi_power)
    {
        if (wifi_power < 40 || wifi_power > 82)
        {
            return kDefaultWiFiPower;
        }
        return wifi_power;
    }
    
    static float FixWaterTankVolumeLitre(const float water_tank_volume_litre)
    {
        if (water_tank_volume_litre < 1 || water_tank_volume_litre > 100 || isnan(water_tank_volume_litre))
        {
            return kDefaultWaterTankVolumeLitre;
        }
        return water_tank_volume_litre;
    }
    
    static float FixWaterHeaterPowerKWatt(const float water_heater_power_kwatt)
    {
        if (water_heater_power_kwatt < 0.1f || water_heater_power_kwatt > 5 || isnan(water_heater_power_kwatt))
        {
            return kDefaultWaterHeaterPowerKWatt;
        }
        return water_heater_power_kwatt;
    }
    
    // От 10 до 200
    static uint8_t FixWaterLevelMeasureIntervalMsec(const uint8_t water_level_measure_interval_msec)
    {
        if (water_level_measure_interval_msec < 10 || water_level_measure_interval_msec > 200)
        {
            return 60;
        }
        return water_level_measure_interval_msec;
    }
    
    // От 1 до kWaterLevelMedianMaxSize
    static uint8_t FixWaterLevelMedianFilterSize(const uint8_t water_level_median_filter_size)
    {
        if (water_level_median_filter_size == 0 || water_level_median_filter_size > kWaterLevelMedianMaxSize)
        {
            return 191;   // Лучше не чётное число.
        }
        return water_level_median_filter_size;
    }
    
    static uint8_t FixWaterLevelAvgFilterSize(const uint8_t water_level_avg_filter_size)
    {
        if (water_level_avg_filter_size == 0 || water_level_avg_filter_size > kWaterLevelAvgFilterMaxSize)
        {
            return 30;
        }
        return water_level_avg_filter_size;
    }
    
    // От 90 до 99
    static uint8_t FixWaterValveCutOffPercent(const uint8_t water_valve_cut_off_percent)
    {
        if (water_valve_cut_off_percent < 90 || water_valve_cut_off_percent > 99)
        {
            return 90;
        }
        return water_valve_cut_off_percent;
    }
    
    static uint8_t FixInternalTempAvgFilterSize(const uint8_t internal_temp_avg_filter_size)
    {
        if (internal_temp_avg_filter_size == 0 || internal_temp_avg_filter_size > kInternalTempAvgFilterSize)
        {
            return 4;
        }
        return internal_temp_avg_filter_size;
    }
    
    // От 1 до 255
    static uint8_t FixWaterLevelErrorThreshold(const uint8_t water_level_error_threshold)
    {
        if (water_level_error_threshold == 0)
        {
            return 50;
        }
        return water_level_error_threshold;
    }
    
} __attribute__((aligned(16)));		// Размер структуры должен быть кратен 4 для удобства подсчета CRC32 или 16 для удобства хранения в странице EEPROM.

//extern PropertyStruct g_properties;
extern PropertyStruct g_writeProperties;
