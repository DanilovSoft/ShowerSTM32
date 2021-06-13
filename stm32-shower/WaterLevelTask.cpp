#include "WaterLevelTask.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"
#include "Common.h"
#include "misc.h"
#include "Properties.h"
#include "TempSensor.h"

void WaterLevelTask::Init()
{
    m_medianFilter.Init(g_properties.WaterLevel_Median_Buffer_Size);
    
    m_usecRange = (g_properties.WaterLevelEmpty - g_properties.WaterLevelFull);
    float usec_per_percent = (g_properties.WaterLevelEmpty - g_properties.WaterLevelFull) / 99.0;
        
    // Trig.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = WL_GPIO_Trig_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    };
    
    GPIO_Init(WL_GPIO_Trig, &gpio_init);
    GPIO_ResetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
        
    // Input Capture.
    gpio_init = 
    {
        .GPIO_Pin = WL_GPIO_TIM_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_IPD
    };
    
    GPIO_Init(WL_GPIO_TIM, &gpio_init);
    GPIO_ResetBits(WL_GPIO_TIM, WL_GPIO_TIM_Pin);
    
    uint16_t prescaler = SystemCoreClock / 1000000 - 1; // 1 микросекунда.
    uint16_t period = g_properties.WaterLevelEmpty + (usec_per_percent * 10); // Диаппазон таймера с запасом на пару процентов.
    
    TIM_TimeBaseInitTypeDef base_timer =
    {
        .TIM_Prescaler = prescaler,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = period,
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0
    };
    
    TIM_TimeBaseInit(WL_TIM, &base_timer);
    TIM_ClearITPendingBit(WL_TIM, TIM_IT_Update);
    
    TIM_ICInitTypeDef TIM_ICInitStructure = 
    {
        .TIM_Channel = TIM_Channel_1,
        .TIM_ICPolarity = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter = 15     // Максимальное значение 15 или 0b1111
    };
    
    TIM_ICInit(WL_TIM, &TIM_ICInitStructure);
        
    NVIC_InitTypeDef NVIC_InitStructure = 
    { 
        .NVIC_IRQChannel = TIM1_UP_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 2,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE    
    };
    
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure = 
    {
        .NVIC_IRQChannel = TIM1_CC_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 2,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    };
    
    NVIC_Init(&NVIC_InitStructure);
        
    TIM_ITConfig(WL_TIM, TIM_IT_Update, ENABLE);
    TIM_ITConfig(WL_TIM, TIM_IT_CC1, ENABLE);
}

void WaterLevelTask::Run()
{
    TickType_t xLastWakeTime;
    m_intervalPauseMsec = g_properties.WaterLevel_Measure_IntervalMsec / portTICK_PERIOD_MS;
    
    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    
    // Хранит прошлое значение пунктов. Используется гистерезисом.
    uint8_t lastPoint = InitDisplay();
    
    // Уровень воды инициализирован.
    m_isInitialized = true;
    
    // Для гистерезиса по умолчанию считаем что уровень воды поднимается (Что-бы не допустить перелив).
    m_waterIsRising = true;
    
    while (true)
    {
        uint16_t usecRaw;
        if (GetRawUsecTime(usecRaw)) // Сырые показания датчика в микросекундах.
        {
            // Скопировать сырые показания микросекунд в публичную переменную.
            UsecRaw = usecRaw;

            // Медианный фильтр.
            uint16_t median = m_medianFilter.AddValue(usecRaw);

            // Скользящее среднее.
            uint16_t avg = m_movingAverageFilter.AddValue(median);

            // Скопировать фильтрованное значение микросекунд в публичную переменную.
            AvgUsec = avg;

            // Проверить не заслонен ли датчик.
            SensorIsBlocked = CheckSensorBlocking(avg);
            
            // Поправка на выход из диаппазона.
            avg = ClampRange(avg);
            
            // Уровень воды в дробных пунктах.
            float pointf = GetPoint(avg);
            
            // Целое количество пунктов.
            uint8_t point = GetIntPoint(pointf);
            
            // Пункты с учетом гистерезиса.
            point = Hysteresis(point, lastPoint);

            // Запомнить прошлое значение c гистерезисом.
            lastPoint = point;
            
            // Уровень воды от 0% до 99%
            uint8_t percent = GetPercent(point);

            // Скопировать уровень воды в процентах в публичную переменную.
            DisplayingPercent = percent;

            // Отобразить на LED.
            TaskDisplayPercent(percent);
        }
        else
        {
            UsecRaw = -1;
        }

        // Пауза для затухания эха.
        vTaskDelayUntil(&xLastWakeTime, m_intervalPauseMsec);
    }
}

