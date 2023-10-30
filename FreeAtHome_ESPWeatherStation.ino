/*************************************************************************************************************
*
* Title			    : FreeAtHome_ESPWeatherStation
* Description:      : Implements the Busch-Jeager / ABB Free@Home API for a ESP32 based Weather Station.
* Version		    : v 0.2
* Last updated      : 2023.10.20
* Target		    : Athom Smart Plug PG01 v2
* Author            : Roeland Kluit
* Web               : https://github.com/roelandkluit/Fah_ESPWeatherStation
* License           : GPL-3.0 license
*
**************************************************************************************************************/

#include "WiFiManager.h" // original from https://github.com/tzapu/WiFiManager
#include "WifiManagerParamHelper.h"

/* Compile using:
* *********************** *********************** *********************** *********************** **********************
Generic ESP32_WROVER Module
* *********************** *********************** *********************** *********************** **********************
*/

#ifdef ESP32
#include <dummy.h>
#include <WiFi.h>
#include <WiFiClient.h>
#else
#error "Platform not supported"
#endif

#define RELAY_CONTACT_GPIO12 12

#include <FreeAtHomeESPapi.h>
#include <FahESPDevice.h>
#include <FahESPWeatherStation.h>

FreeAtHomeESPapi freeAtHomeESPapi;
FahESPWeatherStation* espWeer = NULL;

#include "BuienradarExpectedRain.h"
#include "WindSpeed.h"
#include "TemperatureSensor.h"
#include "BrightnessSensor.h"

#define PIN_WINDSPEED_INTERRUPT 34 //Interrupt (Wind Speed)
#define PIN_ONEWIREBUS_TEMPERATURE 27 //Temperature Outside
#define PIN_LIGHT_SENSOR A0 //Light Intensity
Buienradar* oBuienradar;
WindSpeed* oWindspeed;
TemperatureSensor* oTemperature;
BrightnessSensor* oBrightness;

String deviceID;
String menuHtml;

WiFiManager wm;
WifiManagerParamHelper wm_helper(wm);
uint16_t registrationDelay = 2000;
uint16_t regCount = 0;
uint16_t regCountFail = 0;
uint8_t handler = 0;

constexpr size_t CUSTOM_FIELD_LEN = 40;
constexpr size_t LONLAT_FIELD_LEN = 10;
constexpr std::array<ParamEntry, 5> PARAMS = { {
    {
      "Ap",
      "SysAp",
      CUSTOM_FIELD_LEN,
      ""
    },
    {
      "uG",
      "User Guid",
      CUSTOM_FIELD_LEN,
      ""
    },
    {
      "pw",
      "Password",
      CUSTOM_FIELD_LEN,
      "type=\"password\""
    },
    {
      "lo",
      "Longitude",
      LONLAT_FIELD_LEN,
      ""
    },
    {
      "la",
      "Latitude",
      LONLAT_FIELD_LEN,
      ""
    }
} };

void RegenCallback(const bool& isRainOrExpected, const float& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetRainInformation(amount);
        SetCustomMenu(String(F("Rain Update")));
        //Serial.print("Rain: "); Serial.print(isRainOrExpected); Serial.print(" Amount: "); Serial.println(amount);
    }
}

void WindMSCallback(const float& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetWindGustSpeed(amount);
        //Serial.print("Wind MS: "); Serial.println(amount);
        SetCustomMenu(String(F("Wind Update")));
    }
}

void LightCallback(const uint16_t& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetBrightnessLevelLux(amount, true);
        //Serial.print("Lux: "); Serial.println(amount);
        SetCustomMenu(String(F("Lux Update")));
    }
}

void WindBeaufortCallback(const uint8_t& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetWindSpeedBeaufort(amount);
        //Serial.print("Wind Beaufort: "); Serial.println(amount);
        SetCustomMenu(String(F("Wind Update")));
    }
}

void TemperatureCallback(const float& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetTemperatureLevel(amount);
        //Serial.print("Temperature: "); Serial.println(amount);
        SetCustomMenu(String(F("Temp Update")));
    }
}

