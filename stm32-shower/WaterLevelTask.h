#pragma once
#include "TaskBase.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"
#include "Common.h"
#include "misc.h"
#include "Properties.h"
#include "TempSensorTask.h"
#include "WaterLevelAnimationTask.h"
#include "PropertyWrapper.h"

// PS. При повышении температуры воздуха на 1 °C скорость звука в нем увеличивается на 0.57 м/с 
// 1% на каждые 5 °С => 0.2% на 1 °С
// Возможно у датчика уровня нет поправки на температуру и мы можем ввести такую.

extern "C"
{
    extern volatile uint8_t TIM_CAPTURE_STATE;
    extern volatile uint16_t TIM_CAPTURE_VAL;
}

class WaterLevelTask final : public TaskBase
{	
public:
    
    void Init()
    {   
        Debug::Assert(g_properties.Initialized);
        
        m_medianFilter.Init(g_properties.WaterLevelMedianFilterSize);
        m_movingAverageFilter.Init(g_properties.WaterLevelAvgFilterSize);
        m_usecRange = g_properties.WaterLevelEmpty - g_properties.WaterLevelFull;
        m_pointsPerUsec = 99.0f / m_usecRange;
        m_intervalPauseMsec = g_properties.WaterLevelMeasureIntervalMsec / portTICK_PERIOD_MS;
        m_minimumAllowedUsec = g_properties.WaterLevelFull * 0.8; // На 20% меньше минимально допустимого значения.
    }
    
    // Уровень воды в баке от 0 до 99, %.
    uint8_t GetPercent() volatile
    {
        return m_percent;
    }
    
    // Последнее измеренное значение после усреднений.
    uint16_t GetAvgUsec() volatile
    {
        return m_avgUsec;
    }
    
    // Значение -1 означает что последнее измерение не удалось.
    uint16_t GetUsecRaw() volatile
    {
        return m_usecRaw;
    }
    
    // True если было получено хоть одно показание с датчика уровня.
    bool GetHasPercent() volatile
    {
        return m_hasPercent;
    }
    
    bool GetInitialized() volatile
    {
        return m_initialized;
    }
    
    static void ClearDisplay()
    {
        // Очищаем  как можно раньше что-бы предотвратить кратковременное отображение мусора на дисплее.
        // PS. можно будет убрать если использовать LED дисплей другой полярности.
        DisplayLED(kADash, kBDash);
    }
    
    //    void WaitInitialization()
    //    {
    //        while (!m_initialized)
    //        {
    //            taskYIELD();
    //        }
    //    }
    
        // Возвращает True если датчик, за последнее время, получил слишком много не валидных показаний.
    bool GetIsError() volatile
    {
        return m_errorCounter >= g_properties.WaterLevelErrorThreshold;
    }
    
private:
    
    static constexpr uint8_t kADash = 0b11011111; // Горизонтальный прочерк в первом разряде индикатора.
    static constexpr uint8_t kBDash = 0b11111101; // Горизонтальный прочерк во втором разряде индикатора.
    //static constexpr uint8_t kHysteresisPoints = 2; // Гистерезис на несколько пунктов.
    
    uint16_t m_usecRange; // Ширина полного диаппазона в микросекундах.
    float m_pointsPerUsec; // Сколько микросекунд приходится на один пункт (из 99)
    uint8_t m_intervalPauseMsec; // Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
    // Показания датчика меньше этого значения будут считаться не валидными 
    // (например когда датчик заслонён или ловит своё эхо от зеркала).
    uint16_t m_minimumAllowedUsec;
    MovingAverageFilter m_movingAverageFilter;
    MedianFilter m_medianFilter;
    
    //bool m_waterIsRising = true; // По умолчанию для гистерезиса считать что вода подымается.
    volatile bool m_initialized = false;
    volatile uint8_t m_errorCounter = 0;
    
    // Последнее измеренное значение после усреднений.
    volatile uint16_t m_avgUsec = 0;
    
    // Значение -1 означает что последнее измерение не удалось.
    volatile int16_t m_usecRaw = -1;
    
    // Уровень воды в баке от 0 до 99, %.
    volatile uint8_t m_percent = 0;
    
    // True если было получено хоть одно показание с датчика уровня.
    volatile bool m_hasPercent = false;

