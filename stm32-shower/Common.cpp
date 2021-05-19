#include "Common.h"
#include "string.h"


uint8_t DigitsCount(uint16_t value)
{
	uint8_t count = 0;

	do
	{
		value /= 10;
		count++;
	} while (value);

	return count;
}


char itoa(uint8_t value)
{
    const char digits[] = "0123456789";

    char ch = '0';
    if (value <= 9)
    {
        ch = digits[value];
    }
    return ch;
}


char* itoa(uint16_t number, char* str)
{
	const char digit[] = "0123456789";
	char* p = str;
//	if (i < 0)
//	{
//		*p++ = '-';
//		i *= -1;
//	}
	int32_t shifter = number;
	do
	{     //Move to where representation ends
		++p;
		shifter = shifter / 10;
	} while (shifter);
	*p = '\0';
	do
	{     //Move back, inserting digits as u go
		*--p = digit[number % 10];
		number = number / 10;
	} while (number);
	return str;
}


bool streql(const char* str1, const char* str2)
{
	return strcmp(str1, str2) == 0;
}


uint16_t abs(uint8_t a, uint8_t b)
{
	return a > b ? a - b : b - a;
}


uint16_t abs(uint16_t a, uint16_t b)
{
	return a > b ? a - b : b - a;
}


uint32_t abs(uint32_t a, uint32_t b)
{
	return a > b ? a - b : b - a;
}


//uint16_t atoi(const char* str)
//{
//    uint16_t result = 0;
//    char ch = *str;
//    while (ch != '\0')
//    {
//        result = result * 10 + (ch - '0');
//        ch = * ++str;
//    }
//    return result;
//}


unsigned char ctoi(const char ch)
{
    return ch - 48;
}

bool ArrayEquals(uint8_t* array1, uint8_t arr1Size, uint8_t* array2, uint8_t arr2Size)
{
	if (arr1Size != arr2Size) 
	{
		return false;
	}
	
	for (int i = 0; i < arr1Size; ++i) 
	{
		if (array1[i] != array2[i])
		{
			return false;
		}
	}
	return true;
}

//Passed arrays store different data types
template <typename T, typename U, int size1, int size2> bool Equal(T(&arr1)[size1], U(&arr2)[size2])
{
	return false;
}

//Passed arrays store SAME data types
template <typename T, int size1, int size2> bool Equal(T(&arr1)[size1], T(&arr2)[size2]) 
{
	if (size1 == size2) 
	{
		for (int i = 0; i < size1; ++i) 
		{
			if (arr1[i] != arr2[i]) return false;
		}
		return true;
	}
	return false;
}