#include "MedianFilter.h"

void MedianFilter::Init(const uint8_t medianFilterSize)
{
	_medianFilterSize = medianFilterSize;
}

uint16_t MedianFilter::AddValue(uint16_t datum)
{
	struct Pair *successor;                             // Pointer to successor of replaced data item.
	struct Pair *scan;                                  // Pointer used to scan down the sorted list.
	struct Pair *scanold;                               // Previous value of scan.
	struct Pair *median;                                // Pointer to median.
	uint16_t i;

	if (datum == STOPPER)
	{
		datum = STOPPER + 1;                            // No stoppers allowed.
	}

	if ((++_datpoint - _buffer) >= _medianFilterSize)
	{
		_datpoint = _buffer;                            // Increment and wrap data in pointer.
	}

	_datpoint->Value = datum;                           // Copy in new datum.
	successor = _datpoint->Point;                       // Save pointer to old value's successor.
	median = &_big;                                     // Median initially to first in chain.
	scanold = NULL;										// Scanold initially null.
	scan = &_big;                                       // Points to pointer to first (largest) datum in chain.

	// Handle chain-out of first item in chain as special case
	if (scan->Point == _datpoint)
	{
		scan->Point = successor;
	}
	scanold = scan;                                     // Save this pointer and
	scan = scan->Point;									// step down chain.

	// Loop through the chain, normal loop exit via break.
	for (i = 0; i < _medianFilterSize; ++i)
	{
	    // Handle odd-numbered item in chain.
		if (scan->Point == _datpoint)
		{
			scan->Point = successor;                      // Chain out the old datum.
		}

		if (scan->Value < datum)						  // If datum is larger than scanned value,
		{
			_datpoint->Point = scanold->Point;            // Chain it in here.
			scanold->Point = _datpoint;                   // Mark it chained in.
			datum = STOPPER;
		}

		// Step median pointer down chain after doing odd-numbered element.
		median = median->Point;                       // Step median pointer.
		if (scan == &_small)
		{
			break;									  // Break at end of chain.
		}
		scanold = scan;                               // Save this pointer and
		scan = scan->Point;                           // step down chain.

		// Handle even-numbered item in chain.
		if (scan->Point == _datpoint)
		{
			scan->Point = successor;
		}

		if (scan->Value < datum)
		{
			_datpoint->Point = scanold->Point;
			scanold->Point = _datpoint;
			datum = STOPPER;
		}

		if (scan == &_small)
		{
			break;
		}

		scanold = scan;
		scan = scan->Point;
	}
	return median->Value;
}