#pragma once
#include "ShowerCode.h"
#include "ConnectionsBuffer.h"
#include "Common.h"

typedef enum { NOTHING = -1, TIMEOUT = 0, STR1 = 1, STR2 = 2, STR3 = 3 } WaitStatus;

class UartStream
{
private:
    
    /* Буффер в который копируем данные из RX_FIFO_BUF */
	uint8_t _buf[RX_FIFO_SZ] = { };
    char _str_buf[UART_MAX_STR_LEN] = { };  // Буфер для записи строк из кольцевого буффера _buf
	ConnectionsBuffer _connectionBuffer;
	uint16_t _count = 0;
	uint16_t _head = 0;
	uint16_t _head2 = RX_FIFO_SZ;
	uint16_t _tail = 0;
    
	bool CopyRingBuf();
	void LineReceived(const char* str, const uint8_t strLength);
	bool HandleIpd();
	bool HandleLine();
	uint16_t GetAbsoluteOffset(uint16_t offset);
	bool GetLineLength(uint16_t &count); // Возвращает количество байт до символов \r\n
	uint8_t CopyLine(char* str, const uint16_t count); // Копирует в строку str данные из кольцевого буффера _buf и смещает указатель _head
	bool Find(const char ch, const uint16_t offset, uint16_t &position); // Возвращает индекс первого вхождения строки начиная от 0
	bool IsIpd(); // Проверяет начинается ли кольцевой буффер _buf со строки '+IPD'
	bool StartWith(const char* data, const uint16_t count);
	bool WaitSpecific(const char* data, const uint16_t count, const uint16_t timeoutMsec);
	WaitStatus ReadLine(const char* str1, const char* str2, const char* str3);
	uint8_t ReadIpd(uint8_t &connection_id);
	void DMAWriteData(const char* data, const uint16_t count);
public:
	void Init();
	WaitStatus WaitLine(const char* str1, const char* str2, const char* str3, uint16_t timeoutMsec);
	WaitStatus WaitLine(const char* str1, const char* str2, uint16_t timeoutMsec);
	WaitStatus WaitLine(const char* str1, uint16_t timeoutMsec);
	void WriteLine(const char* str);
	void WriteData(const char* data, const uint16_t count);
	WaitStatus SendResponse(const uint8_t connection_id, const char* data, const uint16_t count);
	uint8_t GetRequestSize(uint8_t &connection_id);
	ShowerCode ReadRequest(const uint8_t connection_id, uint8_t* buf);
	uint8_t GetRequest(uint8_t &connection_id);
};

extern UartStream uartStream;