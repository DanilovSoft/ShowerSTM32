#pragma once
#include "Request.h"
#include "iActiveTask.h"

#define WIFI_INIT_TRY_COUNT     (3)

//  AT+UART_DEF=57600,8,1,0,0
//	AT+CWMODE_DEF=1
//	AT+CWDHCP_DEF=1,1		// Включить DHCP
//	AT+CWJAP_DEF="AcessPoint","Pa$$w0rd",\"d1:cb:5d:12:37:bb\"

class WiFi : public iActiveTask
{
private:
	
	Request m_request;
    uint8_t m_rxData[256] = { };
    
    void Init();
    void Run();
    bool InitWiFi();
    bool TryInitWiFi();
    bool WPS();
    void SetAP(const char* pref, uint8_t prefSize, uint8_t* data, uint8_t length);
    void _SetCurAP(uint8_t* data, uint8_t length);
    void _SetDefAP(uint8_t* data, uint8_t length);
    bool DoEvents();
};

extern WiFi g_wifiTask;