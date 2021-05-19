#pragma once
#include "stdint.h"

class Stopwatch
{
    uint32_t _startValue = 0;
    uint32_t _stopValue = 0;
    
public:
    static void Initialize();
    void Start();
    void Stop();
    uint32_t GetElapsed();
};

