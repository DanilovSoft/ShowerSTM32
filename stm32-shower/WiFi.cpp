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

WiFi _wifiTask;

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
	if (!uartStream.WaitLine("ready", 1000))     // ожидание инициализации wi-fi
	{
		return false;
	}
    	
	//    	uartStream.WriteLine("AT+UART_DEF=57600,8,1,0,0\r\n");
	//		if (!uartStream.WaitLine("OK", 200))
	//			return false;
		
	/* Выключить эхо */
	uartStream.WriteLine("ATE0\r\n");
	
	if (!uartStream.WaitLine("OK", 300))
	{
		return false;
	}
		
	uartStream.WriteLine("AT+CWMODE_CUR=1\r\n");
	
	if (!uartStream.WaitLine("OK", 300))
	{
		return false;
	}
		
	char rfpower[] = "AT+RFPOWER=\0\0\0\0";
	itoa(Properties.WiFiPower, rfpower + 11);
	strcat(rfpower, "\r\n");
	
	uartStream.WriteLine(rfpower);		// 40..82, unit:0.25dBm
	
	if(!uartStream.WaitLine("OK", 300))
	{
		return false;
	}

	uartStream.WriteLine("AT+CIPMUX=1\r\n");     // разрешить множественные подключения.
	
	if(!uartStream.WaitLine("OK", 300))
	{
		return false;
	}

	uartStream.WriteLine("AT+CIPSERVER=1,333\r\n");     // запустить TCP сервер, порт 333.
	
	if (!uartStream.WaitLine("OK", 300))
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
	buzzer.Beep(samples, sizeof(samples) / sizeof(*samples));

			//			uartStream.WriteLine("AT+CWQAP");     // забыть сохраненную точку доступа
			//			if (!uartStream.WaitLine("OK", 100))
			//				return false;

	uartStream.WriteLine("AT+WPS=1\r\n");
	if (!uartStream.WaitLine("OK", 500))
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
    	
	uartStream.WriteLine(str);
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
	if (!_req.GetRequestSize(length))
	{
		return false;
	}
    
	ShowerCode code = _req.GetRequestData(_data);
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
			_req.SendResponse(_writeOnlyPropertiesStruct.WaterLevelEmpty);
			break;
		}
	case GetWaterLevelFull:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.WaterLevelFull);
			break;
		}
	case SetWaterLevelFull:
		{
			if (length == 2)
			{
				_writeOnlyPropertiesStruct.WaterLevelFull = *(uint16_t*)_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case SetWaterLevelEmpty:
		{
			if (length == 2)
			{
				_writeOnlyPropertiesStruct.WaterLevelEmpty = *(uint16_t*)_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevel:
		{
			_req.SendResponse(_waterLevelTask.AvgUsec);
			break;
		}
	case GetWaterLevelRaw:
    	{
        	_req.SendResponse(_waterLevelTask.UsecRaw);
        	break;
    	}
	case GetTempChart:
		{
			_req.SendResponse(&_writeOnlyPropertiesStruct.Chart, sizeof(_writeOnlyPropertiesStruct.Chart));
			break;
		}
	case SetTempChart:
		{
			if (length == sizeof(_writeOnlyPropertiesStruct.Chart))
			{
    			_writeOnlyPropertiesStruct.Chart.Parse(_data);
    			Properties.Chart.Parse(_data); // установить значения в ОЗУ
				_req.SendResponse(OK);
			}
			break;
		}
	case Save:
		{
			_eeprom.Save();
			_req.SendResponse(OK);
			break;
		}
	case Reset:
		{
			_req.SendResponse(OK);
			taskENTER_CRITICAL();
			IWDG_ReloadCounter();
			NVIC_SystemReset();		// (!) Бесконечный цикл внутри.
			break;
		}
	case Ping:
		{
			_req.SendResponse(Ping);
			break;
		}
	case GetWaterPercent:
		{
			_req.SendResponse(_waterLevelTask.DisplayingPercent);
			break;
		}
	case GetExternalTemp:
		{
			_req.SendResponse(_tempSensorTask.ExternalTemp);
			break;
		}
	case GetAverageExternalTemp:
		{
			_req.SendResponse(_tempSensorTask.AverageExternalTemp);
			break;
		}
	case GetInternalTemp:
		{
			_req.SendResponse(_tempSensorTask.InternalTemp);
			break;
		}
	case GetMinimumWaterHeatingLevel:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.MinimumWaterHeatingPercent);
			break;
		}
	case SetMinimumWaterHeatingLevel:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.MinimumWaterHeatingPercent = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetHeatingTimeLimit:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.HeatingTimeLimitMin);
			break;
		}
	case SetHeatingTimeLimit:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.HeatingTimeLimitMin = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetLightBrightness:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.LightBrightness);
			break;
		}
	case SetLightBrightness:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.LightBrightness = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetTimeLeft:
		{
			uint8_t value = _heatingTimeLeft.GetTimeLeft();
			_req.SendResponse(value);
			break;
		}
	case GetHeatingTimeoutState:
		{
			bool value = _heaterTask.GetTimeoutOccured();
			_req.SendResponse(value);
			break;
		}
	case GetAbsoluteTimeoutStatus:
		{
			bool value = _heaterTask.GetAbsoluteTimeoutOccured();
			_req.SendResponse(value);
			break;
		}
	case GetHeaterEnabled:
		{
			bool enabled = _heaterTask.GetIsHeaterEnabled();
			_req.SendResponse(enabled);
			break;
		}
	case GetAbsoluteHeatingTimeLimit:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.AbsoluteHeatingTimeLimitHours);
			break;
		}
	case SetAbsoluteHeatingTimeLimit:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.AbsoluteHeatingTimeLimitHours = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWiFiPower:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.WiFiPower);
			break;
		}
	case SetWiFiPower:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.WiFiPower = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWatchDogWasReset:
		{
			_req.SendResponse(_watchDogTask.WasReset);
			break;
		}
	case GetHasMainPower:
		{
			bool hasMainPower = HasMainPower();
			_req.SendResponse(hasMainPower);
			break;
		}
	case GetHeatingLimit:
		{
			uint8_t value = _heaterTask.GetHeatingLimit();
			_req.SendResponse(value);
			break;
		}
	case GetAverageInternalTemp:
		{
			_req.SendResponse(_tempSensorTask.AverageInternalTemp);
			break;
		}
	case SetCurAP:
		{
			if (length > 1)
			{
				_req.SendResponse(OK);
				_SetCurAP(_data, length);
			}
			break;
		}
	case SetDefAP:
		{
			if (length > 1)
			{
				_req.SendResponse(OK);
				_SetDefAP(_data, length);
			}
			break;
		}
	case GetHeatingProgress:
		{
			uint8_t value = _heatingTimeLeft.GetProgress();
			_req.SendResponse(value);
			break;
		}
	case GetWaterHeated:
		{
			bool value = _heaterTask.WaterHeated();
			_req.SendResponse(value);
			break;
		}
	case GetWaterLevelMeasureInterval:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterLevel_Measure_IntervalMsec);
			break;
		}
	case SetWaterLevelMeasureInterval:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.Customs.WaterLevel_Measure_IntervalMsec = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWaterValveCutOffPercent:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterValve_Cut_Off_Percent);
			break;
		}
	case SetWaterValveCutOffPercent:
		{
			_writeOnlyPropertiesStruct.Customs.WaterValve_Cut_Off_Percent = *_data;
			_req.SendResponse(OK);
			break;
		}
	case GetWaterLevelMedianBufferSize:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterLevel_Median_Buffer_Size);
			break;
		}
	case SetWaterLevelMedianBufferSize:
		{
			if (length == 1)
			{
				auto value = *_data;
				_writeOnlyPropertiesStruct.Customs.WaterLevel_Median_Buffer_Size = value;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevelAverageBufferSize:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterLevel_Avg_Buffer_Size);
			break;
		}
	case SetWaterLevelAverageBufferSize:
		{
			if (length == 1)
			{
    			auto value = *_data;
				_writeOnlyPropertiesStruct.Customs.WaterLevel_Avg_Buffer_Size = value;
    			_req.SendResponse(OK);
			}
			break;
		}
	case GetTempSensorInternalTempAverageSize:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.InternalTemp_Avg_Size);
			break;
		}
	case SetTempSensorInternalTempAverageSize:
		{
			if (length == 1)
			{
				_writeOnlyPropertiesStruct.Customs.InternalTemp_Avg_Size = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWaterLevelUsecPerDeg:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterLevel_Usec_Per_Deg);
			break;
		}
	case SetWaterLevelUsecPerDeg:
		{
			if (length == 2)
			{
				_writeOnlyPropertiesStruct.Customs.WaterLevel_Usec_Per_Deg = *(uint16_t*)_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetWaterValveTimeoutSec:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.Customs.WaterValve_TimeoutSec);	
			break;
		}
	case SetWaterValveTimeoutSec:
		{
			if (length == sizeof(_writeOnlyPropertiesStruct.Customs.WaterValve_TimeoutSec))
			{
				_writeOnlyPropertiesStruct.Customs.WaterValve_TimeoutSec = *_data;
				_req.SendResponse(OK);
			}
			break;
		}
	case GetButtonTimeMsec:
    	{
        	_req.SendResponse(_writeOnlyPropertiesStruct.ButtonPressTimeMs);	
        	break;
    	}
	case SetButtonTimeMsec:
    	{
        	if (length == sizeof(_writeOnlyPropertiesStruct.ButtonPressTimeMs))
        	{
            	_writeOnlyPropertiesStruct.ButtonPressTimeMs = *_data;
            	_req.SendResponse(OK);
        	}
        	break;
    	}
	case GetTempLowerBound:
    	{
        	_req.SendResponse(LOWER_BOUND);	
        	break;
    	}
	case GetTempUpperBound:
    	{
        	_req.SendResponse(UPPER_BOUND);	
        	break;
    	}
	case GetWaterTankVolumeLitre:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.WaterTankVolumeLitre);
			break;	
		}
	case SetWaterTankVolumeLitre:
		{
			if (length == sizeof(_writeOnlyPropertiesStruct.WaterTankVolumeLitre))
			{
				_writeOnlyPropertiesStruct.WaterTankVolumeLitre = *(float*)_data;
				_req.SendResponse(OK);
			}
			break;	
		}
	case GetWaterHeaterPowerKWatt:
		{
			_req.SendResponse(_writeOnlyPropertiesStruct.WaterHeaterPowerKWatt);
			break;	
		}
	case SetWaterHeaterPowerKWatt:
		{
			if (length == sizeof(_writeOnlyPropertiesStruct.WaterHeaterPowerKWatt))
			{
				_writeOnlyPropertiesStruct.WaterHeaterPowerKWatt = *(float*)_data;
				_req.SendResponse(OK);
			}
			break;	
		}
	default:
		{
			_req.SendResponse(UnknownCode);
			break;
		}
	}
	return true;
}