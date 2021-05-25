#pragma once
#include "stdint.h"
#include "Properties.h"
#include "TickCounter.h"

class FrontPanelButton
{
private:
	
    TickCounter _timeCounter;
    bool _pressed = false;
    bool _canPress = true;
    GPIO_TypeDef* _gpio = 0;
    uint16_t _gpio_pin = 0;
    
public:
	
    FrontPanelButton(GPIO_TypeDef* grio, uint16_t gpio_pin);   // ctor
    bool IsPressed();
    bool IsLongPressed();
};