void WaterLevelTask::FixRawTemp(uint16_t &usec)
{
    return;
    
    //    double v = 331 + 0.6 * tempSensor.ExternalTemp;
    //    double d = v / 331;
    //    usec = round(usec * d);
}

bool WaterLevelTask::CheckSensorBlocking(uint16_t usec)
{
    if (usec < (g_properties.WaterLevelFull * 0.8))
        /* Расстояние меньше минимального возможного на 20% */
    {
        return true;
    }
    
    return false;
}
    
uint8_t WaterLevelTask::InitDisplay()
{
    TickType_t xLastWakeTime = xTaskGetTickCount(); 
        
    // Дать датчику эксклюзивное время на инициализацию 
    // И выполнить прогрев. (Почему-то сказывается при включенной оптимизации -O1 и выше).
    vTaskDelayUntil(&xLastWakeTime, m_intervalPauseMsec);
    GPIO_SetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    DELAY_US(10);
    GPIO_ResetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    vTaskDelayUntil(&xLastWakeTime, m_intervalPauseMsec);
        
    // Размер медианного фильтра + буфера скользящее среднее.
    uint16_t warmupCount = g_properties.WaterLevel_Median_Buffer_Size + g_properties.WaterLevel_Avg_Buffer_Size;
    
    uint8_t lastPoint;
    for (uint16_t i = 0; i < warmupCount;)
    {
        uint16_t usecRaw;
        if (GetRawUsecTime(usecRaw)) // Сырые показания датчика.
        {
            UsecRaw = usecRaw;
            
            // Медианный фильтр.
            uint16_t median = m_medianFilter.AddValue(usecRaw);

            // Добавить значение в буффер скользящего среднего.
            uint16_t avg = m_movingAverageFilter.AddValue(median);
            
            if (i < g_properties.WaterLevel_Avg_Buffer_Size)
            {
                // Фильтр 'скользящее среднее' заполнен НЕ полностью, поэтому делим его значение на коэффициент заполненности.
                avg = m_movingAverageFilter.GetAverage() / (i + 1);  // Точность с кажной итерацией будет увеличиваться.
            }
            
            // Проверить не заслонен ли датчик.
            SensorIsBlocked = CheckSensorBlocking(avg);
            
            // Инициализировать глобальную переменную.
            AvgUsec = avg;

            // Поправка на выход из диаппазона.
            avg  = ClampRange(avg);
            
            // Уровень воды в дробных пунктах (От 0 до 99).
            float pointf = GetPoint(avg);

            // Изначальное значение будет округленным.
            uint8_t point = (uint8_t)roundf(pointf);

            // Запомнить прошлый уровень в пунктах.
            lastPoint = point;
            
            // Инициализировать глобальную переменную.
            DisplayingPercent = GetPercent(point);

            // Отобразить.
            TaskDisplayPercent(DisplayingPercent);

            Preinitialized = true;

            i++; // Считаем только успешные измерения.
        }
        else
        {
            UsecRaw = -1;
        }
        
        // Пауза.
        vTaskDelayUntil(&xLastWakeTime, m_intervalPauseMsec);
    }
    
    // Вернуть уровень воды в пунктах.
    return lastPoint;
}

bool WaterLevelTask::GetRawUsecTime(uint16_t &usec)
{
    // Разрешить работу таймера.
    TIM_CAPTURE_STA = 0;

    // Сбросить счетчик таймера.
    TIM_SetCounter(WL_TIM, 0);
        
    // Запустить таймер.
    TIM_Cmd(WL_TIM, ENABLE);
    
    // Отправить звуковой сигнал.
    GPIO_SetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    DELAY_US(10);
    GPIO_ResetBits(WL_GPIO_Trig, WL_GPIO_Trig_Pin);
    
    // Ждём флаг завершения (когда сработает прерывание таймера по заднему фронту или по переполнению).
    while (!(TIM_CAPTURE_STA & WL_SUCCESS))
    {
        taskYIELD();
    }

    TIM_Cmd(WL_TIM, DISABLE);
    
    // Копируем volatile.
    usec = TIM_CAPTURE_VAL;

    // По ДШ расстояние может быть от 2см т.е. примерно 116 мкс.
    // Иногда значение получается 15..16 по непонятным причинам.
    if (usec < 116)
    {
        return false;
    }

    // true если не было переполнения таймера.
    bool success = (!(TIM_CAPTURE_STA & WL_OVERFLOW));
    
    return success;
}
    
