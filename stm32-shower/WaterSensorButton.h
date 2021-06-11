#pragma once
#include "stm32f10x_gpio.h"
#include "Stopwatch.h"

class WaterSensorButton
{
public:
	
	WaterSensorButton(GPIO_TypeDef* gpio, uint16_t gpio_pin);
	~WaterSensorButton();
	bool IsOn();
	
private:
	
	const uint16_t _gpio_pin = 0;
	GPIO_TypeDef* _gpio = 0;
    
	bool _isOn = false;
    bool _pendingOn = false;
    bool _pendingOff = false;
    Stopwatch _debounce;
};

