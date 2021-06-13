#pragma once
#include "iActiveTask.h"
#include "LiquidCrystal.h"
#include "HeatingTimeLeft.h"
#include "WaterLevelAnimTask.h"
#include "TempSensor.h"
#include "WaterLevelTask.h"
#include "HeaterTempLimit.h"
#include "HeaterTask.h"
#include "ValveTask.h"
#include "Common.h"

class LcdTask final : public iActiveTask
{
private:

    LiquidCrystal m_lc;

    void Init()
    {

    }
    
    void DisplayTemp(char* buf, int16_t temp)
    {
        char first_char = ' ';
        if (temp < 0)
        {
            if (temp < -9)
            {
                temp = -9;
            }

            temp = -temp;
            first_char = '-';
        }

        char tmp = Common::itoa(temp / 10);
        buf[0] = tmp == '0' ? first_char : tmp;
        buf[1] = Common::itoa(temp % 10);
    }
    
    void Run()
    {
        if (!m_lc.Setup(16, 2))
        {
            return;
        }
        
        char line_temperature_buf[17] = "--\x01  \xA0\x61\xBA --\x01\x04--\x01";     // 20°  Бак 10°→38°
        char line_time_left[17] = "Oc\xBF\x61\xBBoc\xC4 00 \xBC\xB8\xBD.";  // Осталось 10 мин.
        char line_water_ready[17] = "  \x42o\xE3\x61 \xBD\x61\xB4pe\xBF\x61  ";  // Вода нагрета
        char line_water_level[17] = "\xA9po\xB3\x65\xBD\xC4 \xB3o\xE3\xC3 -- ";   // Уровень воды --%

        m_lc.Clear();

        while (true)
        {
            uint8_t target_temp = 0;
            bool got_target_temp = g_heaterTempLimit.TryGetTargetTemperature(target_temp);
            bool heater_enabled = Common::GetIsHeaterEnabled();
            bool heater_switch_enabled = Common::CircuitBreakerIsOn();     // Включен ли автомат.
            bool valve_is_open = Common::ValveIsOpen();
            
            m_lc.SetCursor(0, 0);
            
            if (valve_is_open && heater_enabled)
            {
                   // уровень воды в 1 строке
                if(g_waterLevelTask.SensorIsBlocked)
                {
                    line_water_level[13] = ' ';
                    line_water_level[14] = '?';
                    line_water_level[15] = ' ';
                }
                else
                {
                    uint8_t water_level = g_waterLevelTask.DisplayingPercent;
                    char tmp = Common::itoa(water_level / 10);
                    line_water_level[13] = tmp == '0' ? ' ' : tmp;
                    line_water_level[14] = Common::itoa(water_level % 10);
                    line_water_level[15] = '%';
                }
                m_lc.WriteString(line_water_level);
            }
            else
            {
                if (g_tempSensorTask.InternalSensorInitialized)
                {
                    // Записывает 2 символа в lineTemperatureBuf в положение "Температура в баке"
                    DisplayTemp(line_temperature_buf + 9, round(g_tempSensorTask.AverageInternalTemp));
                }
            
                if (g_tempSensorTask.ExternalSensorInitialized)
                {
                    // Записывает 2 символа в lineTemperatureBuf в положение "Температура на улице"
                    DisplayTemp(line_temperature_buf, round(g_tempSensorTask.AverageExternalTemp));
                }
            
                if (got_target_temp)
                {
                    // Записать 2 символа до скольки нужно нагреть
                    line_temperature_buf[13] = Common::itoa(target_temp / 10);
                    line_temperature_buf[14] = Common::itoa(target_temp % 10);
                }
            }
            
            m_lc.WriteString(line_temperature_buf);
            
            m_lc.SetCursor(0, 1);	
            if (heater_enabled)
            {
                float time_left_min = g_heatingTimeLeft.GetTimeLeftMin();
            
                // Округляем до целых.
                uint8_t time_left_min_int = roundf(time_left_min);
            
                if (time_left_min_int > 99)
                {
                    time_left_min_int = 99;  // Дисплей не может отображать больше 2 разрядов.
                }
            
                if (time_left_min_int == 0)
                {
                    time_left_min_int = 1;  // Нет смысла отображать 0 минут.
                }
            
                char tmp = Common::itoa(time_left_min_int / 10);
                line_time_left[9] = tmp == '0' ? ' ' : tmp;
                line_time_left[10] = Common::itoa(time_left_min_int % 10);
            
                m_lc.WriteString(line_time_left);
            }
            else
            {
                bool water_heated = false;
                if (got_target_temp && g_tempSensorTask.InternalSensorInitialized)
                {
                    water_heated = (round(g_tempSensorTask.AverageInternalTemp) >= target_temp);
                }
                
                if (heater_switch_enabled && water_heated && g_waterLevelTask.DisplayingPercent >= g_properties.MinimumWaterHeatingPercent)
                {
                      // 'Вода нагрета'
                    m_lc.WriteString(line_water_ready);
                }
                else
                {
                      // Уровень воды во 2 строке.
                    if(g_waterLevelTask.Preinitialized)
                    {
                        if (g_waterLevelTask.SensorIsBlocked)
                        {
                            line_water_level[13] = ' ';
                            line_water_level[14] = '?';
                            line_water_level[15] = g_waterLevelTask.GetIsInitialized() ? ' ' : g_waterLevelAnimTask.GetChar();
                        }
                        else
                        {
                            uint8_t water_level = g_waterLevelTask.DisplayingPercent;
                            char tmp = Common::itoa(water_level / 10);
                            line_water_level[13] = tmp == '0' ? ' ' : tmp;
                            line_water_level[14] = Common::itoa(water_level % 10);
                            line_water_level[15] = g_waterLevelTask.GetIsInitialized() ? '%' : g_waterLevelAnimTask.GetChar();
                        }
                    }
                    else
                    {
                        line_water_level[15] = g_waterLevelAnimTask.GetChar();
                    }
                    m_lc.WriteString(line_water_level);
                }
            }
            taskYIELD();
        }
    }
};

extern LcdTask g_lcdTask;