void WaterLevelTask::FixRange(uint16_t &usec)
{
    if (usec < g_properties.WaterLevelFull)
    {
        usec = g_properties.WaterLevelFull;
    }
    else if (usec > g_properties.WaterLevelEmpty)
    {   
        usec = g_properties.WaterLevelEmpty;
    }
}
    
float WaterLevelTask::GetFloatPercent(uint16_t usec)
{
    /* Поправка на выход из диаппазона */
    FixRange(usec);
        
    /* Смещение */
    usec -= g_properties.WaterLevelFull;
        
    /* Уровень воды в микросекундах */
    usec = m_usecRange - usec;
        
    uint32_t tmp = usec * 99;
        
    /* Сколько пунктов из 99 */
    float point = tmp / (float)m_usecRange; 
        
    return point;
}
    
inline uint8_t WaterLevelTask::GetPercent(uint8_t point)
{
    if (point > 99)
        point = 99;

    return point;
}

inline uint16_t WaterLevelTask::ClampRange(uint16_t usec)
{
    if (usec < g_properties.WaterLevelFull)
    {
        return g_properties.WaterLevelFull;
    }
    else if (usec > g_properties.WaterLevelEmpty)
    {
        return g_properties.WaterLevelEmpty;
    }
    
    return usec;
}
    
inline uint8_t WaterLevelTask::Hysteresis(uint8_t point, uint8_t lastPoint)
{
    // На сколько пунктов изменился уровень воды.
    int16_t diff = (point - lastPoint);
    
    // небольшая оптимизация
    if(diff != 0)
    // Если уровень воды хоть как-то изменился.
    {
        if (m_waterIsRising)
        // Считается что уровень воды увеличивается.
        {
            if (diff < 0)
            // Вода начала убывать.
            {
                if (diff <= -kHysteresisPoints)
                // Уровень воды уменьшился более чем на 4 пункта.
                {
                    // Считать что вода теперь убывает.
                    m_waterIsRising = false;
                }
                else
                // Уровень воды уменьшился не значительно.
                {
                    // Возвращаем прошлое значение.
                    return lastPoint;
                }
            }
        }
        else
        // Считается что уровень воды уменьшается.
        {
            if (diff > 0)
            // Уровень воды начал увеличиваться.
            {
                if (diff >= kHysteresisPoints)
                // Уровень увеличился более чем на 4 пункта.
                {
                    // Считать что вода теперь прибывает.
                    m_waterIsRising = true;
                }
                else
                // Уровень воды увеличился не значительно.
                {
                    // Возвращаем прошлое значение.
                    return lastPoint;
                }
            }
        }
    }
    
    // По умолчанию возвращаем без гистерезиса.
    return point;
}

inline uint8_t WaterLevelTask::GetIntPoint(float pointf)
{
    uint8_t point = roundf(pointf);
    return point;
}

inline float WaterLevelTask::GetPoint(uint16_t usec)
{
    // Смещение.
    usec -= g_properties.WaterLevelFull;
        
    // Уровень воды в микросекундах.
    usec = (m_usecRange - usec);
        
    uint32_t tmp = usec * 99;
        
    // Сколько пунктов из 99.
    float point = tmp / (float)m_usecRange;

    return point;
}
    
