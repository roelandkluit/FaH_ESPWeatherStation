/*************************************************************************************************************
*
* Title			    : FreeAtHome_ESPWeatherStation
* Description:      : Implements the Busch-Jeager / ABB Free@Home API for a ESP32 based Weather Station.
* Version		    : v 0.2
* Last updated      : 2023.10.20
* Target		    : Custom build Weather Station
* Author            : Roeland Kluit
* Web               : https://github.com/roelandkluit/Fah_ESPWeatherStation
* License           : GPL-3.0 license
*
**************************************************************************************************************/
// TemperatureSensor.h

#ifndef _TEMPERATURE_h
#define _TEMPERATURE_SENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMPERATURE_REFRESH_INTERVAL 50005 // Once every 50 seconds (and 5 ms for time drift)
#define TEMPERATURE_AVERAGE_ARRAY_SIZE 5

class TemperatureSensor
{
public:
	TemperatureSensor(const uint8_t &SensorPin);
	~TemperatureSensor();
	void Process();	
	void SetOnTemperatureChangeEvent(void(*callback)(const float& Temperature)) { __CB_TEMPERATURE_CHANGED = callback; }
	float GetTemperature();
private:
	OneWire* oneWireBus = NULL;
	DallasTemperature* oTemperatureSensor = NULL;
	unsigned long previousWeatherInfoCollectMillis = 0;
	void(*__CB_TEMPERATURE_CHANGED)(const float& Temperature) = NULL;
	float temparature_array[TEMPERATURE_AVERAGE_ARRAY_SIZE] = {0};
	float MessuredTemperature = -50; //Initial temperature to force update
	float shiftTemperatureArray(const float& newValue);
	void SetTemperature(const float& newTemperature);
};

#endif

