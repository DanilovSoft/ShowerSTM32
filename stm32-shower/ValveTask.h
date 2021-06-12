#pragma once
#include "iActiveTask.h"
#include "semphr.h"

class ValveTask final : public iActiveTask
{	
public:
	
	// Включен ли клапан воды.
	static bool ValveIsOpen();
	
	// Вызывается только если кнопка была нажата.
	void PushButton();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOn();
	
	// Вызывается каждый раз, после PushButton().
	void SensorOff();

private:
	
	// Время для антидребезга.
	static const auto kValveDebounceMsec = 200;
	
	enum SensorSwitchState { StateOn, StateOff };
	
	SensorSwitchState m_sensorSwitchLastState = SensorSwitchState::StateOff;
	SemaphoreHandle_t m_xValveSemaphore;
	StaticSemaphore_t m_xValveSemaphoreBuffer;
	volatile bool m_stopRequired = false;
	volatile bool m_openValveAllowed = false;
	
	void Init();
	
	void Run();
	
	// Включает питание сенсора.
	void GpioTurnOnSensorSwitch();

	// Выключает питание сенсора.
	void GpioTurnOffSensorSwitch();
	
	void GPIOOpenValve();
	
	void GPIOCloseValve();
		
	void OpenValveIfAllowed();
};

extern ValveTask g_valveTask;
