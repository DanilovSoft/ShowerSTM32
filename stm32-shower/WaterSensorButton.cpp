#include "WaterSensorButton.h"
#include "Properties.h"


WaterSensorButton::WaterSensorButton(GPIO_TypeDef* gpio, uint16_t gpio_pin)
	: _gpio(gpio)
	, _gpio_pin(gpio_pin)
{
	_isOn = false;
	_pendingOn = false;
	_pendingOff = false;
	_debounce.Reset();
}

WaterSensorButton::~WaterSensorButton()
{
}

bool WaterSensorButton::IsOn()
{
	if (GPIO_ReadInputDataBit(_gpio, _gpio_pin))
    {
        if (!_isOn)
        {
            if (!_pendingOn)
            {
                _pendingOn = true;
                _debounce.Reset();
            }
            else
            {
                if (_debounce.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
                {
                    _isOn = true;
                    _pendingOn = false;
                }
            }
        }
        else
        {
            _pendingOff = false;
        }
    }
    else
    {
        if (_isOn)
        {
            if (!_pendingOff)
            {
                _pendingOff = true;
                _debounce.Reset();
            }
            else
            {
                if (_debounce.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
                {
                    _isOn = false;
                    _pendingOff = false;
                }
            }
        }
        else
        {
            _pendingOn = false;
        }
    }
    return _isOn;
}
