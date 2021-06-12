#pragma once
#include "Request.h"
#include "iActiveTask.h"

class WiFiTask final : public iActiveTask
{
private:
	
	static const uint8_t kWiFiInitTryCount = 3;
	Request m_request;
    uint8_t m_rxData[256] = { }; // Буфер для данных входного запроса.
    
    void Init();
    void Run();
    bool InitWiFi();
    bool TryInitWiFi();
    bool WPS();
    void SetAP(const char* pref, uint8_t prefSize, uint8_t* data, uint8_t length);
    void InnerSetCurAP(uint8_t* data, uint8_t length);
    void InnerSetDefAP(uint8_t* data, uint8_t length);
    bool DoWiFiEvents();
};

extern WiFiTask g_wifiTask;
