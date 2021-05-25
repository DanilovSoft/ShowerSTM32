#pragma once
#include "stdint.h"
#include "UartStream.h"
#include "ShowerCode.h"

class Request
{
private:
	
	uint8_t _connectionId;

public:

	bool GetRequestSize(uint8_t &request_size)
    {
    	request_size = _uartStream.GetRequestSize(_connectionId); // RequestSize это первый байт из +IPD
	    if (request_size)
	    {
		    --request_size;
		    return true;
	    }

	    request_size = _uartStream.GetRequest(_connectionId);
	    if (request_size)
	    {
		    --request_size;
			return true;
	    }
	    return false;
    }

	ShowerCode GetRequestData(uint8_t* buf)
	{
		return _uartStream.ReadRequest(_connectionId, buf);
	}
	
	WaitStatus SendResponse(const void* data, uint16_t length)
    {
    	WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)data, length);
	    return status;
    }

	WaitStatus SendResponse(const uint8_t value)
	{
		WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(const uint16_t value)
	{
		WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&value, sizeof(value));
		return status;
	}
	
    WaitStatus SendResponse(const int16_t value)
    {
        WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&value, sizeof(value));
        return status;
    }
    
	WaitStatus SendResponse(const float value)
	{
		WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&value, sizeof(value));
		return status;
	}
	
	WaitStatus SendResponse(const bool value)
	{
		WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(ShowerCode code)
	{
		WaitStatus status = _uartStream.SendResponse(_connectionId, (const char*)&code, 1);
		return status;
	}
};
