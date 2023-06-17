#pragma once
#include "stdint.h"
#include "Stopwatch.h"

class SensorPatternPress final
{
public:

    SensorPatternPress(const ButtonPressedFunc button_pressed_func)
        : m_physicalButtonPressedFunc(button_pressed_func)
    {
        m_stopwatch.Reset();
        m_lastPhysicalButtonPressed = false;
        m_allowLogickButtonPress = true;
        m_logicalButtonPressed = false;
    }
    
    void Update()
    {
        UpdateLogicPress();
    }
    
    bool GetLogicalPressed()
    {
        return m_logicalButtonPressed;
    }
    
    bool UpdateAndGet()
    {
        UpdateLogicPress();
        return m_logicalButtonPressed;
    }
    
private:
    
    const ButtonPressedFunc m_physicalButtonPressedFunc;
    Stopwatch m_stopwatch;
    bool m_lastPhysicalButtonPressed; // Мы должны помнить последнее состояние физической кнопки.
    // Для гистерезиса — кнопка не должна срабатывать повторно пока её не отпустят на какое-то время.
    bool m_allowLogickButtonPress;
    bool m_logicalButtonPressed;
    
    void UpdateLogicPress()
    {   
        if (m_physicalButtonPressedFunc()) // Кнопка физически зажата.
        { 
            
        }
        else // Физическая кнопка отпущена.
        {
            
        }
    }
};