#pragma once
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

struct BeepSound final
{
    const uint16_t kFrequency;
    const uint16_t kDuration;
    
    // Без звука.
    BeepSound(uint16_t duration)
        : kFrequency(0)
        , kDuration(duration)
    {
    
    }
    
    BeepSound(uint16_t frequency, uint16_t duration)
        : kFrequency(frequency)
        , kDuration(duration)
    {
        
    }
};

class Buzzer final
{
public:
    
    Buzzer()
    {
        m_xSemaphore = xSemaphoreCreateBinaryStatic(&m_xSemaphoreBuffer);
        xSemaphoreGive(m_xSemaphore);

        m_xSemaphoreHighPrio = xSemaphoreCreateBinaryStatic(&m_xSemaphoreHighPrioBuffer);
        xSemaphoreGive(m_xSemaphoreHighPrio);
        
        m_xSemaphorePause = xSemaphoreCreateBinaryStatic(&m_xSemaphorePauseBuffer);
    }
    
    void BeepHighPrio(const BeepSound* samples, uint8_t length)
    {
        xSemaphoreTake(m_xSemaphoreHighPrio, portMAX_DELAY);
        
        bool busy = uxSemaphoreGetCount(m_xSemaphore) == 0;

        // Пищалка занята.
        if(busy)
        {
            // Прервать
            xSemaphoreGive(m_xSemaphorePause);
        }
            
        xSemaphoreTake(m_xSemaphore, portMAX_DELAY);
            
        if (busy)
        {
            // Другой поток не успел захватить мьютекс.
            if(uxSemaphoreGetCount(m_xSemaphorePause))
            {
                xSemaphoreTake(m_xSemaphorePause, portMAX_DELAY);
            }
        }
            
        if (busy)
        {
            vTaskDelay(70);
        }
                
        InnerPlaySound(samples, length);
        xSemaphoreGive(m_xSemaphore);
            
        xSemaphoreGive(m_xSemaphoreHighPrio);
        
        // Если был прерван чей-то звук, сделать дополнительную паузу в начале.
    }
    
    // Воспроизводит звуковые семплы блокируя поток.
    void PlaySound(const BeepSound* samples, const uint8_t length)
    {
        xSemaphoreTake(m_xSemaphore, portMAX_DELAY);
        
        if (uxSemaphoreGetCount(m_xSemaphoreHighPrio))
        {
            InnerPlaySound(samples, length);
        }
        
        xSemaphoreGive(m_xSemaphore);
    }
    
private:

    SemaphoreHandle_t m_xSemaphore;
    StaticSemaphore_t m_xSemaphoreBuffer;
    
    SemaphoreHandle_t m_xSemaphoreHighPrio;
    StaticSemaphore_t m_xSemaphoreHighPrioBuffer;
    
    SemaphoreHandle_t m_xSemaphorePause;
    StaticSemaphore_t m_xSemaphorePauseBuffer;
    
    void SetFrequency(const uint16_t freq)
    {
        if (freq == 0)
        {
            Common::DisableBeeper();
        }
        else
        {
            uint16_t period = 100000 / freq;     // compute period as function of 60KHz ticks.
            Buzzer_TIM->ARR = period - 1;
            Buzzer_TIM->CCR1 = period / 2;
            
            Common::EnableBeeper();
        }
    }
    
    void InnerPlaySound(const BeepSound* samples, const uint8_t length)
    {
        for (uint8_t i = 0; i < length; i++)
        {
            SetFrequency(samples->kFrequency);
            
            //vTaskDelay(samples->Duration / portTICK_PERIOD_MS);
            bool interrupted = (xSemaphoreTake(m_xSemaphorePause, samples->kDuration / portTICK_PERIOD_MS) == pdTRUE);
            if (interrupted)
            {
                break;
            }
            samples++;
        }
        SetFrequency(0);
    }
};

extern Buzzer g_buzzer;
