#pragma once
#include "HeaterTempLimit.h"
#include "WaterLevelTask.h"

class HeatingTimeLeft final
{
public:
    
    void Init(const float tank_volume_litre, const float heater_power_kWatt)
    {
        m_tankVolumeLitre = tank_volume_litre;
        m_heaterPowerKWatt = heater_power_kWatt;
    }
    
    // Возвращает время до окончания нагрева в минутах (от 0 до x).
    float GetTimeLeftMin()
    {
        float int_temp = g_tempSensorTask.GetAverageInternalTemp();
        float air_temp = g_tempSensorTask.GetAverageAirTemp();
    
        // Узнаём желаемую температуру воды в баке.
        uint8_t limitTemp;
        g_heaterTempLimit->TryGetTargetTemperature(limitTemp);

        // Нужно учесть на сколько процентов заполнен бак.
        auto tank_percent = g_waterLevelTask.GetPercent();
    
        float minutes = CalcTimeLeft(int_temp, limitTemp, tank_percent);
        return minutes;
    }
    
    // Возвращает примерный прогресс нагрева воды от 0 до 100%.
    uint8_t GetProgress()
    {
        float internal_temp = g_tempSensorTask.GetAverageInternalTemp();
    
        // Узнаём желаемую температуру воды в баке.
        uint8_t limit_temp;
        g_heaterTempLimit->TryGetTargetTemperature(limit_temp);
        
        if (internal_temp > limit_temp)
        {
            internal_temp = limit_temp;
        }
            
        float percent = 100.0 / (limit_temp - kAirTempLowerBound) * (internal_temp - kAirTempLowerBound);
        
        percent = round(percent);
        
        if (percent > 100)
        {
            percent = 100;
        }
            
        return percent;
    }
    
private:
    
    static constexpr double kQ = 0.00117;
    
    // Объём воды полного бака в литрах.
    float m_tankVolumeLitre;

    // Электрическая мощность нагревательного элемента — ТЭНа с учётом его КПД, кВТ.
    float m_heaterPowerKWatt;
    
    // Время до окончания нагрева в минутах.
    // "internalTemp" - Текущая температура воды в баке.
    // "limitTemp" - Целевая темпаратура.
    float CalcTimeLeft(float internal_temp, uint8_t target_temp, uint8_t tank_percent)
    {
        if (internal_temp >= target_temp)
        {
            return 0;
        }
    
        //  Формула расчета времени нагрева T = 0.00117 * V * (tк - tн) / W
        //  Т – время нагрева воды, час
        //  V – объем водонагревательного бака(л)
        //  tк – конечная температура воды, °С(обычно 60°C)
        //  tн – начальная температура воды, °С
        //  W – электрическая мощность нагревательного элемента — ТЭНа, кВТ

        double time_H = kQ * m_tankVolumeLitre * (target_temp - internal_temp) / m_heaterPowerKWatt;

        // В минутах.
        float minutes = time_H * 60;
            
        return minutes;
    }
};

extern HeatingTimeLeft g_heatingTimeLeft;