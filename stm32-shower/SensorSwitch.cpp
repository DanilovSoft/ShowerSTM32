#include "SensorSwitch.h"
#include "Properties.h"


SensorSwitch::SensorSwitch(GPIO_TypeDef* grio, uint16_t gpio_pin)
{
    _grio = grio;
    _gpio_pin = gpio_pin;

    _isOn = false;
    _pendingOn = false;
    _pendingOff = false;
    _counter.Reset();
}

SensorSwitch::~SensorSwitch()
{
}

bool SensorSwitch::IsOn()
{
    if (GPIO_ReadInputDataBit(_grio, _gpio_pin))
    {
        if (!_isOn)
        {
            if (!_pendingOn)
            {
                _pendingOn = true;
                _counter.Reset();
            }
            else
            {
                if (_counter.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
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
                _counter.Reset();
            }
            else
            {
                if (_counter.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
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
