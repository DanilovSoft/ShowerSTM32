#pragma once
#include "iActiveTask.h"

#define ANIM1 ('\x02')
#define ANIM2 ('\x03')
#define ANIM3 ('/')
#define ANIM4 ('-')

class WaterLevelAnimTask : public iActiveTask
{
    volatile char _cur;
    uint8_t _pos = 0;
    void Init();
    void Run();
    void Pause();
public:
    char GetChar();
};

extern WaterLevelAnimTask _waterLevelAnimTask;