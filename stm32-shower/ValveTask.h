#pragma once
#include "iActiveTask.h"
#include "semphr.h"

// Время для антидребезга.
static const auto ValveDebounceMsec = 200;

class ValveTask : public iActiveTask
{	
public:
	
	// Вызывается только если кнопка была нажата.
	void PushButton();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOn();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOff();
	
	volatile bool ValveIsOpen();

private:
	
	enum SensorSwitchState { StateOn, StateOff };
	
	SensorSwitchState _sensorSwitchLastState = SensorSwitchState::StateOff;
	SemaphoreHandle_t _xValveSemaphore;
	StaticSemaphore_t _xValveSemaphoreBuffer;
	volatile bool _stopRequired = false;
	volatile bool _openValveAllowed = false;
	
	void Init();
	
	// Включает питание сенсора.
	void GpioTurnOnSensorSwitch();
	
	// Выключает питание сенсора.
	void GpioTurnOffSensorSwitch();
	
	void GPIOOpenValve();
	
	void GPIOCloseValve();
	
	void Run();
	
	void OpenValveIfAllowed();
};

extern ValveTask _valveTask;