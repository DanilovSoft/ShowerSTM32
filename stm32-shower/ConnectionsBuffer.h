#pragma once
#include "IpdBuffer.h"

class ConnectionsBuffer final
{
public:
    
    ConnectionsBuffer()
    {
        m_nextConnectionId = 0;
    }
    
    IpdBuffer* operator[](const uint8_t index)
    {
        return &m_buf[index];
    }

    void Clear(const uint8_t connection_id)
    {
        if (connection_id < kIpdBufSize)
        {
            m_buf[connection_id].Clear();
        }
    }

    void ClearAll()
    {
        for (uint8_t i = 0; i < kIpdBufSize; i++)
        {
            m_buf[i].Clear();
        }
    }

    uint8_t GetRequestSize(uint8_t &connection_id)
    {
        for (uint8_t i = 0; i < kIpdBufSize; i++)
        {
            MoveNext();
            uint8_t request_length = m_buf[m_nextConnectionId].GetRequestSize();
            if (request_length)
            {
                connection_id = m_nextConnectionId;
                return request_length;
            }
        }
        return 0;
    }
    
private:
    static constexpr auto kIpdBufSize = 5;
    IpdBuffer m_buf[kIpdBufSize];
    uint8_t m_nextConnectionId;

    void MoveNext()
    {
        m_nextConnectionId = (m_nextConnectionId + 1) % kIpdBufSize;
    }
};
