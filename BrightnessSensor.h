// BrightnessSensor.h

#ifndef _BRIGHTNESSSENSOR_h
#define _BRIGHTNESSSENSOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#define BIGHTNESS_REFRESH_INTERVAL 30002 // Once every 30 seconds

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
};

#endif

