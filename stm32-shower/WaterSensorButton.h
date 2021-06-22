#pragma once
#include "Stopwatch.h"
#include "Properties.h"

class WaterSensorButton final
{
public:
    
    WaterSensorButton(ButtonPressedFunc button_pressed_func, const PropertyStruct* const properties)
        : m_buttonPressedFunc(button_pressed_func)
        , m_properties(properties)
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
    
    const PropertyStruct* const m_properties;
    const ButtonPressedFunc m_buttonPressedFunc;
    Stopwatch m_debounce;
    bool m_considerIsOn;
    bool m_pendingOn;
    bool m_pendingOff;
    
    void UpdateConsiderIsOn()
    {
        if (m_buttonPressedFunc())
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
                    if (m_debounce.GetElapsedMsec() >= m_properties->ButtonPressTimeMsec)
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
                    if (m_debounce.GetElapsedMsec() >= m_properties->ButtonPressTimeMsec)
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