void setup()
{
    //Serial.begin(115200);
    setCpuFrequencyMhz(80);
    delay(500);

    deviceID = String(F("ESPWeatherStation_")) + String(WIFI_getChipId(), HEX);
    WiFi.mode(WIFI_AP_STA); // explicitly set mode, esp defaults to STA+AP
    wm.setDebugOutput(false);
    wm_helper.Init(0xABB, PARAMS.data(), PARAMS.size());
    wm.setHostname(deviceID);

    pinMode(RELAY_CONTACT_GPIO12, OUTPUT);
    digitalWrite(RELAY_CONTACT_GPIO12, LOW);

    bool res = wm.autoConnect(deviceID.c_str()); // Non password protected AP

    if (!res)
    {
        //Serial.println(F("Failed to connect"));
        ESP.restart();
    }
    else 
    {
        //if you get here you have connected to the WiFi
        //Serial.println(F("connected"));
        WiFi.mode(WIFI_STA);
        wm.startWebPortal();
        wm.setShowInfoUpdate(true);
        SetCustomMenu(String(F("Initializing")));
        std::vector<const char*> _menuIdsUpdate = {"custom", "sep", "wifi","param","info","update" };
        wm.setMenu(_menuIdsUpdate);
    }

    oWindspeed = new WindSpeed(PIN_WINDSPEED_INTERRUPT);
    oWindspeed->SetOnWindBeaufortChangeEvent(WindBeaufortCallback);
    oWindspeed->SetOnWindGustsChangeEvent(WindMSCallback);

    oBrightness = new BrightnessSensor(PIN_LIGHT_SENSOR);
    oBrightness->SetOnLuxValueChangeEvent(LightCallback);

    oTemperature = new TemperatureSensor(PIN_ONEWIREBUS_TEMPERATURE);
    oTemperature->SetOnTemperatureChangeEvent(TemperatureCallback);

    String lon = wm_helper.GetSetting(3);
    String lat = wm_helper.GetSetting(4);

    if (lon.length() == 0 || lat.length() == 0)
    {
        lon = String(F("52.22"));
        lat = String(F("4.53"));
    }

    oBuienradar = new Buienradar(lon, lat);
    oBuienradar->SetOnRainReportEvent(RegenCallback);
}

String GetWeerStatus()
{
    String WeerInfo = String(F("Temp: {0}<br>Light: {1}<br>WindMS: {2}<br>WindBau: {3}<br>Rain: {4}"));
    WeerInfo.replace("{0}", String(oTemperature->GetTemperature()));
    WeerInfo.replace("{1}", String(oBrightness->GetBrightness()));
    WeerInfo.replace("{2}", String(oWindspeed->GetWindGusts()));
    WeerInfo.replace("{3}", String(oWindspeed->GetSpeedBeaufort()));
    WeerInfo.replace("{4}", String(oBuienradar->GetExpectedAmountOfRain()));
    return WeerInfo;
}

void SetCustomMenu(String StatusText)
{
    String State = "";

    if (espWeer != NULL)
    {
        State = GetWeerStatus();
    }

    menuHtml = String(F("{1}<br/>{2}<br/><meta http-equiv='refresh' content='10'>\n"));
    menuHtml.replace(T_1, State);
    menuHtml.replace(T_2, StatusText);

    wm.setCustomMenuHTML(menuHtml.c_str());
}

void loop()
{
    wm.process();

    if (registrationDelay > 0)
    {
        registrationDelay--;
        delay(1);
    }
    else
    {
        if (!freeAtHomeESPapi.process())
        {
            /*
            Serial.println(String("SysAp: ") + wm_helper.GetSetting(0));
            Serial.println(String("User: ") + wm_helper.GetSetting(1));
            Serial.println(String("Pwd: ") + wm_helper.GetSetting(2));*/
            if ((strlen(wm_helper.GetSetting(0)) > 0) && (strlen(wm_helper.GetSetting(1)) > 0) && (strlen(wm_helper.GetSetting(2)) > 0))
            {
                //Serial.println(F("Connecting WebSocket"));
                if (!freeAtHomeESPapi.ConnectToSysAP(wm_helper.GetSetting(0), wm_helper.GetSetting(1), wm_helper.GetSetting(2), false))
                {
                    SetCustomMenu(String(F("SysAp connect error")));
                    //Prevent to many retries
                    registrationDelay = 10000;
                    regCountFail++;
                }
                else
                {
                    SetCustomMenu(String(F("SysAp connected")));
                    regCount++;
                }
            }
            else
            {
                SetCustomMenu(String(F("No SysAp configuration")));
                registrationDelay = 1000;
            }
        }
        else
        {
            if (espWeer == NULL)
            {
                //Clear string for as high as possible heap placement
                wm.setCustomMenuHTML(NULL);
                menuHtml = "";

                //Serial.println(F("Create Switch Device"));
                String deviceName = String(F("ESP32 Weer ")) + String(WIFI_getChipId(), HEX);
                espWeer = freeAtHomeESPapi.CreateWeatherStation("WeatherStation", deviceName.c_str(), 300);
                if (espWeer != NULL)
                {                    
                    String FahID = freeAtHomeESPapi.U64toString(espWeer->GetFahDeviceID());
                    SetCustomMenu(String(F("Device Registered: ")) + FahID);
                }
                else
                {
                    SetCustomMenu(String(F("Device Registration Error")));
                    //Serial.println(F("Failed to create Virtual device, check user authorizations"));
                    registrationDelay = 30000;
                }
            }
            else
            {
                switch (handler)
                {
                case 0:
                    oBuienradar->SetNightMode(freeAtHomeESPapi.isNightForSysAp());
                    break;
                case 20:
                    oBuienradar->Process();
                    break;
                case 40:
                    oWindspeed->Process();
                    break;
                case 60:
                    oTemperature->Process();
                    break;
                case 80:
                    oBrightness->Process();
                    break;
                default:
                    delay(1);
                    break;
                }
                handler++;
            }
        }
    }
}