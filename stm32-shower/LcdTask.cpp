#include "LcdTask.h"
#include "HeatingTimeLeft.h"
#include "WaterLevelAnimTask.h"
#include "TempSensor.h"
#include "WaterLevelTask.h"
#include "HeaterTempLimit.h"
#include "HeaterTask.h"
#include "ValveTask.h"
#include "Common.h"

LcdTask g_lcdTask;
	
void LcdTask::Init()
{

}
	
void LcdTask::DisplayTemp(char* buf, int16_t temp)
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

    char tmp = itoa(temp / 10);
    buf[0] = tmp == '0' ? first_char : tmp;
    buf[1] = itoa(temp % 10);
}
	
void LcdTask::Run()
{
	if (!m_lc.Setup(16, 2))
	{
		return;
	}
    	
    char lineTemperatureBuf[17] = "--\x01  \xA0\x61\xBA --\x01\x04--\x01";    // 20°  Бак 10°→38°
    char lineTimeLeft[17] = "Oc\xBF\x61\xBBoc\xC4 00 \xBC\xB8\xBD."; // Осталось 10 мин.
    char lineWaterReady[17] = "  \x42o\xE3\x61 \xBD\x61\xB4pe\xBF\x61  "; // Вода нагрета
    char lineWaterLevel[17] = "\xA9po\xB3\x65\xBD\xC4 \xB3o\xE3\xC3 -- ";  // Уровень воды --%

    m_lc.Clear();

    while (true)
    {
        uint8_t internalTempLimit = 0;
	    bool gotInternalTempLimit = g_heaterTempLimit.TryGetTargetTemperature(internalTempLimit);
        bool heaterEnabled = g_heaterTask.GetIsHeaterEnabled();
        bool heaterSwitchEnabled = CircuitBreakerIsOn();  // Включен ли автомат
        bool valveIsOpen = g_valveTask.ValveIsOpen();
    		
        m_lc.SetCursor(0, 0);
    		
        if (valveIsOpen && heaterEnabled)
        {   // уровень воды в 1 строке
            if (g_waterLevelTask.SensorIsBlocked)
            {
                lineWaterLevel[13] = ' ';
                lineWaterLevel[14] = '?';
                lineWaterLevel[15] = ' ';
            }
            else
            {
	            uint8_t water_level = g_waterLevelTask.DisplayingPercent;
                char tmp = itoa(water_level / 10);
                lineWaterLevel[13] = tmp == '0' ? ' ' : tmp;
                lineWaterLevel[14] = itoa(water_level % 10);
                lineWaterLevel[15] = '%';
            }
            m_lc.WriteString(lineWaterLevel);
        }
        else
        {
	        if (g_tempSensorTask.InternalSensorInitialized)
	        {
		        // Записывает 2 символа в lineTemperatureBuf в положение "Температура в баке"
                DisplayTemp(lineTemperatureBuf + 9, round(g_tempSensorTask.AverageInternalTemp));
	        }
	        
	        if (g_tempSensorTask.ExternalSensorInitialized)
	        {
		        // Записывает 2 символа в lineTemperatureBuf в положение "Температура на улице"
                DisplayTemp(lineTemperatureBuf, round(g_tempSensorTask.AverageExternalTemp));
	        }
	        
	        if (gotInternalTempLimit)
            {
                // Записать 2 символа до скольки нужно нагреть
                lineTemperatureBuf[13] = itoa(internalTempLimit / 10);
                lineTemperatureBuf[14] = itoa(internalTempLimit % 10);
            }
        }
			
        m_lc.WriteString(lineTemperatureBuf);
			
        m_lc.SetCursor(0, 1);	
        if (heaterEnabled)
        {
	        float timeLeftMin = g_heatingTimeLeft.GetTimeLeft();
	        
	        // Округляем до целых.
	        uint8_t timeLeft = roundf(timeLeftMin);
	        
	        if (timeLeft > 99)
	        {
		        timeLeft = 99; // Дисплей не может отображать больше 2 разрядов.
	        }
	        
	        if (timeLeft == 0)
	        {
		        timeLeft = 1; // Нет смысла отображать 0 минут.
	        }
	        
            char tmp = itoa(timeLeft / 10);
            lineTimeLeft[9] = tmp == '0' ? ' ' : tmp;
            lineTimeLeft[10] = itoa(timeLeft % 10);
	        
            m_lc.WriteString(lineTimeLeft);
        }
        else
        {
            bool waterReady = false;
	        if (gotInternalTempLimit && g_tempSensorTask.InternalSensorInitialized)
	        {
		        waterReady = (round(g_tempSensorTask.AverageInternalTemp) >= internalTempLimit);
	        }
    			
	        if (heaterSwitchEnabled && waterReady && g_waterLevelTask.DisplayingPercent >= g_properties.MinimumWaterHeatingPercent)
            {  // 'Вода нагрета'
                m_lc.WriteString(lineWaterReady);
            }
            else
            {  // Уровень воды во 2 строке.
	            if (g_waterLevelTask.Preinitialized)
                {
	                if (g_waterLevelTask.SensorIsBlocked)
                    {
                        lineWaterLevel[13] = ' ';
                        lineWaterLevel[14] = '?';
	                    lineWaterLevel[15] = g_waterLevelTask.Initialized ? ' ' : g_waterLevelAnimTask.GetChar();
                    }
                    else
                    {
	                    uint8_t water_level = g_waterLevelTask.DisplayingPercent;
                        char tmp = itoa(water_level / 10);
                        lineWaterLevel[13] = tmp == '0' ? ' ' : tmp;
                        lineWaterLevel[14] = itoa(water_level % 10);
	                    lineWaterLevel[15] = g_waterLevelTask.Initialized ? '%' : g_waterLevelAnimTask.GetChar();
                    }
                }
                else
                {
                    lineWaterLevel[15] = g_waterLevelAnimTask.GetChar();
                }
                m_lc.WriteString(lineWaterLevel);
            }
        }
        taskYIELD();
    }
}