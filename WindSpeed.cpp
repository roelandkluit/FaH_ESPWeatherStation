// 
// 
// 
#include "WindSpeed.h"
#include <FunctionalInterrupt.h>

float WindSpeed::GetWindGusts()
{
    return MaxWindGustMS;
}

uint8_t WindSpeed::GetSpeedBeaufort()
{
    return SpeedBeaufort;
}

WindSpeed::WindSpeed(const uint8_t InterruptPin)
{
    pinMode(InterruptPin, INPUT);
	usedInterruptPin = InterruptPin;
	attachInterrupt(digitalPinToInterrupt(InterruptPin), std::bind(&WindSpeed::WindFaneInterrupt, this), FALLING);
    previousWeatherInfoCollectMillis = millis();
}

WindSpeed::~WindSpeed()
{
	detachInterrupt(usedInterruptPin);
}

void WindSpeed::WindFaneInterrupt()
{
	WindFaneCount++;
}

float WindSpeed::WindSpeedToMsFromRPM(const float &RPMwindspeed)
{
    float mswindspeed = RPM_FACTOR * RPMwindspeed;
    return mswindspeed;
}

uint8_t WindSpeed::Beaufort(const float &Speed)
{
    uint8_t b = pow(pow((Speed / 0.836), 0.33), 2);
    return b;
}

void WindSpeed::shiftWindspeedArray(const unsigned int &newValue)
{
    unsigned long totalspeed = 0;

    MaxWindSpeedRPM = newValue / 2;

    if (newValue > 0)
    {
        if (newValue < MaxWindSpeedRPM)
        {
            MaxWindSpeedRPM = (MaxWindSpeedRPM + newValue) / 2;
        }
        else
        {
            MaxWindSpeedRPM = newValue;
        }
        LastTimeSet = WINDSPEED_REMBER_TIME;
    }

    memcpy(windspeed_array, &windspeed_array[1], sizeof(windspeed_array) - sizeof(unsigned int));
    windspeed_array[WINDSPEED_ARRAY_SIZE - 1] = MaxWindSpeedRPM;

    for (int i = 0; i < WINDSPEED_ARRAY_SIZE; i++)
    {
        totalspeed += windspeed_array[i];
    }
    AverageWindspeedRPM = (totalspeed / WINDSPEED_ARRAY_SIZE);
    #ifdef BUILD_FOR_TEST_ESP32
        Serial.print("Average:");
        Serial.print(WindSpeedToMsFromRPM(AverageWindspeedRPM));
        Serial.print(", BFT:");
        Serial.println(Beaufort(WindSpeedToMsFromRPM(AverageWindspeedRPM)));
    #endif

    if (LastTimeSet == 0)
    {
        MaxWindSpeedRPM = 0;
    }
    else
    {
        LastTimeSet--;
    }
}

void WindSpeed::SetWindspeeds(const float& maxWindGustMS, const uint8_t& windSpeedBeaufort)
{
    if (maxWindGustMS != this->MaxWindGustMS)
    {     
        this->MaxWindGustMS = maxWindGustMS;
        if (__CB_WINDGUST_CHANGED != NULL)
        {
            __CB_WINDGUST_CHANGED(maxWindGustMS);
        }
    }
    if (windSpeedBeaufort != this->SpeedBeaufort)
    {
        this->SpeedBeaufort = windSpeedBeaufort;
        if (__CB_WINDBEAUFORT_CHANGED != NULL)
        {
            __CB_WINDBEAUFORT_CHANGED(SpeedBeaufort);
        }
    }
}

void WindSpeed::Process()
{
    if (millis() - previousWeatherInfoCollectMillis >= (WIND_REFRESH_INTERVAL))
    {
        unsigned int currentWindFaneReading = 0;
        noInterrupts();
        currentWindFaneReading = WindFaneCount;
        WindFaneCount = 0;
        interrupts();
        previousWeatherInfoCollectMillis = millis();
        #ifdef BUILD_FOR_TEST_ESP32
            currentWindFaneReading = int((float(rand()) / float((RAND_MAX)) * 100.0));
        #endif // BUILD_FOR_TEST_ESP32
        shiftWindspeedArray(currentWindFaneReading);

        if (NoNotifyCounter == 0)
        {
            float flMaxWindGustMS = WindSpeedToMsFromRPM(MaxWindSpeedRPM);
            unsigned int ulSpeedBeaufort = Beaufort(WindSpeedToMsFromRPM(AverageWindspeedRPM));
            SetWindspeeds(flMaxWindGustMS, ulSpeedBeaufort);
            NoNotifyCounter = WINDSPEED_SKIP_NOTIFICATIONS;
        }
        else
        {
            NoNotifyCounter--;
        }
    }
}
