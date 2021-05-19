#pragma once
#include "iActiveTask.h"

class EepromTask : public iActiveTask
{
    void Run() { }
    
    void Init() { }
};

extern EepromTask eepromTask;