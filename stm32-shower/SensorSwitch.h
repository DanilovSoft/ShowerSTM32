#pragma once
#include "stm32f10x_gpio.h"
#include "TickCounter.h"

class SensorSwitch
{
    bool _isOn = false;
    bool _pendingOn = false;
    bool _pendingOff = false;
    TickCounter _counter;
    GPIO_TypeDef* _grio = 0;
    uint16_t _gpio_pin = 0;
    
public:
    SensorSwitch(GPIO_TypeDef* grio, uint16_t gpio_pin);
    ~SensorSwitch();
    bool IsOn();
};

