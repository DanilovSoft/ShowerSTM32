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
        
        m_logicalIsOn = false;
        m_pendingOn = false;
        m_pendingOff = false;
        m_debounce.Reset();
    }
    
    void Update()
    {
        UpdateLogicalSwitch();
    }
    
    bool UpdateAndGet()
    {
        UpdateLogicalSwitch();
        return m_logicalIsOn;
    }

private:
    
    const ButtonPressedFunc m_buttonPressedFunc;
    bool m_logicalIsOn = false;
    bool m_pendingOn = false;
    bool m_pendingOff = false;
    Stopwatch m_debounce;
    
    void UpdateLogicalSwitch()
    {
        if (m_buttonPressedFunc())
        {
            if (m_logicalIsOn)
            {
                m_pendingOff = false;
            }
            else
            {
                if (m_pendingOn)
                {
                    if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMsec)
                    {
                        m_logicalIsOn = true;
                        m_pendingOn = false;
                    }
                }
                else
                {
                    m_pendingOn = true;
                    m_debounce.Reset();
                }
            }
        }
        else
        {
            if (m_logicalIsOn)
            {
                if (m_pendingOff)
                {
                    if (m_debounce.GetElapsedMsec() >= g_properties.ButtonPressTimeMsec)
                    {
                        m_logicalIsOn = false;
                        m_pendingOff = false;
                    }
                }
                else
                {
                    m_pendingOff = true;
                    m_debounce.Reset();
                }
            }
            else
            {
                m_pendingOn = false;
            }
        }
    }
};
