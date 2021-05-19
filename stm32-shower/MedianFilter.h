#pragma once
#include "stdint.h"
#include <cstdlib>

#define		STOPPER 0                                      /* Smaller than any datum */
#define		MEDIAN_FILTER_SIZE    (255)

class MedianFilter
{
    struct pair
    {
        struct pair   *point;                              /* Pointers forming list linked in sorted order */
        uint16_t  value; /* Values to sort */
    };
    struct pair buffer[MEDIAN_FILTER_SIZE] = { 0 }; /* Buffer of nwidth pairs */
    struct pair *datpoint = buffer; /* Pointer into circular buffer of data */
    struct pair small = { NULL, STOPPER }; /* Chain stopper */
    struct pair big = { &small, 0 }; /* Pointer to head (largest) of linked list.*/
    
public:
	uint16_t median_filter(uint16_t datum);
};

