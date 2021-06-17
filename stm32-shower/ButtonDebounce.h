#pragma once
#include "stdint.h"
#include "Stopwatch.h"

class ButtonDebounce final
{
public:

    ButtonDebounce(ButtonPressedFunc button_pressed_func, uint16_t press_time_msec, uint16_t release_time_msec)
        : m_pressTimeMsec(press_time_msec)
        , m_releaseTimeMsec(release_time_msec),
        m_buttonPressedFunc(button_pressed_func)
    {
        m_stopwatch.Reset();
        m_physicallyPressed = false;
        m_canPressAgain = true;
        m_consideredPressed = false;
    }
    
    void Update()
    {
        m_consideredPressed = UpdatePressConsider();
    }
    
    bool GetConsideredPressed()
    {
        return m_consideredPressed;
    }
    
    bool UpdateAndGet()
    {
        m_consideredPressed = UpdatePressConsider();
        return m_consideredPressed;
    }
    
private:
    
    const ButtonPressedFunc m_buttonPressedFunc;
    const uint16_t m_pressTimeMsec;
    const uint16_t m_releaseTimeMsec;
    Stopwatch m_stopwatch;
    bool m_physicallyPressed;
    // Для гистерезиса — кнопка не должна срабатывать повторно пока 
    // её не отпустят на какое-то время.
    bool m_canPressAgain;
    bool m_consideredPressed;
    
    // Возвращает True если разрешено выполнять действие кнопки.
    bool UpdatePressConsider()
    {   
        if (m_buttonPressedFunc())
        {
            // Кнопка физически зажата.
             
            if(m_canPressAgain)
            {
                // Нажатие на кнопку разрешено.
                
                if(!m_physicallyPressed)
                {
                    // Кнопка была в отпущенном состоянии.
                        
                    // Фиксируем что кнопка нажата.
                    m_physicallyPressed = true;
                    
                    // Начать отсчет времени зажатой кнопки.
                    m_stopwatch.Reset();
                }
                else
                {
                    // Кнопка должна быть зажата некоторое время.
                    if(m_stopwatch.GetElapsedMsec() >= m_pressTimeMsec)
                    {
                        // Запретить нажатие на кнопку.
                        m_canPressAgain = false;
                        
                        // Разрешено разово выполнить действие кнопки.
                        return true;
                    }
                }
            }
            else
            {
                // Произошло нажатие но оно еще запрещено.
                
                // Начать отсчет времени отпущенной кнопки заново.
                m_stopwatch.Reset();
            }
        }
        else
        {
            // Кнопка отпущена.
                
            if(m_physicallyPressed)
            {
                // Кнопка была в нажатом состоянии.
                        
                // Фиксируем что кнопка отпущена.
                m_physicallyPressed = false;
            
                // Начать отсчет времени отпущенной кнопки.
                m_stopwatch.Reset();
            }
            else
            {
                // Кнопка все еще отпущена.
                        
                if(!m_canPressAgain)
                {
                    // Нажатие на кнопку запрещено.
                                
                    // Кнопка должна быть отпущена некоторое время.
                    if(m_stopwatch.GetElapsedMsec() >= m_releaseTimeMsec)
                    {   
                        // Кнопка отпущена достаточно долго что-бы разрешить повторное нажатие.
                                    
                        // Разрешить повторное нажатие на кнопку.
                        m_canPressAgain = true;
                    }
                }
            }
        }
    
        // Действие кнопки выполнять запрещено.
        return false;
    }
};
