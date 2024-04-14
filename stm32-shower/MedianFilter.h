#pragma once
#include "Properties.h"

class MedianFilter final
{
public:

    void Init(const uint8_t filter_size)
    {
        m_medianFilterSize = filter_size;
    }
    
    uint16_t AddValue(uint16_t datum)
    {
        struct Pair *successor;                             // Pointer to successor of replaced data item.
        struct Pair *scan;                                   // Pointer used to scan down the sorted list.
        struct Pair *scanold;                                // Previous value of scan.
        struct Pair *median;                                 // Pointer to median.
        uint16_t i;
    
        if (datum == kStopper)
        {
            datum = kStopper + 1; // No stoppers allowed.
        }

        if ((++m_datpoint - m_buffer) >= m_medianFilterSize)
        {
            m_datpoint = m_buffer; // Increment and wrap data in pointer.
        }

        m_datpoint->Value = datum; // Copy in new datum.
        successor = m_datpoint->Point; // Save pointer to old value's successor.
        median = &m_big; // Median initially to first in chain.
        scanold = NULL; // Scanold initially null.
        scan = &m_big; // Points to pointer to first (largest) datum in chain.

        // Handle chain-out of first item in chain as special case
        if (scan->Point == m_datpoint)
        {
            scan->Point = successor;
        }
        scanold = scan; // Save this pointer and
        scan = scan->Point; // step down chain.

        // Loop through the chain, normal loop exit via break.
        for (i = 0; i < m_medianFilterSize; ++i)
        {
            // Handle odd-numbered item in chain.
            if (scan->Point == m_datpoint)
            {
                scan->Point = successor; // Chain out the old datum.
            }

            if (scan->Value < datum)						  // If datum is larger than scanned value,
            {
                m_datpoint->Point = scanold->Point; // Chain it in here.
                scanold->Point = m_datpoint; // Mark it chained in.
                datum = kStopper;
            }

            // Step median pointer down chain after doing odd-numbered element.
            median = median->Point; // Step median pointer.
            if (scan == &m_small)
            {
                break;									  // Break at end of chain.
            }
            scanold = scan; // Save this pointer and
            scan = scan->Point; // step down chain.

            // Handle even-numbered item in chain.
            if (scan->Point == m_datpoint)
            {
                scan->Point = successor;
            }

            if (scan->Value < datum)
            {
                m_datpoint->Point = scanold->Point;
                scanold->Point = m_datpoint;
                datum = kStopper;
            }

            if (scan == &m_small)
            {
                break;
            }

            scanold = scan;
            scan = scan->Point;
        }
     
        return median->Value;
    }

private:

    struct Pair
    {
        // Pointers forming list linked in sorted order.
        struct Pair* Point;
        
        // Values to sort.
        uint16_t Value;
    };

    static constexpr uint16_t kStopper = 0;  // Smaller than any datum.
    
    uint8_t m_medianFilterSize;
    
    // Buffer of nwidth pairs.
    struct Pair m_buffer[kWaterLevelMedianMaxSize] = { 0 };

    // Pointer into circular buffer of data.
    struct Pair* m_datpoint = m_buffer;

    // Chain stopper.
    struct Pair m_small = { NULL, kStopper };
    
    // Pointer to head (largest) of linked list.
    struct Pair m_big = { &m_small, 0 };
};
