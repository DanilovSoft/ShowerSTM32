#pragma once
#include "stdlib.h"

class Debug final
{
public:
    
    static void Assert(bool condition)
    {
#if DEBUG
        __asm("bkpt 255");
        exit(0);  
#endif  
    }
};

