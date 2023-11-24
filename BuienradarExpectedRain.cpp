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
#include "BuienradarExpectedRain.h"
#include "BuienradarHTTPClient.h"

Buienradar::~Buienradar()
{
    if (BuienradarRequest != NULL)
    {
        delete BuienradarRequest;
        BuienradarRequest = NULL;
    }
}
Buienradar::Buienradar(const String Latitude, const String Longitude)
{
    this->strLongitude = FixDecimalCount(Longitude);
    this->strLatitude = FixDecimalCount(Latitude);
    BuienradarRequest = new BuienradarHTTPClient();
    previousRefreshMillis = millis();
}

void Buienradar::SetNightMode(const bool& isNightMode)
{
    if (isLowRefreshMode != isNightMode)
    {
        isLowRefreshMode = isNightMode;
    }
}

float Buienradar::GetExpectedAmountOfRain()
{
    return this->amountOfRain;
}

String Buienradar::FixDecimalCount(const String &input)
{
    if (input.indexOf('.') > 0)
    {
        int decimalcount = input.length() - input.indexOf('.') - 1;
        if (decimalcount == 0)
        {
            return input + '0';
        }
        else if (decimalcount > 2)
        {
            return input.substring(0, input.indexOf('.') + 2);
        }
    }
    return input;
}

void Buienradar::ScheduleNextUpdate(const bool& lastUpdateSuccesfull)
{
    previousRefreshMillis = millis();
    if (!lastUpdateSuccesfull)
    {
        MillisTimeWaitTime = 30000; //30 seconden retry     
    }
    else if (isLowRefreshMode)
    {
        MillisTimeWaitTime = 30 * 60000; //60 seconden * 30 minuten
    }
    else
    {
        if (isRainOrExpectedRain)
        {
            MillisTimeWaitTime = 5 * 60000; //60 seconden * 5 minuten
        }
        else
        {
            MillisTimeWaitTime = 15 * 60000; //60 seconden * 15 minuten
        }
    }
    /*
    Serial.print(String(F("Next update in: ")));
    Serial.print(MillisTimeWaitTime / 1000);
    Serial.println(String(F(" sec")));
    */
}

unsigned long Buienradar::GetWaitTime()
{
    return (MillisTimeWaitTime / 1000);
}

long Buienradar::GetRefreshSecondsRemaining()
{
    return ((long(millis() - previousRefreshMillis) + long(MillisTimeWaitTime)) / 1000);
}

bool Buienradar::GetLastRequestSucceeded()
{
    return this->lastRequestSucceeded;
}

void Buienradar::SetRainExpected(const bool &isRainOrExpected, const float &amount)
{
    bool isChanged = false;
    if (isRainOrExpectedRain != isRainOrExpected)
    {
        //Serial.print("Change of rain expected: "); Serial.println(isRainOrExpected);
        isRainOrExpectedRain = isRainOrExpected;
        isChanged = true;
    }
    if (amountOfRain != amount)
    {
        //Serial.print("Change of amount: "); Serial.println(amount);
        amountOfRain = amount;
        isChanged = true;
    }

    if (isChanged && __CB_RAIN_EXPECTED_CHANGED != NULL)
    {
        __CB_RAIN_EXPECTED_CHANGED(isRainOrExpected, amount);
    }
}

void Buienradar::Process()
{
    if (BuienradarRequest->GetAsyncStatus() == HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
    {
        if ((millis() - BuienradarRequest->GetSessionStartMillis()) > HTTP_SESSION_TIMEOUT_MS)
        {
            #ifdef DEBUG
                Serial.println(String(F("Rain Refresh Timeout")));
            #endif // DEBUG

            BuienradarRequest->ReleaseAsync();
            ScheduleNextUpdate(false);
        }
        else
        {
            BuienradarRequest->ProcessAsync();
        }
    }
    else if (BuienradarRequest->GetAsyncStatus() == HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS)
    {
        //Serial.println("Completed");
        String BodyData = BuienradarRequest->GetBody();
        this->LastBodyData = "[" + BodyData + "]";
        if (ParseBuienradarData(BodyData))
        {
            lastRequestSucceeded = true;
        }
        else
        {
            lastRequestSucceeded = false;
        }
        //Serial.println(returndata);
        BuienradarRequest->ReleaseAsync();
        ScheduleNextUpdate(lastRequestSucceeded);
    }
    else if ((millis() - previousRefreshMillis) >= MillisTimeWaitTime)
    {
        #ifdef DEBUG
            Serial.print(String(F("Rain Refresh: ")));
        #endif // DEBUG

        String URI = String(F("/data/raintext/?lat=")) + strLatitude + String(F("&lon=")) + strLongitude;
        if (BuienradarRequest->HTTPRequestAsync("gpsgadget.buienradar.nl", 443, URI))
        {
            #ifdef DEBUG
                Serial.println(String(F("requested")));
            #endif // DEBUG
            //ASync started, reset counter
            ScheduleNextUpdate(false);
        }
        else
        {
            #ifdef DEBUG
                Serial.println(String(F("failed")));
            #endif // DEBUG
            ScheduleNextUpdate(false);
        }
    }
}

void Buienradar::CalculateForcastSampleSize()
{
    if (isLowRefreshMode)
    {
        if (isRainOrExpectedRain)
        {
            maxForcastLinesToCheck = MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST * 6;
        }
        else
        {
            maxForcastLinesToCheck = MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST * 4;
        }
    }
    else
    {   
        if (isRainOrExpectedRain)
        {
            maxForcastLinesToCheck = MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST * 2;
        }
        else
        {
            maxForcastLinesToCheck = MAX_TIME_SEGEMENTS_TO_USE_FOR_RAIN_FORECAST;
        }    
    }

    //Serial.print("Numbr of Lines to check: "); Serial.println(maxForcastLinesToCheck);
}

bool Buienradar::ParseBuienradarData(const String &regendata)
{
    if (regendata.length() < 20)
    {
        //Serial.println("Invalid data");
        return false;
    }

    CalculateForcastSampleSize();

    float ldCurrentAmountOfRain = 0;
    bool blRainExpectedOrRaining = false;
    uint lineCount = 0;

    int startPos = 0;
    while (startPos >= 0)
    {
        int endPos = regendata.indexOf("\n", startPos);
        if (endPos < 0)
        {
            //Serial.println(F("NoMoreEndlines"));
            break;
        }

        int sliderPos = regendata.indexOf("|", startPos);
        if (sliderPos < 0)
        {
            //Serial.println(F("InvalidSliderPos"));
            break;
        }
        float val = regendata.substring(startPos, sliderPos).toInt();

        if (val > 0)
        {
            blRainExpectedOrRaining = true; //If rain is expected for the next X messurements, set it to true
            if (lineCount == 0) //Use the value for the first upcomming messurement
            {
                ldCurrentAmountOfRain = pow(10, ((val - 109) / 32));
            }
            //We have rain detected, no need to look for more records
            break;
        }
        //Serial.print('~'); Serial.print(val);

        startPos = endPos + 1;

        if (lineCount != 0 && lineCount >= maxForcastLinesToCheck)
        {
            break;
        }
        lineCount++;
    }
    //Serial.println();
    SetRainExpected(blRainExpectedOrRaining, ldCurrentAmountOfRain);
    return true;
}