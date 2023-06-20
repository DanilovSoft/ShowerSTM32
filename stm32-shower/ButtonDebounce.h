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
    }
    
    // Возвращает true если логическая кнопка считается нажатой.
    bool IsPressed()
    {
        UpdateState();
        return m_state == ButtonDebounce::LogicalPressed;
    }
    
private:
    
    enum State
    {
        WaitingPhysicalPress,
        AccumulatingPhysicalPress,
        LogicalPressed,
        AccumulatingReset // Отсчитывыем времени отпущенной кнопки.
    }; 
    
    const ButtonPressedFunc m_physicalButtonPressedFunc;
    const uint16_t m_pressTimeMsec;
    const uint16_t m_releaseTimeMsec;
    Stopwatch m_stopwatch;
    State m_state = State::WaitingPhysicalPress;
    
    void UpdateState()
    {   
        if (m_physicalButtonPressedFunc()) // Кнопка физически зажата.
        {
            switch (m_state)
            {
            case WaitingPhysicalPress:
                {
                    m_state = ButtonDebounce::AccumulatingPhysicalPress;
                    m_stopwatch.Reset(); // Начать отсчет времени зажатой кнопки.
                }
                break;
            case AccumulatingPhysicalPress:
                {
                    if (m_stopwatch.GetElapsedMsec() >= m_pressTimeMsec) // Кнопка должна быть зажата некоторое время.
                    {
                        m_state = ButtonDebounce::LogicalPressed;
                    }
                }
                break;
            case LogicalPressed:
                {
                    m_state = ButtonDebounce::AccumulatingReset;
                    m_stopwatch.Reset(); // Начать отсчет времени отпущенной кнопки.
                }
                break;
            case AccumulatingReset: // Произошло нажатие но оно еще запрещено гистерезисом.
                {
                    m_stopwatch.Reset(); // Заново считаем время отпущенной кнопки.
                }
                break;
            default:
                break;
            }
        }
        else // Физическая кнопка отпущена.
        {
            switch (m_state)
            {
            case WaitingPhysicalPress:
                break;
            case AccumulatingPhysicalPress: // Отпустили кнопку слишком рано или дребезг контактов?
                {
                    m_state = ButtonDebounce::WaitingPhysicalPress;
                }
                break;
            case LogicalPressed:
                {
                    m_state = ButtonDebounce::AccumulatingReset;
                    m_stopwatch.Reset(); // Начать отсчет времени отпущенной кнопки.
                }
                break;
            case AccumulatingReset:
                {
                    if (m_stopwatch.GetElapsedMsec() >= m_releaseTimeMsec) // Кнопка должна быть отпущена некоторое время.
                    {
                        m_state = ButtonDebounce::WaitingPhysicalPress; // Кнопка отпущена достаточно долго что-бы разрешить повторное нажатие.
                    }
                }
                break;
            default:
                break;
            }
        }
    }
};
