#pragma once
#include "Stopwatch.h"
#include "Properties.h"

class WaterSensorButton final
{
public:
	
	WaterSensorButton(GPIO_TypeDef* gpio, uint16_t gpio_pin)
		: m_gpio(gpio)
		, kGpioPin(gpio_pin)
	{
		m_isOn = false;
		m_pendingOn = false;
		m_pendingOff = false;
		m_debounce.Reset();
	}
		
	bool IsOn()
	{
		if (GPIO_ReadInputDataBit(m_gpio, kGpioPin))
		{
			if (!m_isOn)
			{
				if (!m_pendingOn)
				{
					m_pendingOn = true;
					m_debounce.Reset();
				}
				else
				{
					if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMsec)
					{
						m_isOn = true;
						m_pendingOn = false;
					}
				}
			}
			else
			{
				m_pendingOff = false;
			}
		}
		else
		{
			if (m_isOn)
			{
				if (!m_pendingOff)
				{
					m_pendingOff = true;
					m_debounce.Reset();
				}
				else
				{
					if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMsec)
					{
						m_isOn = false;
						m_pendingOff = false;
					}
				}
			}
			else
			{
				m_pendingOn = false;
			}
		}
		return m_isOn;
	}

private:
	
	const uint16_t kGpioPin = 0;
	GPIO_TypeDef* m_gpio = 0;
	bool m_isOn = false;
    bool m_pendingOn = false;
    bool m_pendingOff = false;
    Stopwatch m_debounce;
};