inline void WaterLevelTask::TaskDisplayPercent(uint8_t percent)
{   
    const static uint8_t a0 = 0b10100000;
    const static uint8_t a1 = 0b11111100;
    const static uint8_t a2 = 0b10010010;
    const static uint8_t a3 = 0b10011000;
    const static uint8_t a4 = 0b11001100;
    const static uint8_t a5 = 0b10001001;
    const static uint8_t a6 = 0b10000001;
    const static uint8_t a7 = 0b10111100;
    const static uint8_t a8 = 0b10000000;
    const static uint8_t a9 = 0b10001000;
    const static uint8_t b0 = 0b10000010;
    const static uint8_t b1 = 0b11101110;
    const static uint8_t b2 = 0b11000001;
    const static uint8_t b3 = 0b11001000;
    const static uint8_t b4 = 0b10101100;
    const static uint8_t b5 = 0b10011000;
    const static uint8_t b6 = 0b10010000;
    const static uint8_t b7 = 0b11001110;
    const static uint8_t b8 = 0b10000000;
    const static uint8_t b9 = 0b10001000;
    const static uint8_t abBlank = 0b11111111;
    const static uint8_t aA[] { abBlank, a1, a2, a3, a4, a5, a6, a7, a8, a9 }
    ;
    const static uint8_t bB[] { b0, b1, b2, b3, b4, b5, b6, b7, b8, b9 }
    ;
        
    TaskDisplayLED(aA[percent / 10], bB[percent % 10]);
}
    
inline void WaterLevelTask::TaskDisplayLED(uint8_t Ax, uint8_t Bx)
{
    uint16_t value = ((Ax << 8) | Bx);
    TaskDisplayLED(value);
}

inline void WaterLevelTask::DisplayLED(uint8_t Ax, uint8_t Bx)
{
    uint16_t value = ((Ax << 8) | Bx);
    DisplayLED(value);
}
    
void WaterLevelTask::DisplayLED(uint16_t value)
{
    SPISend(value);
    while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_BSY) == SET)
    {
        // Не использовать taskYIELD иначе HardFault при вызове не из Task'a    
    }

    /* Latch */
    GPIO_SetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
    DELAY_US(1);
    GPIO_ResetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
}

void WaterLevelTask::TaskDisplayLED(uint16_t value)
{
    TaskSPISend(value);
    while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_BSY) == SET)
    {
        taskYIELD();    
    }
            
    /* Latch */
    GPIO_SetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
    DELAY_US(1);
    GPIO_ResetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
}
    
void WaterLevelTask::SPISend(uint16_t data)
{
    while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_TXE) == RESET)
    {
        // Не использовать taskYIELD иначе HardFault при вызове не из Task'a
    }
        
    SPI_I2S_SendData(WL_SPI, data);
}
    
void WaterLevelTask::TaskSPISend(uint16_t data)
{
    while (SPI_I2S_GetFlagStatus(WL_SPI, SPI_I2S_FLAG_TXE) == RESET)
    {
        taskYIELD();
    }

    SPI_I2S_SendData(WL_SPI, data);
}

void WaterLevelTask::InitGpioAndClearDisplay()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    SPI_InitTypeDef spi_init_struct = 
    {
        .SPI_Direction = SPI_Direction_1Line_Tx,                // Только передача.
        .SPI_Mode = SPI_Mode_Master,                            // Режим - мастер.
        .SPI_DataSize = SPI_DataSize_16b,                       // Передаём по 16 бит.
        .SPI_CPOL = SPI_CPOL_Low,                               // Полярность и
        .SPI_CPHA = SPI_CPHA_1Edge,                             // фаза тактового сигнала.
        .SPI_NSS = SPI_NSS_Soft,                                // Управлять состоянием сигнала NSS программно.
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,       // Предделитель SCK.
        .SPI_FirstBit = SPI_FirstBit_MSB,                       // Первым отправляется старший бит.
        .SPI_CRCPolynomial = 7
    };
    SPI_Init(WL_SPI, &spi_init_struct);                                                     // Настраиваем SPI
    SPI_Cmd(WL_SPI, ENABLE);                                                                // Включаем модуль SPI
    SPI_NSSInternalSoftwareConfig(WL_SPI, SPI_NSSInternalSoft_Set);

    // LED LATCH.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = WL_GPIO_LATCH_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    };
    GPIO_Init(WL_GPIO_LATCH, &gpio_init);
    GPIO_ResetBits(WL_GPIO_LATCH, WL_GPIO_LATCH_Pin);
        
    // Порты SPI.
    gpio_init = 
    {
        .GPIO_Pin = WL_GPIO_SPI_MOSI_Pin | WL_GPIO_SPI_SCK_Pin,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_AF_PP
    };
    GPIO_Init(WL_GPIO_SPI, &gpio_init);

    DisplayLED(kADash, kBDash);
}

void WaterLevelTask::WaitInitialization()
{
    while (!m_isInitialized)
    {
        taskYIELD();
    }
}


