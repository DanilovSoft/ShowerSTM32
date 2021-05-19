#pragma once
#include "stdint.h"
#include "ButtonDebounce.h"
#include "Properties.h"

class FrontPanelButton
{
    TickCounter _timeCounter;
    bool _pressed = false;
    bool _canPress = true;
    GPIO_TypeDef* _gpio = 0;
    uint16_t _gpio_pin = 0;
    
public:
    FrontPanelButton(GPIO_TypeDef* grio, uint16_t gpio_pin);   // ctor
    bool IsPressed();
    bool IsLongPressed();
    /*void Reset();
    uint16_t GetTime();*/
};