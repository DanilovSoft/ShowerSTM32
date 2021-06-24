#include "Common.h"
#include "Properties.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"
#include "WaterLevelTask.h"
#include "UartStream.h"
#include "HeaterTask.h"
#include "WiFiTask.h"
#include "LcdTask.h"
#include "LedLightTask.h"
#include "ButtonsTask.h"

__attribute__((noreturn)) void __break_func(const char * file_name, int line)
{
    __asm("bkpt 255");
    exit(0);
}


void Common::InitWaterLevel(const PropertyStruct* const properties)
{
    Debug::Assert(properties != NULL);
    
    float usec_per_percent = (properties->WaterLevelEmpty - properties->WaterLevelFull) / 99.0;
        
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
    
    const uint16_t prescaler = SystemCoreClock / 1000000 - 1;      // 1 микросекунда.
    const uint16_t period = properties->WaterLevelEmpty + (usec_per_percent * 10);       // Диаппазон таймера с запасом на пару процентов.
    
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

void Common::InitLedLight(const PropertyStruct* const properties)
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
    uint16_t pulse = (tim_time_base_init_struct.TIM_Period + 1) / 100 * properties->LightBrightness;
    
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

void Common::InitUartPeripheral(const uint32_t memory_base_addr)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        
    // Настраиваем ногу TxD как выход push-pull c альтернативной функцией.
    GPIO_InitTypeDef gpio_init = 
    {
        .GPIO_Pin = GPIO_WIFI_Pin_Tx,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_AF_PP
    };
    GPIO_Init(GPIO_WIFI_USART, &gpio_init);

    // Настраиваем ногу как вход UARTа (RxD).
    gpio_init = 
    {
        .GPIO_Pin = GPIO_WIFI_Pin_Rx,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_IN_FLOATING
    };
    GPIO_Init(GPIO_WIFI_USART, &gpio_init);
        
    // Заполняем структуру настройками UARTa.
    USART_InitTypeDef uart_struct = 
    {
        .USART_BaudRate = kWiFiUartSpeed,
        .USART_WordLength = USART_WordLength_8b,
        .USART_StopBits = USART_StopBits_1,
        .USART_Parity = USART_Parity_No,
        .USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
        .USART_HardwareFlowControl = USART_HardwareFlowControl_None
    };

    // В методе USART_Init есть ошибка, подробности по ссылке
    // http://we.easyelectronics.ru/STM32/nastroyka-uart-v-stm32-i-problemy-dvoichno-desyatichnoy-arifmetiki.html
    USART_Init(WIFI_USART, &uart_struct);     // Инициализируем UART.
        
    DMA_DeInit(WIFI_DMA_CH_RX);
    DMA_DeInit(WIFI_DMA_CH_TX);
        
    DMA_InitTypeDef dma_init_struct = 
    {
        .DMA_PeripheralBaseAddr = (uint32_t)&(WIFI_USART->DR),
        .DMA_MemoryBaseAddr = memory_base_addr,
        .DMA_DIR = DMA_DIR_PeripheralSRC,
        .DMA_BufferSize = kUartRxFifoSize,
        .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
        .DMA_Mode = DMA_Mode_Circular,
        .DMA_Priority = DMA_Priority_Low,
        .DMA_M2M = DMA_M2M_Disable
    };
    DMA_Init(WIFI_DMA_CH_RX, &dma_init_struct);
        
    // Разрешить DMA для USART.
    USART_DMACmd(WIFI_USART, USART_DMAReq_Rx | USART_DMAReq_Tx, ENABLE);
        
    // Включаем UART.
    USART_Cmd(WIFI_USART, ENABLE);
        
    // Старт приема через DMA.
    DMA_Cmd(WIFI_DMA_CH_RX, ENABLE);
}

void Common::InitI2C()
{
    // 100kHz.
    I2C_InitTypeDef i2c_init_struct = 
    {
        .I2C_ClockSpeed = 100000,
        .I2C_Mode = I2C_Mode_I2C,
        .I2C_DutyCycle = I2C_DutyCycle_2,
        .I2C_OwnAddress1 = 0x15,
        .I2C_Ack = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit
    };
    I2C_Init(I2C_EE_LCD, &i2c_init_struct);
}

void Common::AssertAllTasksInitialized()
{
    //Debug::Assert(g_tempSensorTask != NULL);
    //Debug::Assert(g_heaterTask != NULL);
    Debug::Assert(g_wlAnimationTask != NULL);
    //Debug::Assert(g_wifiTask != NULL);
    Debug::Assert(g_lcdTask != NULL);
    Debug::Assert(g_ledLightTask != NULL);
    Debug::Assert(g_buttonsTask != NULL);
    Debug::Assert(g_valveTask != NULL);
    Debug::Assert(g_watchDogTask != NULL);
}