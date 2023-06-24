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
        m_state = SensorPatternPress::WaitingStart;
    }
    
    bool IsPatternMatch()
    {
        UpdateState();
        return m_state == SensorPatternPress::PatternMatch;
    }
    
    void Reset()
    {
        m_state = SensorPatternPress::BackToBegining;
    }
    
private:
    
    enum State
    {
        WaitingStart,
        Step1Detected,
        Step2Detected,
        PatternMatch,
        BackToBegining // Когда сенсор включен но патерн отмерять не нужно.
    };
    
    const static uint16_t MinimumTimeMsec = 200;
    const static uint16_t MaximumTimeMsec = 600;
    const static uint16_t MaxDeviationMsec = 100;
    
    Stopwatch m_stopwatch;
    uint32_t m_lastTime;
    State m_state;
    
    void UpdateState()
    {
        if (Common::ButtonSensorSwitchIsOn())
        {
            switch (m_state)
            {
            case WaitingStart:
                {
                    m_state = SensorPatternPress::Step1Detected;
                    m_stopwatch.Reset(); // Начинаем замерять первый этап.
                }
                break;
            case Step2Detected:
                {
                    auto step2Msec = m_stopwatch.GetElapsedMsec();

                    if (step2Msec < MinimumTimeMsec || step2Msec > MaximumTimeMsec || Common::abs(step2Msec, m_lastTime) > MaxDeviationMsec)
                    {
                        m_state = SensorPatternPress::BackToBegining; // Был обнаружен патерн но не прошли по таймингам, поэтому начнём с начала.
                    }
                    else
                    {
                        m_state = SensorPatternPress::PatternMatch;
                    }
                }
                break;
            case PatternMatch:
                {
                    m_state = SensorPatternPress::BackToBegining;
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
            case Step1Detected:
                {
                    m_lastTime = m_stopwatch.GetElapsedMsec();

                    if (m_lastTime < MinimumTimeMsec || m_lastTime > MaximumTimeMsec)
                    {
                        m_state = SensorPatternPress::WaitingStart; // Не прошли по таймингам, поэтому начнём с начала.
                    }
                    else
                    {
                        m_state = SensorPatternPress::Step2Detected;
                        m_stopwatch.Reset(); // Начинаем замерять второй этап.
                    }
                }
                break;
            case PatternMatch:
            case BackToBegining:
                {
                    m_state = SensorPatternPress::WaitingStart;
                }
                break;
            default:
                break;
            }
        }
    }
};