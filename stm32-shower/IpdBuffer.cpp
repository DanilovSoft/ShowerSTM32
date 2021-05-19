#include "IpdBuffer.h"


void IpdBuffer::WriteByte(const uint8_t byte)
{
	if (_count < BUF_SZ)
	{
		_buf[_tail] = byte;
		_count++;
		_tail = (_tail + 1) % BUF_SZ;
	}
	else
	{
		// переполнение буфера
		Clear();
	}
}


ShowerCode IpdBuffer::Take(uint8_t* buf)
{
	if (_count > 0)
	{
		uint8_t length = _buf[_head];  // Payload Size
		_head = (_head + 1) % BUF_SZ;
			
		ShowerCode code = (ShowerCode)_buf[_head];
		_head = (_head + 1) % BUF_SZ;
			
		length -= 2; // Учесть размер хедера
		for (uint8_t i = 0; i < length; i++)
		{
			buf[i] = _buf[_head];
			_head = (_head + 1) % BUF_SZ;
		}
		_count -= (length + 2);
		return code;
	}
    return ShowerCode::None;
}
	

uint8_t IpdBuffer::GetRequestSize()
{
	if (_count)
	{
    	uint8_t payload_length = _buf[_head]; // В первом байте записан размер Payload
			
		// Payload не может быть меньше 2
		if (payload_length < 2)
		{
			_count--;
			_head = (_head + 1) % BUF_SZ;
			return 0;
		}
		return payload_length - 1;
	}
	return 0;
}


void IpdBuffer::Clear()
{
	_count = 0;
	_head = 0;
	_tail = 0;
}