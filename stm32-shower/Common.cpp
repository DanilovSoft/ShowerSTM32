#include "Common.h"
#include "Properties.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "WaterLevelTask.h"

void Common::InitWaterLevel()
{
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
    
    uint16_t prescaler = SystemCoreClock / 1000000 - 1;      // 1 микросекунда.
    uint16_t period = g_properties.WaterLevelEmpty + (usec_per_percent * 10);      // Диаппазон таймера с запасом на пару процентов.
    
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
    
    TIM_ICInitTypeDef tim_ic_init_struct = 
    {
        .TIM_Channel = TIM_Channel_1,
        .TIM_ICPolarity = TIM_ICPolarity_Rising,
        .TIM_ICSelection = TIM_ICSelection_DirectTI,
        .TIM_ICPrescaler = TIM_ICPSC_DIV1,
        .TIM_ICFilter = 15     // Максимальное значение 15 или 0b1111.
    };
    
    TIM_ICInit(WL_TIM, &tim_ic_init_struct);
        
    NVIC_InitTypeDef nvic_init_struct = 
    { 
        .NVIC_IRQChannel = TIM1_UP_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 2,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE    
    };
    
    NVIC_Init(&nvic_init_struct);
    
    nvic_init_struct = 
    {
        .NVIC_IRQChannel = TIM1_CC_IRQn,
        .NVIC_IRQChannelPreemptionPriority = 2,
        .NVIC_IRQChannelSubPriority = 0,
        .NVIC_IRQChannelCmd = ENABLE
    };
    
    NVIC_Init(&nvic_init_struct);
        
    TIM_ITConfig(WL_TIM, TIM_IT_Update, ENABLE);
    TIM_ITConfig(WL_TIM, TIM_IT_CC1, ENABLE);
}

void Common::InitHeater()
{
    // Нагреватель.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = GPIO_Pin_Heater,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP,
    };
        
    GPIO_Init(GPIO_Heater, &gpio_init);
        
    // Светодиод нагревателя.
    gpio_init = 
    {
        .GPIO_Pin = GPIO_Heater_Led_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,	
        .GPIO_Mode = GPIO_Mode_Out_PP,
    };
    
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
    GPIO_Init(GPIO_Heater_Led, &gpio_init);
    GPIO_SetBits(GPIO_Heater_Led, GPIO_Heater_Led_Pin);
        
    TurnOffHeater();
}

void Common::InitWiFi()
{
    // Кнопка WPS.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = GPIO_WPS_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_IPU
    };
        
    GPIO_Init(GPIO_WPS, &gpio_init);
    GPIO_SetBits(GPIO_WPS, GPIO_WPS_Pin);
        
    // CH_PD
    gpio_init = 
    {
        .GPIO_Pin = WIFI_GPIO_CH_PD_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    };
    
    GPIO_Init(WIFI_GPIO, &gpio_init);
    GPIO_ResetBits(WIFI_GPIO, WIFI_GPIO_CH_PD_Pin);
}

void Common::InitTempSensor()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
        
    // USART Tx.
    GPIO_InitTypeDef gpio_init_struct = 
    {
        .GPIO_Pin = OW_GPIO_Pin_Tx,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_AF_OD,
    };
    
    GPIO_Init(OneWire_GPIO, &gpio_init_struct);

    USART_InitTypeDef usart_init_struct = 
    {
        .USART_BaudRate = 115200,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_1,
        .USART_Parity = USART_Parity_No,
        .USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
        .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
    };

    USART_Init(OneWire_USART, &usart_init_struct);
    USART_Cmd(OneWire_USART, ENABLE);
    USART_HalfDuplexCmd(OneWire_USART, ENABLE);
}

void Common::InitLedLight()
{
    // Тактуем таймер от шины APB1.
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    
    GPIO_InitTypeDef gpio_init_struct = 
    {
        .GPIO_Pin = GPIO_Pin_LED,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_IN_FLOATING
    };
    GPIO_Init(GPIO_LED, &gpio_init_struct);

    // Частота таймера - 1 мегагерц.
    uint16_t prescaler = SystemCoreClock / 1000000 - 1;
    
    TIM_TimeBaseInitTypeDef tim_time_base_init_struct = 
    {
        .TIM_Prescaler = prescaler,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = 100 - 1,
        .TIM_ClockDivision = TIM_CKD_DIV1, 
        // без  делителя.
        .TIM_RepetitionCounter = 0,
    };
    TIM_TimeBaseInit(LED_TIM, &tim_time_base_init_struct);
    
    // Скважность от 0 до 100%.
    uint16_t pulse = (tim_time_base_init_struct.TIM_Period + 1) / 100 * g_properties.LightBrightness;
    
    TIM_OCInitTypeDef tim_oc_init_struct = 
    {
        .TIM_OCMode = TIM_OCMode_PWM1,
        .TIM_OutputState = TIM_OutputState_Enable,
        .TIM_OutputNState = TIM_OutputNState_Disable,
        .TIM_Pulse = pulse,
        .TIM_OCPolarity = TIM_OCPolarity_High,
        .TIM_OCNPolarity = TIM_OCPolarity_High,
        .TIM_OCIdleState = TIM_OCIdleState_Reset,
        .TIM_OCNIdleState = TIM_OCNIdleState_Reset
    };
    TIM_OC2Init(LED_TIM, &tim_oc_init_struct);

    // Включаем таймер.
    TIM_Cmd(LED_TIM, ENABLE);
}

void Common::InitButtons()
{
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = Button_Temp_Plus | Button_Temp_Minus | Button_Water_Pin | Button_SensorSwitch_OUT,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_IPD
    };
    GPIO_Init(Button_GPIO, &gpio_init);
}

void Common::InitValve()
{
    // Клапан.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = Valve_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    };
    GPIO_Init(Valve_GPIO, &gpio_init);
        
    // Закрыть клапан.
    GPIO_ResetBits(Valve_GPIO, Valve_Pin);
        
    // Питающий вывод сенсора.
    gpio_init = 
    {
        .GPIO_Pin = SensorSwitch_Power_Pin,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    };
    GPIO_Init(SensorSwitch_Power_GPIO, &gpio_init);

    // Выключить питание сенсора до окончания инициали датчика уровня воды.
    TurnOffSensorSwitch();
}

void Common::InitWaterLevelPeripheral()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    SPI_InitTypeDef spi_init_struct = 
    {
        .SPI_Direction = SPI_Direction_1Line_Tx, // Только передача.
        .SPI_Mode = SPI_Mode_Master, // Режим - мастер.
        .SPI_DataSize = SPI_DataSize_16b, // Передаём по 16 бит.
        .SPI_CPOL = SPI_CPOL_Low, // Полярность и
        .SPI_CPHA = SPI_CPHA_1Edge, // фаза тактового сигнала.
        .SPI_NSS = SPI_NSS_Soft, // Управлять состоянием сигнала NSS программно.
        .SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2, // Предделитель SCK.
        .SPI_FirstBit = SPI_FirstBit_MSB, // Первым отправляется старший бит.
        .SPI_CRCPolynomial = 7
    };
    SPI_Init(WL_SPI, &spi_init_struct);      // Настраиваем SPI.
    SPI_Cmd(WL_SPI, ENABLE);                 // Включаем модуль SPI.
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
}