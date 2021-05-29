#pragma once
#include "stdint.h"
#include <cstdlib>

constexpr auto STOPPER = 0;   // Smaller than any datum.
constexpr auto WL_MEDIAN_BUF_MAX_SIZE = 255; // Максимально допустимый размер медианного фильтра для уровня воды.

class MedianFilter
{
public:

	void Init(const uint8_t medianFilterSize);
	uint16_t AddValue(uint16_t datum);

private:

    struct Pair
    {
	    // Pointers forming list linked in sorted order.
        struct Pair *Point;
	    
	    // Values to sort.
        uint16_t Value;
    };
	
	// Buffer of nwidth pairs.
    struct Pair _buffer[WL_MEDIAN_BUF_MAX_SIZE] = { 0 };

	// Pointer into circular buffer of data.
    struct Pair *_datpoint = _buffer;

	// Chain stopper.
	struct Pair _small = { NULL, STOPPER };
    
	// Pointer to head (largest) of linked list.
	struct Pair _big = { &_small, 0 };
    
	uint8_t _medianFilterSize;
};

