#pragma once
#include "Stopwatch.h"

class WaterSensorButton final
{
public:
	
	WaterSensorButton(GPIO_TypeDef* gpio, uint16_t gpio_pin);
	bool IsOn();
	
private:
	
	const uint16_t m_gpio_pin = 0;
	GPIO_TypeDef* m_gpio = 0;
	bool m_isOn = false;
    bool m_pendingOn = false;
    bool m_pendingOff = false;
    Stopwatch m_debounce;
};

