#pragma once
#include "stdint.h"
#include "Stopwatch.h"
#include "Common.h"

class SensorPatternPress final
{
public:

    SensorPatternPress()
    {
        m_stopwatch.Reset();
        m_state = SensorPatternPress::Ready;
    }
    
    bool IsPatternMatch()
    {
        UpdateLogicPress();
        return m_state == SensorPatternPress::PatternMatch;
    }
    
    void Reset()
    {
        m_state = SensorPatternPress::BackToBegining;
    }
    
private:
    
    enum State
    {
        Ready,
        Step1,
        Step2,
        PatternMatch,
        BackToBegining // Когда сенсор включен но патерн отмерять не нужно.
    };
    
    const static uint16_t MinimumTimeMsec = 160;
    const static uint16_t MaximumTimeMsec = 400;
    const static uint16_t MaxDeviationMsec = 80;
    
    Stopwatch m_stopwatch;
    uint32_t m_lastTime;
    State m_state;
    
    void UpdateLogicPress()
    {
        if (Common::ButtonSensorSwitchIsOn())
        {
            switch (m_state)
            {
            case SensorPatternPress::Ready:
                {
                    m_state = SensorPatternPress::Step1;
                    m_stopwatch.Reset(); // Начинаем замерять первый этап.
                }
                break;
            case SensorPatternPress::Step2:
                {
                    auto step2Msec = m_stopwatch.GetElapsedMsec();

                    if (step2Msec < MinimumTimeMsec || step2Msec > MaximumTimeMsec || Common::abs(step2Msec, m_lastTime) > MaxDeviationMsec)
                    {
                        m_state = SensorPatternPress::BackToBegining;
                    }
                    else
                    {
                        m_state = SensorPatternPress::PatternMatch;
                    }
                }
                break;
            default:
                break;
            }
        }
        else // Сенсор выключен.
        {
            switch (m_state)
            {
            case SensorPatternPress::Step1:
                {
                    m_lastTime = m_stopwatch.GetElapsedMsec();

                    if (m_lastTime < MinimumTimeMsec || m_lastTime > MaximumTimeMsec)
                    {
                        m_state = SensorPatternPress::Ready;
                    }
                    else
                    {
                        m_state = SensorPatternPress::Step2;
                        m_stopwatch.Reset(); // Начинаем замерять второй этап.
                    }
                }
                break;
            case SensorPatternPress::PatternMatch:
                {
                    m_state = SensorPatternPress::Ready;
                }
                break;
            case SensorPatternPress::BackToBegining:
                {
                    m_state = SensorPatternPress::Ready;
                }
                break;
            default:
                break;
            }
        }
    }
};