#pragma once
#include "TickCounter.h"

#define Button_Debounce_TimeMsec		(10)

class ButtonDebounce
{
	bool pressed = false;
	bool canPress = true;
	TickCounter counter;
	
public:
	
    bool CanPress()
    {
        return canPress;
    }
	
    void Pressed()
    {
        pressed = true; // ������ ������
        canPress = false; // ������ ��������� � � ������ �������� ��������
    }
    
    void Released()
	{
    	if (pressed)
        // ������ ���� ������ � ������ � ���������
    	{
        	pressed = false; // ��������� ��� ������ �� ������
        	counter.Restart();    // ������ ������ ������� ����� ��������� ������
    	}
    	else
    	{
        	if (!canPress)
        	{
            	if (counter.TimeOut(Button_Debounce_TimeMsec))
            	{
                	canPress = true; // ����� ������, ������ ��������� �������� ��������.
            	}
        	}
    	}
	}
};

