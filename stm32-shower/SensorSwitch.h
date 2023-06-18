#pragma once
#include "Stopwatch.h"
#include "Properties.h"

class SensorSwitch final
{
public:
    
    SensorSwitch()
    {
        Debug::Assert(g_properties.Initialized);
        
        m_logicalIsOn = false;
        m_pendingOn = false;
        m_pendingOff = false;
        m_debounce.Reset();
    }

    bool IsOn()
    {
        UpdateLogicalSwitch();
        return m_logicalIsOn;
    }
    
    void PowerOff()
    {
        Common::PowerOffSensorSwitch(); // Выключить сенсор.
    }
    
    void PowerOn()
    {
        Common::PowerOnSensorSwitch();
    }
    
private:
    
    bool m_logicalIsOn = false;
    bool m_pendingOn = false;
    bool m_pendingOff = false;
    Stopwatch m_debounce;
    
    void UpdateLogicalSwitch()
    {
        if (Common::ButtonSensorSwitchIsOn())
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
