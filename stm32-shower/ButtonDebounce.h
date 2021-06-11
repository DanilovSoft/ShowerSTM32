#pragma once
#include "stdint.h"
#include "Stopwatch.h"

class ButtonDebounce
{
public:
	
	ButtonDebounce(GPIO_TypeDef* gpio, uint16_t gpio_pin, uint8_t pressTimeMs, uint16_t releaseTimeMs);
	bool IsPressed();
	
private:
	
	const uint8_t _pressTimeMs;
	const uint8_t _releaseTimeMs;
    const uint16_t _gpioPin = 0;
    GPIO_TypeDef* _gpio = 0;
    Stopwatch _stopwatch;
    bool _pressed = false;
    // Для гистерезиса — кнопка не должна срабатывать повторно пока 
    // её не отпустят на какое-то время.
    bool _canPressAgain = true;
};