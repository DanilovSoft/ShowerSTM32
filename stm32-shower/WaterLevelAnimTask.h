#pragma once
#include "iActiveTask.h"

#define ANIM1 ('\x02') // '\'
#define ANIM2 ('\x03') // '|'
#define ANIM3 ('/')
#define ANIM4 ('-')

class WaterLevelAnimTask final : public iActiveTask
{
public:
	
	char GetChar();
	
private:
	
    uint8_t m_pos = 0;
    volatile char m_cur;
    void Init();
    void Run();
    void Pause();
};

extern WaterLevelAnimTask g_waterLevelAnimTask;
