#pragma once
#include "stdint.h"
#include "UartStream.h"
#include "ShowerCode.h"

class Request
{
	uint8_t _connection_id;

public:

	bool GetRequestSize(uint8_t &request_size)
    {
    	request_size = uartStream.GetRequestSize(_connection_id); // RequestSize это первый байт из +IPD
	    if (request_size)
	    {
		    --request_size;
		    return true;
	    }

	    request_size = uartStream.GetRequest(_connection_id);
	    if (request_size)
	    {
		    --request_size;
			return true;
	    }
	    return false;
    }

	ShowerCode GetRequestData(uint8_t* buf)
	{
		return uartStream.ReadRequest(_connection_id, buf);
	}
	
	WaitStatus SendResponse(const void* data, uint16_t length)
    {
    	WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)data, length);
	    return status;
    }

	WaitStatus SendResponse(const uint8_t value)
	{
		WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(const uint16_t value)
	{
		WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&value, sizeof(value));
		return status;
	}
	
    WaitStatus SendResponse(const int16_t value)
    {
        WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&value, sizeof(value));
        return status;
    }
    
	WaitStatus SendResponse(const float value)
	{
		WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&value, sizeof(value));
		return status;
	}
	
	WaitStatus SendResponse(const bool value)
	{
		WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&value, 1);
		return status;
	}
	
	WaitStatus SendResponse(ShowerCode code)
	{
		WaitStatus status = uartStream.SendResponse(_connection_id, (const char*)&code, 1);
		return status;
	}
};
