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
        pressed = true; // кнопка нажата
        canPress = false; // кнопка сработала и её нельзя нажимать повторно
    }
    
    void Released()
	{
    	if (pressed)
        // кнопка была нажата а сейчас её отпустили
    	{
        	pressed = false; // запомнить что кнопка не нажата
        	counter.Restart();    // начать отсчет времени когда отпустили кнопку
    	}
    	else
    	{
        	if (!canPress)
        	{
            	if (counter.TimeOut(Button_Debounce_TimeMsec))
            	{
                	canPress = true; // время прошло, кнопку разрешено нажимать повторно.
            	}
        	}
    	}
	}
};

