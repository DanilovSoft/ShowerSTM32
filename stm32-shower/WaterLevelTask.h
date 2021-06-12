#pragma once
#include "iActiveTask.h"
#include "MedianFilter.h"
#include "MovingAverageFilter.h"

#define WL_SUCCESS                  (0b10000000)
#define WL_RISING_EDGE              (0b01000000)
#define WL_OVERFLOW                 (0b00100000)

// При повышении температуры воздуха на 1 °C скорость звука в нем увеличивается на 0.57 м/с 
// 1% на каждые 5 °С => 0.2% на 1 °С

extern "C"
{
	extern volatile uint8_t TIM_CAPTURE_STA;
	extern volatile uint16_t TIM_CAPTURE_VAL;
}

class WaterLevelTask final : public iActiveTask
{	
public:
	
	// Последнее измеренное значение после усреднений.
	volatile uint16_t AvgUsec = 0;
	
	// Значение -1 означает что последнее измерение не удалось.
    volatile int16_t UsecRaw = -1;
    
	// Если True то расстояние от датчика оказалось слишком малым.
	volatile bool SensorIsBlocked = false;
	
	// Отображаемый уровень воды в баке, %.
	volatile uint8_t DisplayingPercent = 0;
	
	volatile bool Preinitialized = false;
	
	volatile bool Initialized = false;
	
	void InitGPIO_ClearDisplay();
	
	void WaitInitialization();
	
private:
	
	// Горизонтальный прочерк в первом разряде индикатора.
    static const uint8_t kADash = 0b11011111;
	// Горизонтальный прочерк во втором разряде индикатора.
    static const uint8_t kBDash = 0b11111101;
    // Гистерезис на 4 пункта.
    static const uint8_t kHysteresisPoints = 4;
    
	// Рекомендуют измерять не чаще 60мс что бы не получить эхо прошлого сигнала.
	uint8_t m_intervalPauseMsec = 0;
    // Ширина полного диаппазона в микросекундах.
	uint16_t m_usecRange = 0;
	MovingAverageFilter m_movingAverageFilter;
	MedianFilter m_medianFilter;

    // По умолчанию для гистерезиса считать что вода подымается.
    bool m_waterIsRising = true;
	
	void Init();
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
	
	inline void TaskDisplayPercent(uint8_t percent);
	
    inline void TaskDisplayLED(uint8_t Ax, uint8_t Bx);
    
    inline void DisplayLED(uint8_t Ax, uint8_t Bx);
	
    void DisplayLED(uint16_t value);

	void TaskDisplayLED(uint16_t value);
	
    void SPISend(uint16_t data);
   
    void TaskSPISend(uint16_t data);
};

extern WaterLevelTask g_waterLevelTask;
