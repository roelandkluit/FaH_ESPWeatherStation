// BrightnessSensor.h

#ifndef _BRIGHTNESSSENSOR_h
#define _BRIGHTNESSSENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define BIGHTNESS_REFRESH_INTERVAL 30002 // Once every 30 seconds
#define NUMBER_OF_PROBES 10 //Number of probes for average value callculation
#define BIGHTNESS_UPDATE_INTERVAL BIGHTNESS_REFRESH_INTERVAL / NUMBER_OF_PROBES


class BrightnessSensor {
public:
	void Process();	
	BrightnessSensor(const uint8_t& pin);
	void SetOnLuxValueChangeEvent(void(*callback)(const uint16_t& Luxvalue)) { __CB_BRIGHTNESS_CHANGED = callback; }
	uint16_t GetBrightness();
	uint16_t GetRawBrightness();
private:
	void(*__CB_BRIGHTNESS_CHANGED)(const uint16_t& Luxvalue) = NULL;
	unsigned long previousWeatherInfoCollectMillis = 0;
	uint8_t PIN_Sensor;
	uint16_t BrightnessLightLevel = 0xFFFF;
	uint8_t forceUpdateCounter = 0;
	unsigned int readings[NUMBER_OF_PROBES] = { 0 };  // the readings from the analog input
	unsigned int readIndex = 0;          // the index of the current reading
	unsigned int total = 0;              // the running total
	unsigned int averageBrightnessLightLevel = 0;            // the average
	bool ProcessReading(const unsigned int& Value);
};

#endif

