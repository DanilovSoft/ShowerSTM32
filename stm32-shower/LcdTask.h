#pragma once
#include "TaskBase.h"
#include "LiquidCrystal.h"
#include "HeatingTimeLeft.h"
#include "InitializationTask.h"
#include "TempSensorTask.h"
#include "WaterLevelTask.h"
#include "WaterLevelAnimationTask.h"
#include "HeaterTempLimit.h"
#include "HeaterTask.h"
#include "ValveTask.h"
#include "Common.h"

class LcdTask final : public TaskBase
{
public:
    
    void Init()
    {
    }
    
private:

    LiquidCrystal m_liquidCrystal;
    
    static void DisplayTemp(char* buf, int16_t temp)
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

        char tmp = Common::DigitToChar(temp / 10);
        buf[0] = tmp == '0' ? first_char : tmp;
        buf[1] = Common::DigitToChar(temp % 10);
    }
    
    void Run()
    {
        g_initializationTask.WaitForPropertiesInitialization();
        
        if (!m_liquidCrystal.Setup(16, 2))
        {
            return;
        }
        
        char line_temperature_buf[17] = "--\x01  \xA0\x61\xBA --\x01\x04--\x01";     // 20°  Бак 10°→38°
        char line_time_left[17] = "Oc\xBF\x61\xBBoc\xC4 00 \xBC\xB8\xBD.";  // Осталось 10 мин.
        char line_water_ready[17] = "  \x42o\xE3\x61 \xBD\x61\xB4pe\xBF\x61  ";  // Вода нагрета
        char line_water_level[17] = "\xA9po\xB3\x65\xBD\xC4 \xB3o\xE3\xC3 -- ";   // Уровень воды --%

        m_liquidCrystal.Clear();

        while (true)
        {
            uint8_t target_temp = 0;
            
            bool got_target_temp = g_heaterTempLimit.TryGetTargetTemperature(target_temp);
            bool circuit_breaker_is_on = Common::CircuitBreakerIsOn();  // Включен ли автомат.
            bool heater_is_on = Common::HeaterIsOn();  // Включен ли ТЭН.
            bool valve_is_open = Common::ValveIsOpen(); // Набирается ли вода.
            
            m_liquidCrystal.SetCursor(0, 0);
            
            if (valve_is_open && heater_is_on)
            {
                // Набирается вода, включен автомат и включен ТЭН.
                
                if(g_waterLevelTask.PreInitialized)
                {
                    CopyPercent(line_water_level);
                
                    // Уровень воды в первой строке.
                    if(g_waterLevelTask.GetIsError() || !g_waterLevelTask.GetIsInitialized())
                    {
                        line_water_level[15] = g_wlAnimationTask.GetWaterLevelAnimChar();
                    }
                    else
                    {
                        line_water_level[15] = '%';
                    }
                }
                else
                {
                    line_water_level[13] = '-';
                    line_water_level[14] = '-';
                    line_water_level[15] = g_wlAnimationTask.GetWaterLevelAnimChar();
                }
                
                m_liquidCrystal.WriteString(line_water_level);
            }
            else
            {
                if (g_tempSensorTask.InternalSensorInitialized)
                {
                    // Записывает 2 символа в line_temperature_buf в положение "Температура в баке".
                    DisplayTemp(line_temperature_buf + 9, round(g_tempSensorTask.AverageInternalTemp));
                }
            
                if (g_tempSensorTask.ExternalSensorInitialized)
                {
                    // Записывает 2 символа в line_temperature_buf в положение "Температура на улице".
                    DisplayTemp(line_temperature_buf, round(g_tempSensorTask.AverageExternalTemp));
                }
            
                if (got_target_temp)
                {   
                    // Записать 2 символа до скольки нужно нагреть.
                    line_temperature_buf[13] = Common::DigitToChar(target_temp / 10);
                    line_temperature_buf[14] = Common::DigitToChar(target_temp % 10);
                }
            }
            
            m_liquidCrystal.WriteString(line_temperature_buf);
            m_liquidCrystal.SetCursor(0, 1);
            
            if (heater_is_on)
            {
                // ТЭН включен.
                
                float time_left_min = g_heatingTimeLeft->GetTimeLeftMin();
            
                // Округляем до целых.
                uint8_t time_left_min_int = roundf(time_left_min);
            
                if (time_left_min_int > 99)
                {
                    time_left_min_int = 99;  // Дисплей не может отображать больше двух разрядов.
                }
            
                if (time_left_min_int == 0)
                {
                    time_left_min_int = 1;  // Нет смысла отображать 0 минут.
                }
            
                char first_digit = Common::DigitToChar(time_left_min_int / 10);
                line_time_left[9] = first_digit == '0' ? ' ' : first_digit;
                line_time_left[10] = Common::DigitToChar(time_left_min_int % 10);
            
                m_liquidCrystal.WriteString(line_time_left);
            }
            else
            {
                bool water_heated = false;
                
                if (got_target_temp && g_tempSensorTask.InternalSensorInitialized)
                {
                    water_heated = (round(g_tempSensorTask.AverageInternalTemp) >= target_temp);
                }
                
                if (circuit_breaker_is_on && water_heated && g_waterLevelTask.Percent >= g_properties.MinimumWaterHeatingPercent)
                {
                    // 'Вода нагрета'.
                    m_liquidCrystal.WriteString(line_water_ready);
                }
                else
                {
                    // Уровень воды во второй строке.
                    if(g_waterLevelTask.PreInitialized)
                    {
                        // У датчика есть показание.
                        
                        CopyPercent(line_water_level);
                        
                        if (g_waterLevelTask.GetIsError() || !g_waterLevelTask.GetIsInitialized())
                        {
                            line_water_level[15] = g_wlAnimationTask.GetWaterLevelAnimChar();
                        }
                        else
                        {
                            line_water_level[15] = '%';
                        }
                    }
                    else
                    {
                        // У датчика уровня ещё нет ни одного показания.
                        
                        line_water_level[13] = '-';
                        line_water_level[14] = '-';
                        line_water_level[15] = g_wlAnimationTask.GetWaterLevelAnimChar();
                    }
                    
                    m_liquidCrystal.WriteString(line_water_level);
                }
            }
            taskYIELD();
        }
    }
    
    static void CopyPercent(char* line_water_level)
    {
        uint8_t water_level = g_waterLevelTask.Percent;
        char first_digit = Common::DigitToChar(water_level / 10);
        line_water_level[13] = first_digit == '0' ? ' ' : first_digit;
        line_water_level[14] = Common::DigitToChar(water_level % 10);
    }
};

extern LcdTask g_lcdTask;