    virtual void Run() override
    {
        vTaskDelay(510 / portTICK_PERIOD_MS); // DYP-A22 первые 500ms находится в режиме Modbus
        
        // Initialise the xLastWakeTime variable with the current time.
        TickType_t xLastWakeTime = xTaskGetTickCount();
    
        // Хранит прошлое значение пунктов. Используется гистерезисом.
        InitDisplay();
    
        // Уровень воды инициализирован.
        m_initialized = true;
    
        while (true)
        {
            // Пауза для затухания эха.
            EchoPause(&xLastWakeTime);
            
            uint16_t usec_raw; // Сырые показания датчика в микросекундах.
            if (!GetRawUsecTime(usec_raw))
            {
                m_usecRaw = -1;
                IncrementError();
                continue;
            }
                
            // Скопировать сырые показания микросекунд в публичную переменную.
            m_usecRaw = usec_raw;

            // Проверить не заслонен ли датчик.
            if (SensorIsBlocked(usec_raw))
            {
                // Датчик чем-то заслонён — расстояние слишком короткое.
                // Считаем это ошибкой получения уровня воды.
                IncrementError();
                continue;
            }
                
            // Медианный фильтр.
            uint16_t median_usec = m_medianFilter.AddValue(usec_raw);

            // Скользящее среднее.
            uint16_t avg_usec = m_movingAverageFilter.AddValue(median_usec);

            // Скопировать фильтрованное значение микросекунд в публичную переменную.
            m_avgUsec = avg_usec;
            
            // Уровень воды в дробных пунктах.
            float pointf = TimeToPoints(avg_usec);
            
            // Строго от 0 до 99
            uint8_t point = ClampFloatToInt(pointf);

            // Скопировать уровень воды в процентах в публичную переменную.
            m_percent = point; // volatile

            // Отобразить на LED.
            TaskDisplayPercent(point);
                
            DecrementError();
        }
    }

    void IncrementError()
    {
        if (m_errorCounter < g_properties.WaterLevelErrorThreshold)
        {
            m_errorCounter++;
            
            if (GetIsError())
            {
                // Опять запускаем анимацию на LCD.
                g_wlAnimationTask.ResumeAnimation();
            }
        }
        else
        {
            TaskDisplayLED(kADash, kBDash);
        }
    }
    
    void DecrementError()
    {
        if (m_errorCounter > 0)
        {
            m_errorCounter--;
        }
    }
    
    // рекомендуется повторять циклы измерения расстояния минимум через 60 мсек — что-бы утих переотражённый звук.
    inline void EchoPause(TickType_t* const pxPreviousWakeTime)
    {
        // MEMO vTaskDelayUntil в отличии от vTaskDelay работает так что может выдерживать паузу меньше чем мы запрашиваем;
        // например передаём 60 мсек но с прошлого вызова vTaskDelayUntil уже прошло 10 мсек, значит пауза выполнится на 50 мсек.
        
        vTaskDelayUntil(pxPreviousWakeTime, m_intervalPauseMsec);
    }
    
    // Возвращает True если расстояние от датчика до воды оказалось меньше минимально возможного значения на 20%.
    inline bool SensorIsBlocked(uint16_t usec)
    {
        return usec < m_minimumAllowedUsec;
    }
    
