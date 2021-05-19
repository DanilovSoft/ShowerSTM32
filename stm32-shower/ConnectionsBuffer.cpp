#include "ConnectionsBuffer.h"

void ConnectionsBuffer::MoveNext()
{
	_nextConnectionId = (_nextConnectionId + 1) % IPD_BUF_SZ;
}

IpdBuffer* ConnectionsBuffer::operator[](const uint8_t index)
{
	return &_buf[index];
}

void ConnectionsBuffer::Clear(const uint8_t connection_id)
{
	if (connection_id < IPD_BUF_SZ)
	{
		_buf[connection_id].Clear();
	}
}

void ConnectionsBuffer::ClearAll()
{
	for (uint8_t i = 0; i < IPD_BUF_SZ; i++)
	{
		_buf[i].Clear();
	}
}

uint8_t ConnectionsBuffer::GetRequestSize(uint8_t &connection_id)
{
	for (uint8_t i = 0; i < IPD_BUF_SZ; i++)
	{
		MoveNext();
		uint8_t request_length = _buf[_nextConnectionId].GetRequestSize();
		if (request_length)
		{
			connection_id = _nextConnectionId;
			return request_length;
		}
	}
	return 0;
}