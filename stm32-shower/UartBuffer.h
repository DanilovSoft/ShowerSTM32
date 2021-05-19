#pragma once
#include "Interlocked.h"

class UartBuffer
{
	const static uint16_t BUF_SZ = 500;
	unsigned char _buf[BUF_SZ];
	uint32_t _count;  	        //счетчик символов
	uint16_t _head;   			//"указатель" головы буфера
	uint16_t _tail;   				//"указатель" хвоста буфера

public:

	void PutChar(unsigned char ch)
	{
		if (Interlocked::Read(&_count) < BUF_SZ)
		{
			_buf[_tail] = ch;
			Interlocked::Increment(&_count);
			_tail++;
			if (_tail == BUF_SZ)
				_tail = 0;
		}
	}

	bool GetChar(unsigned char &ch)
	{
		if (Interlocked::Read(&_count) > 0)
		{
			ch = _buf[_head];
			Interlocked::Decrement(&_count);
			_head++;
			if (_head == BUF_SZ)
				_head = 0;

			return true;
		}
		return false;
	}
};
