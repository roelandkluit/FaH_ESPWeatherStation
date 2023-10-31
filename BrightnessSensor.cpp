// 
// 
// 

#include "BrightnessSensor.h"

void BrightnessSensor::Process()
{
    if (millis() - previousWeatherInfoCollectMillis >= (BIGHTNESS_UPDATE_INTERVAL))
    {
        previousWeatherInfoCollectMillis = millis();
        if (ProcessReading(analogRead(PIN_Sensor)))
        {
            //Only update once after full loop of all array values
            uint16_t tBrightnessLightLevel = averageBrightnessLightLevel;
            if (forceUpdateCounter >= 10 || averageBrightnessLightLevel != this->BrightnessLightLevel)
            {
                this->BrightnessLightLevel = averageBrightnessLightLevel;
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
}

bool BrightnessSensor::ProcessReading(const unsigned int& Value)
{
    // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = Value;
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;

    // if we're at the end of the array...
    if (readIndex >= NUMBER_OF_PROBES)
    {
        // ...wrap around to the beginning:
        readIndex = 0;
        // calculate the average:
        averageBrightnessLightLevel = total / NUMBER_OF_PROBES;
        return true;
    }
    return false;
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
