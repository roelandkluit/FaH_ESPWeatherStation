// 
// 
// 

#include "BrightnessSensor.h"

void BrightnessSensor::Process()
{
    if (millis() - previousWeatherInfoCollectMillis >= (BIGHTNESS_REFRESH_INTERVAL))
    {
        previousWeatherInfoCollectMillis = millis();
        uint16_t tBrightnessLightLevel = analogRead(PIN_Sensor);

        if (forceUpdateCounter >= 10 || tBrightnessLightLevel != this->BrightnessLightLevel)
        {
            this->BrightnessLightLevel = tBrightnessLightLevel;
            if (__CB_BRIGHTNESS_CHANGED != NULL)
            {
                uint16_t B2 = tBrightnessLightLevel + pow((tBrightnessLightLevel / 15), 2);
                __CB_BRIGHTNESS_CHANGED(B2);
                forceUpdateCounter = 0;
            }
        }
        else
        {
            forceUpdateCounter++;
        }
    }
}

BrightnessSensor::BrightnessSensor(const uint8_t& pin)
{
    pinMode(pin, INPUT);	
	this->PIN_Sensor = pin;
    previousWeatherInfoCollectMillis = millis();
}

uint16_t BrightnessSensor::GetBrightness()
{
    return BrightnessLightLevel + pow((BrightnessLightLevel / 15), 2);
}

uint16_t BrightnessSensor::GetRawBrightness()
{
    return BrightnessLightLevel;
}
