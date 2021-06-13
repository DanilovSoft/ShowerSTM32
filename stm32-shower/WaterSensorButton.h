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
        m_considerIsOn = false;
        m_pendingOn = false;
        m_pendingOff = false;
        m_debounce.Reset();
    }
    
    void Update()
    {
        UpdateConsiderIsOn();
    }
    
    bool UpdateAndGet()
    {
        UpdateConsiderIsOn();
        return m_considerIsOn;
    }

private:
    
    const uint16_t kGpioPin;
    GPIO_TypeDef* m_gpio;
    Stopwatch m_debounce;
    bool m_considerIsOn;
    bool m_pendingOn;
    bool m_pendingOff;
    
    void UpdateConsiderIsOn()
    {
        if (GPIO_ReadInputDataBit(m_gpio, kGpioPin))
        {
            if (!m_considerIsOn)
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
                        m_considerIsOn = true;
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
            if (m_considerIsOn)
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
                        m_considerIsOn = false;
                        m_pendingOff = false;
                    }
                }
            }
            else
            {
                m_pendingOn = false;
            }
        }
    }
};
