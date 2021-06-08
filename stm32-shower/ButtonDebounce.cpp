#include "ButtonDebounce.h"

// ctor
ButtonDebounce::ButtonDebounce(GPIO_TypeDef* gpio, uint16_t gpio_pin)
{
    _gpio = gpio;
    _gpioPin = gpio_pin;
    _timeCounter.Reset();
}

bool ButtonDebounce::IsPressed()
{
    if (GPIO_ReadInputDataBit(_gpio, _gpioPin))
    // Кнопка физически зажата.
    {
        if (_canPressAgain)
        // Нажатие на кнопку разрешено.
        {
            if (!_pressed)
            // Кнопка была в отпущенном состоянии.
            {
                // Фиксируем что кнопка нажата.
                _pressed = true;
                    
                // Начать отсчет времени зажатой кнопки.
                _timeCounter.Reset();
            }
            else
            {
                // Кнопка должна быть зажата некоторое время.
                if (_timeCounter.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
                {
                    // Запретить нажатие на кнопку.
                    _canPressAgain = false;
                        
                    // Разрешено выполнить действие кнопки.
                    return true;
                }
            }
        }
        else
        // Произошло нажатие но оно еще запрещено.
        {
            // Начать отсчет времени отпущенной кнопки заново.
            _timeCounter.Reset();
        }
    }
    else
    // Кнопка отпущена.
    {
        if (_pressed)
        // Кнопка была в нажатом состоянии.
        {
            // Фиксируем что кнопка отпущена.
            _pressed = false;
            
            // Начать отсчет времени отпущенной кнопки.
            _timeCounter.Reset();
        }
        else
        // Кнопка все еще отпущена.
        {
            if (!_canPressAgain)
            // Нажатие на кнопку запрещено.
            {
                // Кнопка должна быть отпущена некоторое время.
                if (_timeCounter.GetElapsedMsec() >= Properties.ButtonPressTimeMs)
                // Кнопка отпущена достаточно времени что-бы разрешить повторное нажатие.
                {   
                    // Разрешить повторное нажатие на кнопку.
                    _canPressAgain = true;
                }
            }
        }
    }
    
    // Действие кнопки выполнять запрещено.
    return false;
}

bool ButtonDebounce::IsLongPressed()
{
	return false;
}
