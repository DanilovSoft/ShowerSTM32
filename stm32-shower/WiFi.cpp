#include "WiFi.h"
#include "Common.h"
#include "iActiveTask.h"
#include "Buzzer.h"
#include "Properties.h"
#include "HeaterTask.h"
#include "Eeprom.h"
#include "stm32f10x_iwdg.h"
#include "WatchDogTask.h"
#include "string.h"
#include "WaterLevelTask.h"
#include "HeatingTimeLeft.h"
#include "TempSensor.h"

WiFi g_wifiTask;

bool WiFi::InitWiFi()
{
	bool wpsButtonPressed = GPIO_ReadInputDataBit(GPIO_WPS, GPIO_WPS_Pin) == RESET;
		
	for (int8_t i = 0; i <= WIFI_INIT_TRY_COUNT; i++)
	{
		vTaskDelay(100 / portTICK_PERIOD_MS);
		GPIO_SetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
    	
		if (TryInitWiFi())
		{
			if (wpsButtonPressed)
			{
				if (!WPS())
				{
					return false;
				}
			}
			return true;
		}
			
		GPIO_ResetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
	}
		
	return false;
}
	

bool WiFi::TryInitWiFi()
{
	if (!g_uartStream.WaitLine("ready", 1000))     // ожидание инициализации wi-fi
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
		
	char rfpower[] = "AT+RFPOWER=\0\0\0\0";
	itoa(g_properties.WiFiPower, rfpower + 11);
	strcat(rfpower, "\r\n");
	
	g_uartStream.WriteLine(rfpower);		// 40..82, unit:0.25dBm
	
	if(!g_uartStream.WaitLine("OK", 300))
	{
		return false;
	}

	g_uartStream.WriteLine("AT+CIPMUX=1\r\n");     // разрешить множественные подключения.
	
	if(!g_uartStream.WaitLine("OK", 300))
	{
		return false;
	}

	g_uartStream.WriteLine("AT+CIPSERVER=1,333\r\n");     // запустить TCP сервер, порт 333.
	
	if (!g_uartStream.WaitLine("OK", 300))
	{
		return false;
	}
	
	//    	uartStream.WriteLine("AT+CWJAP_DEF=\"Miles\",\"PASSW0RD\",\"d4:ca:6d:11:38:af\"\r\n");
	//    	if (!uartStream.WaitLine("OK", 300))
	//        	return false;
	
	return true;
}
	

