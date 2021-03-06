#pragma once
#include "Request.h"
#include "TaskBase.h"
#include "Common.h"
#include "Buzzer.h"
#include "Properties.h"
#include "HeaterTask.h"
#include "EepromHelper.h"
#include "stm32f10x_iwdg.h"
#include "WatchDogTask.h"
#include "string.h"
#include "WaterLevelTask.h"
#include "HeatingTimeLeft.h"
#include "TempSensorTask.h"
#include "InitializationTask.h"

class WiFiTask final : public TaskBase
{
public:
    
private:
    
    Request m_request;
    uint8_t m_requestData[256] = {0}; // Буфер для данных входного запроса.
    
    bool InitWiFi()
    {
        bool wps_button_pressed = Common::ButtonWPSPressed();
        
        for (auto i = 0; i <= kWiFiTryInitLimit; i++)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            Common::EnableEsp8266();
        
            if (TryInitWiFi())
            {
                if (wps_button_pressed)
                {
                    if (!WPS())
                    {
                        return false;
                    }
                }
                return true;
            }
            
            Common::DisableEsp8266();
        }
        
        return false;
    }
    
    bool TryInitWiFi()
    {
        // Ожидание инициализации WiFi.
        if (!g_uartStream.WaitLine("ready", 1000))
        {
            return false;
        }
        
        //    	uartStream.WriteLine("AT+UART_DEF=57600,8,1,0,0\r\n");
        //		if (!uartStream.WaitLine("OK", 200))
        //			return false;
        
        // Выключить эхо.
        g_uartStream.WriteLine("ATE0\r\n");
    
        if (!g_uartStream.WaitLine("OK", 300))
        {
            return false;
        }
        
        g_uartStream.WriteLine("AT+CWMODE_CUR=1\r\n");
    
        if (!g_uartStream.WaitLine("OK", 300))
        {
            return false;
        }
        
        char rf_power[] = "AT+RFPOWER=\0\0\0\0";
        Common::NumberToString(g_properties.WiFiPower, rf_power + 11);
        strcat(rf_power, "\r\n");
    
        g_uartStream.WriteLine(rf_power); 		// 40..82, unit:0.25dBm
    
        if(!g_uartStream.WaitLine("OK", 300))
        {
            return false;
        }

        g_uartStream.WriteLine("AT+CIPMUX=1\r\n");      // разрешить множественные подключения.
    
        if(!g_uartStream.WaitLine("OK", 300))
        {
            return false;
        }

        g_uartStream.WriteLine("AT+CIPSERVER=1,333\r\n");      // запустить TCP сервер, порт 333.
    
        if(!g_uartStream.WaitLine("OK", 300))
        {
            return false;
        }
    
        //    	uartStream.WriteLine("AT+CWJAP_DEF=\"Miles\",\"PASSW0RD\",\"d4:ca:6d:11:38:af\"\r\n");
        //    	if (!uartStream.WaitLine("OK", 300))
        //        	return false;
    
        return true;
    }
    
    bool WPS()
    {
        static const BeepSound samples[] = 
        { 
            BeepSound(3300, 140),
            BeepSound(30),
            BeepSound(3600, 120),
            BeepSound(30),
            BeepSound(4000, 120),
            BeepSound(30),
        };
        g_buzzer.PlaySound(samples, sizeof(samples) / sizeof(*samples));

        //			uartStream.WriteLine("AT+CWQAP");     // забыть сохраненную точку доступа
        //			if (!uartStream.WaitLine("OK", 100))
        //				return false;

        g_uartStream.WriteLine("AT+WPS=1\r\n");
        
        if (!g_uartStream.WaitLine("OK", 500))
        {
            return false;
        }
        
        return true;
    }
    
    void SetAP(const char* pref, uint8_t pref_size, uint8_t* data, uint8_t length)
    {
        pref_size -= 1;   // Не учитываем нуль-терминатор.
        
        char str[pref_size + length + 2 + 1];
        memcpy(str, pref, pref_size);
        memcpy(str + pref_size, data, length);
        
        str[pref_size + length] = '\r';
        str[pref_size + length + 1] = '\n';
        str[pref_size + length + 2] = 0;
        
        g_uartStream.WriteLine(str);
    }
    
    void InnerSetCurAP(uint8_t* data, uint8_t length)
    {
        static constexpr char pref[] = "AT+CWJAP_CUR=";
        SetAP(pref, sizeof(pref), data, length);
    }
    
    void InnerSetDefAP(uint8_t* data, uint8_t length)
    {
        static constexpr char pref[] = "AT+CWJAP_DEF=";
        SetAP(pref, sizeof(pref), data, length);
    }
    
    virtual void Run() override
    {
        Debug::Assert(g_properties.Initialized);
        
        if (InitWiFi())
        {
            while (true)
            {
                if (!DoWiFiEvents())
                {
                    taskYIELD();   // Нет запросов, можно отдать квант времени.
                }
            }
        }
    }

    bool DoWiFiEvents()
    {
        uint8_t request_length;
        if (!m_request.GetRequestSize(request_length))
        {
            return false;
        }
    
        ShowerCode code = m_request.GetRequestData(m_requestData);
        switch (code)
        {
        case ShowerCode::kNone:
        case ShowerCode::kUnknownCode:
        case ShowerCode::kOK:
            {
                break;
            }
        case ShowerCode::kGetWaterLevelEmpty:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelEmpty);
                break;
            }
        case ShowerCode::kGetWaterLevelFull:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelFull);
                break;
            }
        case ShowerCode::kSetWaterLevelFull:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelFull))
                {
                    g_writeProperties.WaterLevelFull = *(uint16_t*)m_requestData;
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kSetWaterLevelEmpty:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelEmpty))
                {
                    g_writeProperties.WaterLevelEmpty = *(uint16_t*)m_requestData;
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWaterLevel:
            {
                m_request.SendResponse(g_waterLevelTask.GetAvgUsec());
                break;
            }
        case ShowerCode::kGetWaterLevelRaw:
            {
                m_request.SendResponse(g_waterLevelTask.GetUsecRaw());
                break;
            }
        case ShowerCode::kGetTempChart:
            {
                m_request.SendResponse(&g_writeProperties.Chart, sizeof(g_writeProperties.Chart));
                break;
            }
        case ShowerCode::kSetTempChart:
            {
                if (request_length == sizeof(g_writeProperties.Chart))
                {
                    g_writeProperties.Chart.Parse(m_requestData);
                    g_properties.Chart.Parse(m_requestData);  // Установить значения в ОЗУ.
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kSave:
            {
                g_eepromHelper.Save();
                m_request.SendResponse(kOK);
                break;
            }
        case ShowerCode::kReset:
            {
                m_request.SendResponse(kOK);
                taskENTER_CRITICAL();
                IWDG_ReloadCounter();
                NVIC_SystemReset(); 		// (!) Бесконечный цикл внутри.
                break;
            }
        case ShowerCode::kPing:
            {
                m_request.SendResponse(kPing);
                break;
            }
        case ShowerCode::kGetWaterPercent:
            {
                m_request.SendResponse(g_waterLevelTask.GetPercent());
                break;
            }
        case ShowerCode::kGetAirTemp:
            {
                m_request.SendResponse(g_tempSensorTask.GetAirTemp());
                break;
            }
        case ShowerCode::kGetAverageAirTemp:
            {
                m_request.SendResponse(g_tempSensorTask.GetAverageAirTemp());
                break;
            }
        case ShowerCode::kGetInternalTemp:
            {
                m_request.SendResponse(g_tempSensorTask.GetInternalTemp());
                break;
            }
        case ShowerCode::kGetMinimumWaterHeatingLevel:
            {
                m_request.SendResponse(g_writeProperties.MinimumWaterHeatingPercent);
                break;
            }
        case ShowerCode::kSetMinimumWaterHeatingLevel:
            {
                if (request_length == sizeof(g_writeProperties.MinimumWaterHeatingPercent))
                {
                    g_writeProperties.MinimumWaterHeatingPercent = PropertyStruct::FixAbsoluteHeatingTimeLimitHours(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetHeatingTimeLimit:
            {
                m_request.SendResponse(g_writeProperties.HeatingTimeLimitMin);
                break;
            }
        case ShowerCode::kSetHeatingTimeLimit:
            {
                if (request_length == sizeof(g_writeProperties.HeatingTimeLimitMin))
                {
                    g_writeProperties.HeatingTimeLimitMin = PropertyStruct::FixAbsoluteHeatingTimeLimitHours(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetLightBrightness:
            {
                m_request.SendResponse(g_writeProperties.LightBrightness);
                break;
            }
        case ShowerCode::kSetLightBrightness:
            {
                if (request_length == sizeof(g_writeProperties.LightBrightness))
                {
                    g_writeProperties.LightBrightness = PropertyStruct::FixLightBrightness(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetTimeLeft:
            {
                uint8_t value = g_heatingTimeLeft.GetTimeLeftMin();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetHeatingTimeoutState:
            {
                bool value = g_heaterTask.GetTimeoutOccured();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetAbsoluteTimeoutStatus:
            {
                bool value = g_heaterTask.GetAbsoluteTimeoutOccured();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetHeaterEnabled:
            {
                bool enabled = Common::HeaterIsOn();
                m_request.SendResponse(enabled);
                break;
            }
        case ShowerCode::kGetAbsoluteHeatingTimeLimit:
            {
                m_request.SendResponse(g_writeProperties.AbsoluteHeatingTimeLimitHours);
                break;
            }
        case ShowerCode::kSetAbsoluteHeatingTimeLimit:
            {
                if (request_length == sizeof(g_writeProperties.AbsoluteHeatingTimeLimitHours))
                {
                    g_writeProperties.AbsoluteHeatingTimeLimitHours = PropertyStruct::FixAbsoluteHeatingTimeLimitHours(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWiFiPower:
            {
                m_request.SendResponse(g_writeProperties.WiFiPower);
                break;
            }
        case ShowerCode::kSetWiFiPower:
            {
                if (request_length == sizeof(g_writeProperties.WiFiPower))
                {
                    g_writeProperties.WiFiPower = PropertyStruct::FixWiFiPower(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWatchDogWasReset:
            {
                m_request.SendResponse(g_watchDogTask.GetWasReset());
                break;
            }
        case ShowerCode::kGetHasMainPower:
            {
                bool circuit_breaker_is_on = Common::CircuitBreakerIsOn();
                m_request.SendResponse(circuit_breaker_is_on);
                break;
            }
        case ShowerCode::kGetHeatingLimit:
            {
                uint8_t value = g_heaterTask.GetHeatingLimit();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetAverageInternalTemp:
            {
                m_request.SendResponse(g_tempSensorTask.GetAverageInternalTemp());
                break;
            }
        case ShowerCode::kSetCurAP:
            {
                if (request_length > 1)
                {
                    m_request.SendResponse(kOK);
                    InnerSetCurAP(m_requestData, request_length);
                }
                break;
            }
        case ShowerCode::kSetDefAP:
            {
                if (request_length > 1)
                {
                    m_request.SendResponse(kOK);
                    InnerSetDefAP(m_requestData, request_length);
                }
                break;
            }
        case ShowerCode::kGetHeatingProgress:
            {
                uint8_t value = g_heatingTimeLeft.GetProgress();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetWaterHeated:
            {
                bool value = g_heaterTask.GetIsWaterHeated();
                m_request.SendResponse(value);
                break;
            }
        case ShowerCode::kGetWaterLevelMeasureInterval:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelMeasureIntervalMsec);
                break;
            }
        case ShowerCode::kSetWaterLevelMeasureInterval:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelMeasureIntervalMsec))
                {
                    g_writeProperties.WaterLevelMeasureIntervalMsec = PropertyStruct::FixWaterLevelMeasureIntervalMsec(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWaterValveCutOffPercent:
            {
                m_request.SendResponse(g_writeProperties.WaterValveCutOffPercent);
                break;
            }
        case ShowerCode::kSetWaterValveCutOffPercent:
            {
                if (request_length == sizeof(g_writeProperties.WaterValveCutOffPercent))
                {
                    g_writeProperties.WaterValveCutOffPercent = PropertyStruct::FixWaterValveCutOffPercent(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWaterLevelMedianBufferSize:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelMedianFilterSize);
                break;
            }
        case ShowerCode::kSetWaterLevelMedianBufferSize:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelMedianFilterSize))
                {
                    g_writeProperties.WaterLevelMedianFilterSize = PropertyStruct::FixWaterLevelMedianFilterSize(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetWaterLevelAverageBufferSize:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelAvgFilterSize);
                break;
            }
        case ShowerCode::kSetWaterLevelAverageBufferSize:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelAvgFilterSize))
                {
                    g_writeProperties.WaterLevelAvgFilterSize = PropertyStruct::FixWaterLevelAvgFilterSize(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetTempSensorInternalTempAverageSize:
            {
                m_request.SendResponse(g_writeProperties.InternalTempAvgFilterSize);
                break;
            }
        case ShowerCode::kSetTempSensorInternalTempAverageSize:
            {
                if (request_length == sizeof(g_writeProperties.InternalTempAvgFilterSize))
                {
                    g_writeProperties.InternalTempAvgFilterSize = PropertyStruct::FixInternalTempAvgFilterSize(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetButtonTimeMsec:
            {
                m_request.SendResponse(g_writeProperties.ButtonPressTimeMsec);	
                break;
            }
        case ShowerCode::kSetButtonTimeMsec:
            {
                if (request_length == sizeof(g_writeProperties.ButtonPressTimeMsec))
                {
                    g_writeProperties.ButtonPressTimeMsec = PropertyStruct::FixButtonPressTimeMsec(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetButtonLongPressTimeMsec:
            {
                m_request.SendResponse(g_writeProperties.ButtonLongPressTimeMsec);	
                break;
            }
        case ShowerCode::kSetButtonLongPressTimeMsec:
            {
                if (request_length == sizeof(g_writeProperties.ButtonLongPressTimeMsec))
                {
                    g_writeProperties.ButtonLongPressTimeMsec = PropertyStruct::FixButtonLongPressTimeMsec(*(uint16_t*)m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;
            }
        case ShowerCode::kGetTempLowerBound:
            {
                m_request.SendResponse(kAirTempLowerBound);	
                break;
            }
        case ShowerCode::kGetTempUpperBound:
            {
                m_request.SendResponse(kAirTempUpperBound);	
                break;
            }
        case ShowerCode::kGetWaterTankVolumeLitre:
            {
                m_request.SendResponse(g_writeProperties.WaterTankVolumeLitre);
                break;	
            }
        case ShowerCode::kSetWaterTankVolumeLitre:
            {
                if (request_length == sizeof(g_writeProperties.WaterTankVolumeLitre))
                {
                    g_writeProperties.WaterTankVolumeLitre = PropertyStruct::FixWaterTankVolumeLitre(*(float*)m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;	
            }
        case ShowerCode::kGetWaterHeaterPowerKWatt:
            {
                m_request.SendResponse(g_writeProperties.WaterHeaterPowerKWatt);
                break;	
            }
        case ShowerCode::kSetWaterHeaterPowerKWatt:
            {
                if (request_length == sizeof(g_writeProperties.WaterHeaterPowerKWatt))
                {
                    g_writeProperties.WaterHeaterPowerKWatt = PropertyStruct::FixWaterHeaterPowerKWatt(*(float*)m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;	
            }
        case ShowerCode::kGetWaterLevelErrorThreshold:
            {
                m_request.SendResponse(g_writeProperties.WaterLevelErrorThreshold);
                break;	
            }
        case ShowerCode::kSetWaterLevelErrorThreshold:
            {
                if (request_length == sizeof(g_writeProperties.WaterLevelErrorThreshold))
                {
                    g_writeProperties.WaterLevelErrorThreshold = PropertyStruct::FixWaterLevelErrorThreshold(*m_requestData);
                    m_request.SendResponse(kOK);
                }
                break;	
            }
        default:
            {
                m_request.SendResponse(kUnknownCode);
                break;
            }
        }
        return true;
    }
};

extern WiFiTask g_wifiTask;
