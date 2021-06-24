#pragma once
#include "Stopwatch.h"
#include "Properties.h"

class WaterSensorButton final
{
public:
    
    WaterSensorButton(const ButtonPressedFunc button_pressed_func)
        : m_buttonPressedFunc(button_pressed_func)
    {
        Debug::Assert(g_properties.Initialized);
        
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
    
    const ButtonPressedFunc m_buttonPressedFunc;
    bool m_considerIsOn = false;
    bool m_pendingOn = false;
    bool m_pendingOff = false;
    Stopwatch m_debounce;
    
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