bool WiFi::WPS()
{
	const BeepSound samples[] = 
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
	

void WiFi::Init()
{
	// Кнопка WPS
	GPIO_InitTypeDef gpio_init = 
	{
		.GPIO_Pin = GPIO_WPS_Pin,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_IPU
	};
		
	GPIO_Init(GPIO_WPS, &gpio_init);
	GPIO_SetBits(GPIO_WPS, GPIO_WPS_Pin);
		
	// CH_PD
	gpio_init = 
	{
		.GPIO_Pin = WIFI_GPIO_CH_PD_Pin,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_Mode = GPIO_Mode_Out_PP
	};
	
	GPIO_Init(WIFI_GPIO, &gpio_init);
	GPIO_ResetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
}
	

void WiFi::SetAP(const char* pref, uint8_t prefSize, uint8_t* data, uint8_t length)
{
	prefSize -= 1;  // Не учитываем нуль-терминатор.
		
	char str[prefSize + length + 2 + 1];
	memcpy(str, pref, prefSize);
	memcpy(str + prefSize, data, length);
		
	str[prefSize + length] = '\r';
	str[prefSize + length + 1] = '\n';
	str[prefSize + length + 2] = 0;
    	
	g_uartStream.WriteLine(str);
}
	

void WiFi::_SetCurAP(uint8_t* data, uint8_t length)
{
	const char pref[] = "AT+CWJAP_CUR=";
	SetAP(pref, sizeof(pref), data, length);
}
	

void WiFi::_SetDefAP(uint8_t* data, uint8_t length)
{
	const char pref[] = "AT+CWJAP_DEF=";
	SetAP(pref, sizeof(pref), data, length);
}
	

void WiFi::Run()
{
	if (InitWiFi())
	{
		while (true)
		{
			if (!DoEvents())
			{
				taskYIELD();  // Нет запросов, можно отдать квант времени
			}
		}
	}
}


bool WiFi::DoEvents()
{
	uint8_t length;
	if (!m_request.GetRequestSize(length))
	{
		return false;
	}
    
	ShowerCode code = m_request.GetRequestData(m_rxData);
	switch (code)
	{
	case None:
	case UnknownCode:
	case OK:
		{
			break;
		}
	case GetWaterLevelEmpty:
		{
			m_request.SendResponse(g_writeProperties.WaterLevelEmpty);
			break;
		}
	case GetWaterLevelFull:
		{
			m_request.SendResponse(g_writeProperties.WaterLevelFull);
			break;
		}
	case SetWaterLevelFull:
		{
			if (length == 2)
			{
				g_writeProperties.WaterLevelFull = *(uint16_t*)m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case SetWaterLevelEmpty:
		{
			if (length == 2)
			{
				g_writeProperties.WaterLevelEmpty = *(uint16_t*)m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevel:
		{
			m_request.SendResponse(g_waterLevelTask.AvgUsec);
			break;
		}
	case GetWaterLevelRaw:
    	{
        	m_request.SendResponse(g_waterLevelTask.UsecRaw);
        	break;
    	}
	case GetTempChart:
		{
			m_request.SendResponse(&g_writeProperties.Chart, sizeof(g_writeProperties.Chart));
			break;
		}
	case SetTempChart:
		{
			if (length == sizeof(g_writeProperties.Chart))
			{
    			g_writeProperties.Chart.Parse(m_rxData);
    			g_properties.Chart.Parse(m_rxData); // установить значения в ОЗУ
				m_request.SendResponse(OK);
			}
			break;
		}
	case Save:
		{
			g_eeprom.Save();
			m_request.SendResponse(OK);
			break;
		}
	case Reset:
		{
			m_request.SendResponse(OK);
			taskENTER_CRITICAL();
			IWDG_ReloadCounter();
			NVIC_SystemReset();		// (!) Бесконечный цикл внутри.
			break;
		}
	case Ping:
		{
			m_request.SendResponse(Ping);
			break;
		}
	case GetWaterPercent:
		{
			m_request.SendResponse(g_waterLevelTask.DisplayingPercent);
			break;
		}
	case GetExternalTemp:
		{
			m_request.SendResponse(g_tempSensorTask.ExternalTemp);
			break;
		}
	case GetAverageExternalTemp:
		{
			m_request.SendResponse(g_tempSensorTask.AverageExternalTemp);
			break;
		}
	case GetInternalTemp:
		{
			m_request.SendResponse(g_tempSensorTask.InternalTemp);
			break;
		}
	case GetMinimumWaterHeatingLevel:
		{
			m_request.SendResponse(g_writeProperties.MinimumWaterHeatingPercent);
			break;
		}
	case SetMinimumWaterHeatingLevel:
		{
			if (length == 1)
			{
				g_writeProperties.MinimumWaterHeatingPercent = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetHeatingTimeLimit:
		{
			m_request.SendResponse(g_writeProperties.HeatingTimeLimitMin);
			break;
		}
	case SetHeatingTimeLimit:
		{
			if (length == 1)
			{
				g_writeProperties.HeatingTimeLimitMin = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetLightBrightness:
		{
			m_request.SendResponse(g_writeProperties.LightBrightness);
			break;
		}
	case SetLightBrightness:
		{
			if (length == 1)
			{
				g_writeProperties.LightBrightness = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetTimeLeft:
		{
			uint8_t value = g_heatingTimeLeft.GetTimeLeft();
			m_request.SendResponse(value);
			break;
		}
	case GetHeatingTimeoutState:
		{
			bool value = g_heaterTask.GetTimeoutOccured();
			m_request.SendResponse(value);
			break;
		}
	case GetAbsoluteTimeoutStatus:
		{
			bool value = g_heaterTask.GetAbsoluteTimeoutOccured();
			m_request.SendResponse(value);
			break;
		}
	case GetHeaterEnabled:
		{
			bool enabled = g_heaterTask.GetIsHeaterEnabled();
			m_request.SendResponse(enabled);
			break;
		}
	case GetAbsoluteHeatingTimeLimit:
		{
			m_request.SendResponse(g_writeProperties.AbsoluteHeatingTimeLimitHours);
			break;
		}
	case SetAbsoluteHeatingTimeLimit:
		{
			if (length == 1)
			{
				g_writeProperties.AbsoluteHeatingTimeLimitHours = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWiFiPower:
		{
			m_request.SendResponse(g_writeProperties.WiFiPower);
			break;
		}
	case SetWiFiPower:
		{
			if (length == 1)
			{
				g_writeProperties.WiFiPower = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWatchDogWasReset:
		{
			m_request.SendResponse(g_watchDogTask.GetWasReset());
			break;
		}
	case GetHasMainPower:
		{
			bool hasMainPower = CircuitBreakerIsOn();
			m_request.SendResponse(hasMainPower);
			break;
		}
	case GetHeatingLimit:
		{
			uint8_t value = g_heaterTask.GetHeatingLimit();
			m_request.SendResponse(value);
			break;
		}
	case GetAverageInternalTemp:
		{
			m_request.SendResponse(g_tempSensorTask.AverageInternalTemp);
			break;
		}
	case SetCurAP:
		{
			if (length > 1)
			{
				m_request.SendResponse(OK);
				_SetCurAP(m_rxData, length);
			}
			break;
		}
	case SetDefAP:
		{
			if (length > 1)
			{
				m_request.SendResponse(OK);
				_SetDefAP(m_rxData, length);
			}
			break;
		}
	case GetHeatingProgress:
		{
			uint8_t value = g_heatingTimeLeft.GetProgress();
			m_request.SendResponse(value);
			break;
		}
	case GetWaterHeated:
		{
			bool value = g_heaterTask.WaterHeated();
			m_request.SendResponse(value);
			break;
		}
	case GetWaterLevelMeasureInterval:
		{
			m_request.SendResponse(g_writeProperties.WaterLevel_Measure_IntervalMsec);
			break;
		}
	case SetWaterLevelMeasureInterval:
		{
			if (length == 1)
			{
				g_writeProperties.WaterLevel_Measure_IntervalMsec = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWaterValveCutOffPercent:
		{
			m_request.SendResponse(g_writeProperties.WaterValve_Cut_Off_Percent);
			break;
		}
	case SetWaterValveCutOffPercent:
		{
			g_writeProperties.WaterValve_Cut_Off_Percent = *m_rxData;
			m_request.SendResponse(OK);
			break;
		}
	case GetWaterLevelMedianBufferSize:
		{
			m_request.SendResponse(g_writeProperties.WaterLevel_Median_Buffer_Size);
			break;
		}
	case SetWaterLevelMedianBufferSize:
		{
			if (length == 1)
			{
				auto value = *m_rxData;
				g_writeProperties.WaterLevel_Median_Buffer_Size = value;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevelAverageBufferSize:
		{
			m_request.SendResponse(g_writeProperties.WaterLevel_Avg_Buffer_Size);
			break;
		}
	case SetWaterLevelAverageBufferSize:
		{
			if (length == 1)
			{
    			auto value = *m_rxData;
				g_writeProperties.WaterLevel_Avg_Buffer_Size = value;
    			m_request.SendResponse(OK);
			}
			break;
		}
	case GetTempSensorInternalTempAverageSize:
		{
			m_request.SendResponse(g_writeProperties.InternalTemp_Avg_Size);
			break;
		}
	case SetTempSensorInternalTempAverageSize:
		{
			if (length == 1)
			{
				g_writeProperties.InternalTemp_Avg_Size = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevelUsecPerDeg:
		{
			m_request.SendResponse((uint16_t)0);
			break;
		}
	case SetWaterLevelUsecPerDeg:
		{
			if (length == 2)
			{
				//_writeOnlyPropertiesStruct.WaterLevel_Usec_Per_Deg = *(uint16_t*)_data;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetWaterValveTimeoutSec:
		{
			m_request.SendResponse(g_writeProperties.WaterValve_TimeoutSec);	
			break;
		}
	case SetWaterValveTimeoutSec:
		{
			if (length == sizeof(g_writeProperties.WaterValve_TimeoutSec))
			{
				g_writeProperties.WaterValve_TimeoutSec = *m_rxData;
				m_request.SendResponse(OK);
			}
			break;
		}
	case GetButtonTimeMsec:
    	{
        	m_request.SendResponse(g_writeProperties.ButtonPressTimeMs);	
        	break;
    	}
	case SetButtonTimeMsec:
    	{
        	if (length == sizeof(g_writeProperties.ButtonPressTimeMs))
        	{
            	g_writeProperties.ButtonPressTimeMs = *m_rxData;
            	m_request.SendResponse(OK);
        	}
        	break;
    	}
	case GetTempLowerBound:
    	{
        	m_request.SendResponse(LOWER_BOUND);	
        	break;
    	}
	case GetTempUpperBound:
    	{
        	m_request.SendResponse(UPPER_BOUND);	
        	break;
    	}
	case GetWaterTankVolumeLitre:
		{
			m_request.SendResponse(g_writeProperties.WaterTankVolumeLitre);
			break;	
		}
	case SetWaterTankVolumeLitre:
		{
			if (length == sizeof(g_writeProperties.WaterTankVolumeLitre))
			{
				g_writeProperties.WaterTankVolumeLitre = *(float*)m_rxData;
				m_request.SendResponse(OK);
			}
			break;	
		}
	case GetWaterHeaterPowerKWatt:
		{
			m_request.SendResponse(g_writeProperties.WaterHeaterPowerKWatt);
			break;	
		}
	case SetWaterHeaterPowerKWatt:
		{
			if (length == sizeof(g_writeProperties.WaterHeaterPowerKWatt))
			{
				g_writeProperties.WaterHeaterPowerKWatt = *(float*)m_rxData;
				m_request.SendResponse(OK);
			}
			break;	
		}
	default:
		{
			m_request.SendResponse(UnknownCode);
			break;
		}
	}
	return true;
}