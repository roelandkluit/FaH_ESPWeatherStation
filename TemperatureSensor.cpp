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

#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor(const uint8_t &SensorPin)
{
	//this->UsedSensorPin = SensorPin;
	oneWireBus = new OneWire(SensorPin);
	oTemperatureSensor = new DallasTemperature(oneWireBus);
	oTemperatureSensor->begin();
	previousWeatherInfoCollectMillis = millis() - (TEMPERATURE_REFRESH_INTERVAL / 2);
}

TemperatureSensor::~TemperatureSensor()
{
	if (oTemperatureSensor != NULL)
	{
		delete oTemperatureSensor;
		oTemperatureSensor = NULL;
	}
	if (oneWireBus != NULL)
	{
		delete oneWireBus;
		oneWireBus = NULL;
	}
}

void TemperatureSensor::Process()
{
	if (millis() - previousWeatherInfoCollectMillis >= (TEMPERATURE_REFRESH_INTERVAL))
	{
		previousWeatherInfoCollectMillis = millis();
		oTemperatureSensor->requestTemperatures(); // Send the command to get temperature readings
		float tTemperatureCoutside = oTemperatureSensor->getTempCByIndex(0);

		if (tTemperatureCoutside < -40 || tTemperatureCoutside > 60)
		{
			if (MessuredTemperature < -40)
			{
				//Never had a correct reading, sensor broken?
				//Serial.println("Sensor Error?");
				tTemperatureCoutside = 0;
			}
			else
			{
				tTemperatureCoutside = MessuredTemperature;
			}			
		}
		float temperatureCoutside = shiftTemperatureArray(tTemperatureCoutside);
		SetTemperature(temperatureCoutside);
	}
}

float TemperatureSensor::GetTemperature()
{
	return MessuredTemperature;
}

float TemperatureSensor::shiftTemperatureArray(const float& newValue)
{
	if (MessuredTemperature < -40) //Initial
	{
		for (int i = 0; i < TEMPERATURE_AVERAGE_ARRAY_SIZE; i++)
		{
			temparature_array[i] = newValue;
		}
		return newValue;
	}
	else
	{
		float temperatureTotal = 0;
		memcpy(temparature_array, &temparature_array[1], sizeof(temparature_array) - sizeof(float));
		temparature_array[TEMPERATURE_AVERAGE_ARRAY_SIZE - 1] = newValue;

		for (int i = 0; i < TEMPERATURE_AVERAGE_ARRAY_SIZE; i++)
		{
			temperatureTotal += temparature_array[i];
		}
		return temperatureTotal / TEMPERATURE_AVERAGE_ARRAY_SIZE;
	}
}

void TemperatureSensor::SetTemperature(const float& newTemperature)
{
	if (newTemperature != this->MessuredTemperature)
	{
		this->MessuredTemperature = newTemperature;
		if (__CB_TEMPERATURE_CHANGED != NULL)
		{
			__CB_TEMPERATURE_CHANGED(newTemperature);
		}
	}
}