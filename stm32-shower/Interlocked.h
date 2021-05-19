#pragma once
#include "core_cm3.h"
#include "stdint.h"

class Interlocked
{
public:
	inline static bool CompareExchange(uint32_t* ptr, uint32_t oldValue, uint32_t newValue)
	{
		if (__LDREXW(ptr) == oldValue)
		{
			return __STREXW(newValue, ptr) == 0;
		}

		__CLREX();  // снять блокировку после __LDREXW
		return false;
	}

	inline static uint32_t Increment(uint32_t* ptr)
	{
		uint32_t oldValue;
		uint32_t newValue;
		do
		{
			oldValue = __LDREXW(ptr);
			newValue = oldValue + 1;
		} while (__STREXW(newValue, ptr));
		return newValue;
	}

	inline static uint32_t Decrement(uint32_t* ptr)
	{
		uint32_t oldValue;
		uint32_t newValue;
		do
		{
			oldValue = __LDREXW(ptr);
			newValue = oldValue - 1;
		} while (__STREXW(newValue, ptr));
		return newValue;
	}

	inline static uint32_t Read(uint32_t* ptr)
	{
		uint32_t value = __LDREXW(ptr);
		__CLREX();
		return value;
	}

	inline static uint32_t Add(uint32_t* ptr, uint32_t value)
	{
		uint32_t oldValue;
		uint32_t newValue;
		do
		{
			oldValue = __LDREXW(ptr);
			newValue = oldValue + value;
		} while (__STREXW(newValue, ptr));
		return newValue;
	}
};