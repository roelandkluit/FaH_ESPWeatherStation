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
// WindSpeed.h

#ifndef _WINDSPEED_h
#define _WINDSPEED_h

//#define BUILD_FOR_TEST_ESP32

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define WIND_REFRESH_INTERVAL 10000 // Once every 10 seconds
#define WINDSPEED_ARRAY_SIZE 60 //Baufort is calculated over 10 minutes, with a refresh every 10 seconds, the array needs to store 60 items
#define WINDSPEED_REMBER_TIME 2 //20 seconds
#define WINDSPEED_MAX_SAMPLE_COUNT 6 // Use last 60 seconds for Wind Speed calculation
#define WINDSPEED_SKIP_NOTIFICATIONS 2 //Only report every 30 seconds (0-2 * WIND_REFRESH_INTERVAL)

//float pi = 3.14159265;
//float radius = 0.8;
//Dual count
//float RPM_FACTOR = ((2 * pi * radius) / 60) * RPMwindspeed;  // Calculate wind speed on m/s
#define RPM_FACTOR 0.041887902

class WindSpeed
{
private:
	void WindFaneInterrupt();
	uint8_t usedInterruptPin = 0;
	volatile unsigned int WindFaneCount = 0;
	unsigned int windspeed_array[WINDSPEED_ARRAY_SIZE] = {0};
	unsigned int AverageWindspeedRPM = 0;
	unsigned int LastRecorderWindSpeedRPM = 0;
	unsigned int MaxWindSpeedAvgRPM = 0;
	float MaxWindGustMS = -1;
	uint8_t SpeedBeaufort = 255;
	uint8_t LastTimeSet = 0;
	uint8_t NoNotifyCounter = 1;
	unsigned long previousWeatherInfoCollectMillis = 0;
	uint8_t Beaufort(const float& Speed);
	void shiftWindspeedArray(const unsigned int& newValue);
	void SetWindspeeds(const float& maxWindGustMS, const uint8_t& SpeedBeaufort);
	float WindSpeedToMsFromRPM(const float& RPMwindspeed);
	void(*__CB_WINDGUST_CHANGED)(const float& maxWindGust) = NULL;
	void(*__CB_WINDBEAUFORT_CHANGED)(const uint8_t& BeaufortSpeed) = NULL;
public:
	String GetValues();
	void SetOnWindGustsChangeEvent(void(*callback)(const float& maxWindGust)) { __CB_WINDGUST_CHANGED = callback; }
	void SetOnWindBeaufortChangeEvent(void(*callback)(const uint8_t& BeaufortSpeed)) { __CB_WINDBEAUFORT_CHANGED = callback; }
	unsigned int currentWindFaneReading = 0;
	float GetWindGusts();
	uint8_t GetSpeedBeaufort();
	WindSpeed(const uint8_t InterruptPin);	
	~WindSpeed();
	void Process();	
	
};

#endif

