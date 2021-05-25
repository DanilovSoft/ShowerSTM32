#pragma once
#include "Request.h"
#include "iActiveTask.h"

#define WIFI_GPIO               (GPIOB)
#define WIFI_GPIO_CH_PD_Pin     (GPIO_Pin_1)
#define GPIO_WPS                (GPIOB)
#define GPIO_WPS_Pin            (GPIO_Pin_12)
#define  WIFI_INIT_TRY_COUNT    (3)

//  AT+UART_DEF=57600,8,1,0,0
//	AT+CWMODE_DEF=1
//	AT+CWDHCP_DEF=1,1		// Включить DHCP
//	AT+CWJAP_DEF="AcessPoint","Pa$$w0rd",\"d1:cb:5d:12:37:bb\"

class WiFi : public iActiveTask
{
private:
	
	Request _req;
    uint8_t _data[256] = { };
    
    bool InitWiFi();
    bool TryInitWiFi();
    bool WPS();
    void SetAP(const char* pref, uint8_t prefSize, uint8_t* data, uint8_t length);
    void _SetCurAP(uint8_t* data, uint8_t length);
    void _SetDefAP(uint8_t* data, uint8_t length);
    bool DoEvents();
    void Init();
    void Run();
};

extern WiFi _wifiTask;