    void InitDisplay()
    {
        TickType_t xLastWakeTime = xTaskGetTickCount(); 
        
        // Дать датчику эксклюзивное время на инициализацию 
        // И выполнить прогрев. (Почему-то сказывается при включенной оптимизации -O1 и выше).
        EchoPause(&xLastWakeTime);
        Trig();
        
        // Размер медианного фильтра + буфера скользящее среднее.
        uint16_t warmup_count = g_properties.WaterLevelMedianFilterSize + g_properties.WaterLevelAvgFilterSize;
    
        for (uint16_t i = 0; i < warmup_count;)
        {
            // Пауза что-бы утихло эхо.
            EchoPause(&xLastWakeTime);
            
            uint16_t usec_raw; // Сырые показания датчика.
            if (!GetRawUsecTime(usec_raw)) 
            {
                m_usecRaw = -1;
                IncrementError();
                continue;   
            }
            
            m_usecRaw = usec_raw;
            
            // Проверить не заслонен ли датчик.
            if (SensorIsBlocked(usec_raw))
            {
                // Датчик чем-то заслонён? — расстояние слишком короткое.
                IncrementError();
                continue;
            }
                    
            // Медианный фильтр.
            uint16_t median_usec = m_medianFilter.AddValue(usec_raw);

            // Добавить значение в буффер скользящего среднего.
            uint16_t avg_usec = m_movingAverageFilter.AddValue(median_usec);
            
            if (i < g_properties.WaterLevelAvgFilterSize)
            {
                // Фильтр 'скользящее среднее' заполнен НЕ полностью — делим его значение на коэффициент заполненности.
                avg_usec = m_movingAverageFilter.GetAverage() / (i + 1); // Точность с кажной итерацией будет увеличиваться.
            }
            
            // Инициализировать глобальную переменную.
            m_avgUsec = avg_usec;
            
            // Уровень воды в дробных пунктах (грубо в диаппазоне от 0 до 99).
            float pointf = TimeToPoints(avg_usec);

            // строго от 0 до 99
            uint8_t point = ClampFloatToInt(pointf);

            // Инициализировать глобальную переменную.
            m_percent = point;

            // Отобразить на LED дисплее.
            TaskDisplayPercent(point);

            m_hasPercent = true;

            i++; // Считаем только успешные измерения.

            DecrementError();
        }
    }

    bool GetRawUsecTime(uint16_t &usec)
    {
        // Разрешить работу таймера.
        TIM_CAPTURE_STATE = SoundSensorTimerState::WAITING_RISING_EDGE;

        // Сбросить счетчик таймера.
        TIM_SetCounter(WL_TIM, 0);
        
        // Запустить таймер.
        TIM_Cmd(WL_TIM, ENABLE);
    
        // Отправить звуковой сигнал.
        Trig();
    
        // Ждём флаг завершения (когда сработает прерывание таймера по заднему фронту или по переполнению).
        while (!(TIM_CAPTURE_STATE & SoundSensorTimerState::COMPLETED))
        {
            taskYIELD();
        }

        TIM_Cmd(WL_TIM, DISABLE);
    
        if (TIM_CAPTURE_STATE & SoundSensorTimerState::OVERFLOW)
        {
            usec = 0;
            return false;
        }
        
        // Копируем volatile.
        usec = TIM_CAPTURE_VAL;

        // По ДШ расстояние может быть от 2см т.е. примерно 116 мкс.
        // Иногда значение получается 15..16 по непонятным причинам.
        if (usec < 56)
        {
            return false;
        }
    
        return true;
    }
    
    // Функция предотвращает плавание показаний когда уровень воды неподвижен.
    // Возвращает или point или last_point.
//    uint8_t Hysteresis(uint8_t point, uint8_t last_point)
//    {
//        // На сколько пунктов изменился уровень воды.
//        int16_t diff = (point - last_point);
//    
//        if (diff == 0)
//        {
//            return point;
//        }
//            
//        if (m_waterIsRising)
//        {
//            // Считается что уровень воды увеличивается.
//                    
//            if (diff < 0)
//            {
//                // Вода начала убывать.
//                            
//                if (diff <= -kHysteresisPoints)
//                {
//                    // Уровень воды уменьшился более чем на 4 пункта.
//                                    
//                    // Считать что вода теперь убывает.
//                    m_waterIsRising = false;
//                }
//                else
//                {
//                    // Уровень воды уменьшился не значительно.
//                                    
//                    // Возвращаем прошлое значение.
//                    return last_point;
//                }
//            }
//        }
//        else
//        {
//            // Считается что уровень воды уменьшается.
//                    
//            if (diff > 0)
//            {
//                // Уровень воды начал увеличиваться.
//                    
//                if (diff >= kHysteresisPoints)
//                {
//                    // Уровень увеличился более чем на 4 пункта.
//                            
//                    // Считать что вода теперь прибывает.
//                    m_waterIsRising = true;
//                }
//                else
//                {
//                    // Уровень воды увеличился не значительно.
//                            
//                    // Возвращаем прошлое значение.
//                    return last_point;
//                }
//            }
//        }
//    
//        // По умолчанию возвращаем без гистерезиса.
//        return point;
//    }

