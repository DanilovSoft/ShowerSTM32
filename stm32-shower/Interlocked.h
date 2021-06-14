#pragma once
#include "core_cm3.h"
#include "stdint.h"

class Interlocked final
{
public:
    
    inline static bool CompareExchange(uint32_t* ptr, uint32_t old_value, uint32_t new_value)
    {
        if (__LDREXW(ptr) == old_value)
        {
            return __STREXW(new_value, ptr) == 0;
        }

        __CLREX();  // снять блокировку после __LDREXW
        return false;
    }

    inline static uint32_t Increment(uint32_t* ptr)
    {
        uint32_t old_value;
        uint32_t new_value;
        do
        {
            old_value = __LDREXW(ptr);
            new_value = old_value + 1;
        } while (__STREXW(new_value, ptr));
        return new_value;
    }

    inline static uint32_t Decrement(uint32_t* ptr)
    {
        uint32_t old_value;
        uint32_t new_value;
        do
        {
            old_value = __LDREXW(ptr);
            new_value = old_value - 1;
        } while (__STREXW(new_value, ptr));
        return new_value;
    }

    inline static uint32_t Read(uint32_t* ptr)
    {
        uint32_t value = __LDREXW(ptr);
        __CLREX();
        return value;
    }

    inline static uint32_t Add(uint32_t* ptr, uint32_t value)
    {
        uint32_t old_value;
        uint32_t new_value;
        do
        {
            old_value = __LDREXW(ptr);
            new_value = old_value + value;
        } while (__STREXW(new_value, ptr));
        return new_value;
    }
};
