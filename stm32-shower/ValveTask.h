#pragma once
#include "iActiveTask.h"
#include "semphr.h"

#define Valve_GPIO                  (GPIOA)
#define Valve_Pin                   (GPIO_Pin_11)
#define SensorSwitch_Power_GPIO     (GPIOA)
#define SensorSwitch_Power_Pin      (GPIO_Pin_6)
#define ValveOpened()               (GPIO_ReadInputDataBit(Valve_GPIO, Valve_Pin))
constexpr auto Valve_Delay = 200;

class ValveTask : public iActiveTask
{
private:
	
	enum SensorSwitchState { StateOn, StateOff };
    SensorSwitchState SensorSwitchLastState = SensorSwitchState::StateOff;
	volatile bool StopRequired = false;
	volatile bool OpenValveAllowed = false;
	SemaphoreHandle_t xValveSemaphore;
	StaticSemaphore_t xValveSemaphoreBuffer;
	
	void Init();
	
	// Включает питание сенсора.
	void GpioTurnOnSensorSwitch();
	
	// Выключает питание сенсора.
	void GpioTurnOffSensorSwitch();
	
	void GPIOOpenValve();
	
	void GPIOCloseValve();
	
	void Run();
	
	void OpenValveIfAllowed();
	
public:
	
	// Вызывается только если кнопка была нажата.
	void PushButton();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOn();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOff();
	
	volatile bool ValveIsOpen();
};

extern ValveTask _valveTask;