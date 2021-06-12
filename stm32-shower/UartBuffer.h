#pragma once
#include "Interlocked.h"

class UartBuffer final
{
public:

	void PutChar(unsigned char ch)
	{
		if (Interlocked::Read(&m_count) < kBufSize)
		{
			m_buf[m_tail] = ch;
			Interlocked::Increment(&m_count);
			m_tail++;
			if (m_tail == kBufSize)
				m_tail = 0;
		}
	}

	bool GetChar(unsigned char &ch)
	{
		if (Interlocked::Read(&m_count) > 0)
		{
			ch = m_buf[m_head];
			Interlocked::Decrement(&m_count);
			m_head++;
			if (m_head == kBufSize)
				m_head = 0;

			return true;
		}
		return false;
	}
	
private:
	static const uint16_t kBufSize = 500;
	unsigned char m_buf[kBufSize];
	uint32_t m_count;  	        // Счетчик символов.
	uint16_t m_head;   			// "Указатель" головы буфера.
	uint16_t m_tail;   			// "Указатель" хвоста буфера.
};
