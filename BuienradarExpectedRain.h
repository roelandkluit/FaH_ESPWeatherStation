/*************************************************************************************************************
*
* Title			    : FreeAtHome_ESPWeatherStation
* Description:      : Implements the Busch-Jeager / ABB Free@Home API for a ESP32 based Weather Station.
* Version		    : v 0.9
* Last updated      : 2023.12.11
* Target		    : Custom build Weather Station
* Author            : Roeland Kluit
* Web               : https://github.com/roelandkluit/Fah_ESPWeatherStation
* License           : GPL-3.0 license
*
**************************************************************************************************************/
// BuienradarExpectedRain.h

#ifndef _BUIENRADAREXPECTEDRAIN_h
#define _BUIENRADAREXPECTEDRAIN_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#define MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST	3

class BuienradarHTTPClient;

class Buienradar
{
private:
	BuienradarHTTPClient *BuienradarRequest = NULL;
	unsigned long previousRefreshMillis = 0;
	unsigned long MillisTimeWaitTime = 5000; //5 Seconds initial wait
	String strLongitude;
	String strLatitude;
	bool isLowRefreshMode = false;
	bool isRainOrExpectedRain = false;
	float amountOfRain = -1; //Set to invalid value to force update first poll
	uint8_t maxForcastLinesToCheck = MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST;
	bool ParseBuienradarData(const String &regendata);
	void ScheduleNextUpdate(const bool &lastUpdateSuccesfull);	
	//void ProcessInternal();
	void SetRainExpected(const bool& isRainOrExpected, const float& amount);
	//void ProcessBuienradar(void* optParm, AsyncHTTPSRequest* request, int readyState);
	void CalculateForcastSampleSize();
	String FixDecimalCount(const String& input);
	void(*__CB_RAIN_EXPECTED_CHANGED)(const bool &isRainOrExpected, const float &amount) = NULL;
	bool lastRequestSucceeded = false;
public:
	void SetOnRainReportEvent(void(*callback)(const bool& isRainOrExpected, const float& amount)) { __CB_RAIN_EXPECTED_CHANGED = callback; }
	~Buienradar();
	Buienradar(const String Latitude, const String Longitude);
	void SetNightMode(const bool& isNightMode);
	float GetExpectedAmountOfRain();
	unsigned long GetWaitTime();
	long GetRefreshSecondsRemaining();
	bool GetLastRequestSucceeded();
	void Process();	
	String LastBodyData = "";
};

#endif

