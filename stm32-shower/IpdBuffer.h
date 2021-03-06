#pragma once
#include "stdint.h"
#include "ShowerCode.h"

// '+IPD' = Получение сетевых данных.
class IpdBuffer final
{
public:

    void WriteByte(const uint8_t byte)
    {
        if (m_count < kBufSize)
        {
            m_buf[m_tail] = byte;
            m_count++;
            m_tail = (m_tail + 1) % kBufSize;
        }
        else
        {
            // Переполнение буфера.
            Clear();
        }
    }

    ShowerCode Take(uint8_t* buffer)
    {
        if (m_count > 0)
        {
            uint8_t length = m_buf[m_head];   // Payload Size.
            m_head = (m_head + 1) % kBufSize;
            
            ShowerCode code = (ShowerCode)m_buf[m_head];
            m_head = (m_head + 1) % kBufSize;
            
            length -= 2;  // Учесть размер хедера.
            for(uint8_t i = 0 ; i < length ; i++)
            {
                buffer[i] = m_buf[m_head];
                m_head = (m_head + 1) % kBufSize;
            }
            m_count -= (length + 2);
            return code;
        }
        return ShowerCode::kNone;
    }
    
    uint8_t GetRequestSize()
    {
        if (m_count)
        {
            uint8_t payload_length = m_buf[m_head];  // В первом байте записан размер Payload.
            
            // Payload не может быть меньше 2.
            if(payload_length < 2)
            {
                m_count--;
                m_head = (m_head + 1) % kBufSize;
                return 0;
            }
            return payload_length - 1;
        }
        return 0;
    }

    void Clear()
    {
        m_count = 0;
        m_head = 0;
        m_tail = 0;
    }

private:

    static const uint8_t kBufSize = 200;
    uint8_t m_buf[kBufSize] = {0};
    uint8_t m_count = 0;
    uint8_t m_head = 0;
    uint8_t m_tail = 0;
};
