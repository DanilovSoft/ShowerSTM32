#include "WaterSensorButton.h"
#include "Properties.h"


WaterSensorButton::WaterSensorButton(GPIO_TypeDef* gpio, uint16_t gpio_pin)
	: m_gpio(gpio)
	, m_gpio_pin(gpio_pin)
{
	m_isOn = false;
	m_pendingOn = false;
	m_pendingOff = false;
	m_debounce.Reset();
}

bool WaterSensorButton::IsOn()
{
	if (GPIO_ReadInputDataBit(m_gpio, m_gpio_pin))
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
                if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMs)
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
                if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMs)
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
