#pragma once
#include "stdint.h"
#include "TickCounter.h"

class ButtonDebounce
{
public:
	
	ButtonDebounce(GPIO_TypeDef* grio, uint16_t gpio_pin, uint8_t pressTimeMs);     // ctor
	bool IsPressed();
	
private:
	
	uint8_t _pressTimeMs;
    TickCounter _stopwatch;
    bool _pressed = false;
    // Для гистерезиса — кнопка не должна срабатывать повторно пока 
    // её не отпустят на какое-то время (используем то-же самое значение антидребезга для экономии параметра).
    bool _canPressAgain = true;
    GPIO_TypeDef* _gpio = 0;
    uint16_t _gpioPin = 0;
};