    // Trig на 10 микро секунд
    inline static void Trig()
    {
        // Отправить звуковой сигнал.
        GPIO_SetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
        DELAY_US(10);
        GPIO_ResetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    }
    
    // От 0 до 99.
    static uint8_t ClampFloatToInt(float pointf)
    {
        pointf = Common::Clamp(pointf, 0, 99);
        
        uint8_t point = roundf(pointf);

        return point;
    }
    
    // От 0 до 99.
    float TimeToPoints(uint16_t usec)
    {
        // Уровень воды в микросекундах.
        usec = m_usecRange - (usec - g_properties.WaterLevelFull);

        // Сколько пунктов из 99
        float point = usec * m_pointsPerUsec;

        return point;
    }
    
    inline static void TaskDisplayPercent(const uint8_t percent)
    {   
        static constexpr uint8_t a0 = 0b10100000;
        static constexpr uint8_t a1 = 0b11111100;
        static constexpr uint8_t a2 = 0b10010010;
        static constexpr uint8_t a3 = 0b10011000;
        static constexpr uint8_t a4 = 0b11001100;
        static constexpr uint8_t a5 = 0b10001001;
        static constexpr uint8_t a6 = 0b10000001;
        static constexpr uint8_t a7 = 0b10111100;
        static constexpr uint8_t a8 = 0b10000000;
        static constexpr uint8_t a9 = 0b10001000;
        static constexpr uint8_t b0 = 0b10000010;
        static constexpr uint8_t b1 = 0b11101110;
        static constexpr uint8_t b2 = 0b11000001;
        static constexpr uint8_t b3 = 0b11001000;
        static constexpr uint8_t b4 = 0b10101100;
        static constexpr uint8_t b5 = 0b10011000;
        static constexpr uint8_t b6 = 0b10010000;
        static constexpr uint8_t b7 = 0b11001110;
        static constexpr uint8_t b8 = 0b10000000;
        static constexpr uint8_t b9 = 0b10001000;
        static constexpr uint8_t abBlank = 0b11111111;
        static constexpr uint8_t aA[] { abBlank, a1, a2, a3, a4, a5, a6, a7, a8, a9 }
        ;
        const constexpr uint8_t bB[] { b0, b1, b2, b3, b4, b5, b6, b7, b8, b9 }
        ;
        
        // Индикатор может отображать только 2 разряда.
        //uint8_t displayPercent = percent == 100 ? 99 : percent;
        
        TaskDisplayLED(aA[percent / 10], bB[percent % 10]);
    }
    
    // Отправка данных в SPI в контексте RTOS.
    static void TaskDisplayLED(const uint8_t a_value, const uint8_t b_value)
    {
        uint16_t value = ((a_value << 8) | b_value);
        TaskDisplayLED(value);
    }

    // Отправка данных в SPI НЕ в контексте RTOS.
    static void DisplayLED(const uint8_t a_value, const uint8_t b_value)
    {
        uint16_t value = ((a_value << 8) | b_value);
        DisplayLED(value);
    }
    
    // Отправка данных в SPI НЕ в контексте RTOS.
    static void DisplayLED(const uint16_t value)
    {
        SPISend(value);
        while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_BSY) == SET)
        {
            // Не использовать taskYIELD иначе HardFault при вызове не из Task'a.  
        }

        // Latch.
        GPIO_SetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
        DELAY_US(1);
        GPIO_ResetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
    }

    // Отправка данных в SPI в контексте RTOS.
    static void TaskDisplayLED(uint16_t value)
    {
        TaskSPISend(value);
        while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_BSY) == SET)
        {
            taskYIELD();    
        }
            
        // Latch.
        GPIO_SetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
        DELAY_US(1);
        GPIO_ResetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
    }
    
    // Отправка данных в SPI НЕ в контексте RTOS.
    static void SPISend(const uint16_t data)
    {
        while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_TXE) == RESET)
        {
            // Не использовать taskYIELD иначе HardFault при вызове не из Task'a.
        }
        
        SPI_I2S_SendData(WL_SPI, data);
    }
    
    // Отправка данных в SPI в контексте RTOS.
    static void TaskSPISend(uint16_t data)
    {
        while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_TXE) == RESET)
        {
            taskYIELD();
        }

        SPI_I2S_SendData(WL_SPI, data);
    }
};

extern WaterLevelTask g_waterLevelTask;
