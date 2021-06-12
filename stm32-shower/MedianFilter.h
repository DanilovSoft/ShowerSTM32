#pragma once
#include "Properties.h"

class MedianFilter final
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

	const static uint16_t kStopper = 0;     // Smaller than any datum.
	
	// Buffer of nwidth pairs.
    struct Pair m_buffer[WL_MEDIAN_BUF_MAX_SIZE] = { 0 };

	// Pointer into circular buffer of data.
    struct Pair *m_datpoint = m_buffer;

	// Chain stopper.
	struct Pair m_small = { NULL, kStopper };
    
	// Pointer to head (largest) of linked list.
	struct Pair m_big = { &m_small, 0 };
    
	uint8_t m_medianFilterSize;
};
