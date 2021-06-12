#pragma once
#include "stdint.h"
#include "UartStream.h"
#include "ShowerCode.h"

class Request final
{
public:

	bool GetRequestSize(uint8_t &request_size)
    {
    	request_size = g_uartStream.GetRequestSize(m_connectionId); // RequestSize это первый байт из +IPD
	    if (request_size)
	    {
		    --request_size;
		    return true;
	    }

	    request_size = g_uartStream.GetRequest(m_connectionId);
	    if (request_size)
	    {
		    --request_size;
			return true;
	    }
	    return false;
    }

	ShowerCode GetRequestData(uint8_t* buf)
	{
		return g_uartStream.ReadRequest(m_connectionId, buf);
	}
	
	WaitStatus SendResponse(const void* data, uint16_t length)
    {
    	WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)data, length);
	    return status;
    }

	WaitStatus SendResponse(const uint8_t value)
	{
		WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(const uint16_t value)
	{
		WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&value, sizeof(value));
		return status;
	}
	
    WaitStatus SendResponse(const int16_t value)
    {
        WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&value, sizeof(value));
        return status;
    }
    
	WaitStatus SendResponse(const float value)
	{
		WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&value, sizeof(value));
		return status;
	}
	
	WaitStatus SendResponse(const bool value)
	{
		WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(ShowerCode code)
	{
		WaitStatus status = g_uartStream.SendResponse(m_connectionId, (const char*)&code, 1);
		return status;
	}

private:
	
	uint8_t m_connectionId;
};
