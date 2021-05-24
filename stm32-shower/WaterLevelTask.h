#pragma once
#include "iActiveTask.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"

#define WL_SPI                      (SPI2)
#define WL_TIM                      (TIM1)
#define WL_GPIO_Trig                (GPIOA)
#define WL_GPIO_Trig_Pin            (GPIO_Pin_9)
#define WL_GPIO_LATCH               (GPIOB)
#define WL_GPIO_LATCH_Pin           (GPIO_Pin_14)
#define WL_GPIO_SPI                 (GPIOB)
#define WL_GPIO_SPI_SCK_Pin         (GPIO_Pin_13)
#define WL_GPIO_SPI_MOSI_Pin        (GPIO_Pin_15)
#define WL_GPIO_TIM                 (GPIOA)
#define WL_GPIO_TIM_Pin             (GPIO_Pin_8)
#define WL_SUCCESS                  (0b10000000)
#define WL_RISING_EDGE              (0b01000000)
#define WL_OVERFLOW                 (0b00100000)

/*
При повышении температуры воздуха на 1 °C скорость звука в нем увеличивается на 0.57 м/с 
1% на каждые 5 °С => 0.2% на 1 °С
http://joxi.ru/a2XkJWtyxePRmg
*/

extern "C"
{
	extern volatile uint8_t TIM_CAPTURE_STA;
	extern volatile uint16_t TIM_CAPTURE_VAL;
}

class WaterLevelTask : public iActiveTask
{	
	// Горизонтальный прочерк в первом разряде индикатора.
    const static uint8_t aDash = 0b11011111;
	// Горизонтальный прочерк во втором разряде индикатора.
    const static uint8_t bDash = 0b11111101;

    // Гистерезис на 4 пункта.
    const static uint8_t HysteresisPoints = 4;
    
	// Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
	uint8_t _intervalPauseMsec = 0;
    // Ширина полного диаппазона в микросекундах.
	uint16_t _usecRange = 0;
	MovingAverageFilter _movingAverageFilter;
	MedianFilter _medianFilter;

    // По умолчанию для гистерезиса считать что вода подымается.
    bool _waterIsRising = true;
	
	void Run();
    bool CheckSensorBlocking(uint16_t usec);
    void FixRawTemp(uint16_t &usec);
	uint8_t InitDisplay();
	bool GetRawUsecTime(uint16_t &usec);
	void FixRange(uint16_t &usec);
	float GetFloatPercent(uint16_t usec);
    static inline uint8_t GetPercent(uint8_t point);
    inline uint16_t ClampRange(uint16_t usec);
    inline uint8_t GetIntPoint(float pointf);
	// Функция возвращает или point или lastPoint.
    inline uint8_t Hysteresis(uint8_t point, uint8_t lastPoint);
    
    // От 0 до 99.
	inline float GetPoint(uint16_t usec);
	
	void Init();
	
	inline void TaskDisplayPercent(uint8_t percent);
	
    inline void TaskDisplayLED(uint8_t Ax, uint8_t Bx);
    
    inline void DisplayLED(uint8_t Ax, uint8_t Bx);
	
    void DisplayLED(uint16_t value);

	void TaskDisplayLED(uint16_t value);
	
    void SPISend(uint16_t data);
   
    void TaskSPISend(uint16_t data);
   

public:
	
	// Последнее измеренное значение после усреднений.
	volatile uint16_t AvgUsec = 0;
    volatile int16_t UsecRaw = -1;
    
	// Если True то расстояние от датчика оказалось слишком малым.
	volatile bool SensorIsBlocked = false;
	
	// Отображаемый уровень воды в баке, %.
	volatile uint8_t DisplayingPercent = 0;
	
	volatile bool Preinitialized = false;
	
	volatile bool Initialized = false;
	
	void InitGPIO_ClearDisplay();
	
	void WaitInitialization();
};

extern WaterLevelTask _waterLevelTask;