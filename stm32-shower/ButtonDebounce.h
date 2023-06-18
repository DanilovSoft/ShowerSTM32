#pragma once
#include "stdint.h"
#include "Stopwatch.h"

class ButtonDebounce final
{
public:

    ButtonDebounce(const ButtonPressedFunc button_pressed_func, uint16_t press_time_msec, uint16_t release_time_msec)
        : m_pressTimeMsec(press_time_msec)
        , m_releaseTimeMsec(release_time_msec),
        m_physicalButtonPressedFunc(button_pressed_func)
    {
        m_stopwatch.Reset();
        m_lastPhysicalButtonPressed = false;
        m_allowLogickButtonPress = true;
        m_logicalButtonPressed = false;
    }
    
    // Возвращает true если логическая кнопка считается нажатой.
    bool IsPressed()
    {
        UpdateLogicPress();
        return m_logicalButtonPressed;
    }
    
private:
    
    const ButtonPressedFunc m_physicalButtonPressedFunc;
    const uint16_t m_pressTimeMsec;
    const uint16_t m_releaseTimeMsec;
    Stopwatch m_stopwatch;
    bool m_lastPhysicalButtonPressed; // Мы должны помнить последнее состояние физической кнопки.
    // Для гистерезиса — кнопка не должна срабатывать повторно пока её не отпустят на какое-то время.
    bool m_allowLogickButtonPress;
    bool m_logicalButtonPressed;
    
    void UpdateLogicPress()
    {   
        if (m_physicalButtonPressedFunc()) // Кнопка физически зажата.
        { 
            if (m_allowLogickButtonPress) // Нажатие на кнопку разрешено.
            {
                if (m_lastPhysicalButtonPressed) // В прошлый раз физическая кнопка была в нажатом состоянии.
                {
                    if (m_stopwatch.GetElapsedMsec() >= m_pressTimeMsec) // Кнопка должна быть зажата некоторое время.
                    {
                        m_allowLogickButtonPress = false; // Запретить нажатие на кнопку.
                        m_logicalButtonPressed = true; // Считаем что логическая кнопка нажата.
                    }
                }
                else // Физическая кнопка была в отпущенном состоянии.
                {
                    m_lastPhysicalButtonPressed = true; // Запоминаем что физическая кнопка нажата.
                    m_stopwatch.Reset(); // Начать отсчет времени зажатой кнопки.
                }
            }
            else // Произошло нажатие но оно еще запрещено гистерезисом.
            {
                m_stopwatch.Reset(); // Заново считаем время отпущенной кнопки.
            }
        }
        else // Физическая кнопка отпущена.
        {
            if (m_lastPhysicalButtonPressed) // В прошлый раз физическая кнопка была в нажатом состоянии.
            {
                m_lastPhysicalButtonPressed = false; // Запоминаем что физическая кнопка отпущена.
                m_stopwatch.Reset(); // Начать отсчет времени отпущенной кнопки.
            }
            else // В прошлый раз физическая кнопка тоже была отпущена.
            {           
                if (!m_allowLogickButtonPress) // Нажатие на кнопку ещё запрещено гистерезисом.
                {
                    if (m_stopwatch.GetElapsedMsec() >= m_releaseTimeMsec) // Кнопка должна быть отпущена некоторое время.
                    {   
                        // Кнопка отпущена достаточно долго что-бы разрешить повторное нажатие.
                        m_allowLogickButtonPress = true; // Разрешить повторное нажатие на кнопку.
                    }
                }
            }
        }
    
        m_logicalButtonPressed = false; // Считаем что логическая кнопка отпущена.
    }
};
