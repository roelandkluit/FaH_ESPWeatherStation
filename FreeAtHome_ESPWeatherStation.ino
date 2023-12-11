/*************************************************************************************************************
*
* Title			    : FreeAtHome_ESPWeatherStation
* Description:      : Implements the Busch-Jeager / ABB Free@Home API for a ESP32 based Weather Station.
* Version		    : v 0.2
* Last updated      : 2023.12.11
* Target		    : Custom build Weather Station
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

#include <FahESPBuildConfig.h>
//#define DEBUG

#ifdef DEBUG
    #define DEBUG_PL Serial.println
    #define DEBUG_P Serial.print
    #define DEBUG_F Serial.printf
#endif

#ifdef ESP32
#include <dummy.h>
#include <WiFi.h>
#include <WiFiClient.h>
#else
#error "Platform not supported"
#endif

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
uint16_t registrationDelay = 5000;
uint16_t regCount = 0;
uint16_t regCountFail = 0;
uint8_t handler = 0;

constexpr size_t CUSTOM_FIELD_LEN = 40;
constexpr size_t LONLAT_FIELD_LEN = 10;
constexpr std::array<ParamEntry, 6> PARAMS = { {
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
    },
    {
      "dn",
      "Name",
      CUSTOM_FIELD_LEN,
      ""
    }
} };

void RegenCallback(const bool& isRainOrExpected, const float& amount)
{
    if (espWeer != NULL)
    {
        espWeer->SetRainInformation(amount, isRainOrExpected);
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
        espWeer->SetBrightnessLevelLux(amount);
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

void SendLegacyRest()
{
    if (oWindspeed == NULL || oTemperature == NULL || oBrightness == NULL)
    {
        wm.server->send(503, String(F("text/plain")), String(F("Not Ready")));
    }
    else
    {
        char temp[200];
        float wsms = oWindspeed->GetWindGusts();
        unsigned int awsms = oWindspeed->GetSpeedBeaufort();
        unsigned long long uptime = esp_timer_get_time() / 1000 / 1000;
        unsigned long SunLightLevel = oBrightness->GetBrightness();
        float temperatureCoutside = oTemperature->GetTemperature();
        snprintf(temp, 200, String(F("{\"weather\":{\"windmax\":\"%2.1f\",\"windavg\":\"%u\",\"sun\":\"%u\",\"temperature\":\"%4.1f\",\"uptm\":\"%llu\"}}")).c_str(), wsms, awsms, SunLightLevel, temperatureCoutside, uptime);
        wm.server->send(200, String(F("application/json")), temp);
    }
}

void handleDevice()
{
    /*
    String Date = GetDateTime(boot_unixtimestamp);
    #ifdef ESP32
        String Text = "Timestamp: " + Date + " > " + String(boot_unixtimestamp) + "\r\nHeap: " + String(ESP.getFreeHeap()) + "\r\nMaxHeap: " + String(ESP.getMaxAllocHeap()) + "\r\n";
    #else //ESP8266
        String Text = "Timestamp: " + Date + " > " + String(boot_unixtimestamp) + "\r\nHeap: " + String(ESP.getFreeHeap()) + "\r\nMaxHeap: " + String(ESP.getMaxFreeBlockSize()) + "\r\nFragemented:" + String(ESP.getHeapFragmentation()) + "\r\n";
    #endif // ESP32
    */
#ifdef ESP32
    String Text = "Heap: " + String(ESP.getFreeHeap()) + "\r\nMaxHeap: " + String(ESP.getMaxAllocHeap()) + String(F("\r\nFAHESP:")) + freeAtHomeESPapi.Version() + String(F("\r\nConnectCount:")) + String(regCount) + String(F("\r\nConnectFail:")) + String(regCountFail);
#else //ESP8266
    String Text = String(F("Heap: ")) + String(ESP.getFreeHeap()) + String(F("\r\nMaxHeap: ")) + String(ESP.getMaxFreeBlockSize()) + String(F("\r\nFragemented:")) + String(ESP.getHeapFragmentation()) + String(F("\r\nFAHESP:")) + freeAtHomeESPapi.Version() + String(F("\r\nConnectCount:")) + String(regCount) + String(F("\r\nConnectFail:")) + String(regCountFail);
#endif // ESP32

    if (espWeer != NULL)
    {
        Text += String(F("\r\nWind RPM Count: ")) + String(oWindspeed->currentWindFaneReading);
        Text += String(F("\r\nPD: ")) + String(espWeer->GetPendingDatapointCount());
        Text += String(F("\r\nMSC: ")) + String(espWeer->GetMScounter());
    }

    if (oBuienradar != NULL)
    {
        Text += String(F("\r\nBS: ")) + String(oBuienradar->GetLastRequestSucceeded());
        Text += String(F("\r\nWT: ")) + String(oBuienradar->GetWaitTime());
        Text += String(F("\r\nRS: ")) + String(oBuienradar->GetRefreshSecondsRemaining());
        Text += String(F("\r\nLBD: ")) + oBuienradar->LastBodyData;
    }

    wm.server->send(200, String(F("text/plain")), Text.c_str());
}


void setup()
{
    setCpuFrequencyMhz(80);

    #ifdef DEBUG
        Serial.begin(115200);
    #endif
    delay(500);
    DEBUG_PL("Starting in debug mode");

    deviceID = String(F("ESPWeatherStation_")) + String(WIFI_getChipId(), HEX);
    WiFi.mode(WIFI_AP_STA); // explicitly set mode, esp defaults to STA+AP
    wm.setDebugOutput(false);
    wm_helper.Init(0xABBF, PARAMS.data(), PARAMS.size());
    wm.setHostname(deviceID);

    bool res = wm.autoConnect(deviceID.c_str()); // Non password protected AP

    if (!res)
    {
        DEBUG_PL(String(F("Failed to connect")));
        ESP.restart();
    }
    else 
    {
        //if you get here you have connected to the WiFi
        DEBUG_PL(F("connected"));
        WiFi.mode(WIFI_STA);
        wm.startWebPortal();
        wm.setShowInfoUpdate(true);
        SetCustomMenu(String(F("Initializing")));
        wm.server->on("/fah", handleDevice);
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

    wm.server->on("/rest", SendLegacyRest);
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

    menuHtml = String(F("Name:{n}<br/>{1}<br/>{2}<br/><meta http-equiv='refresh' content='10'>\n"));
    menuHtml.replace(T_n, wm_helper.GetSetting(5));
    menuHtml.replace(T_1, State);
    menuHtml.replace(T_2, StatusText);

    DEBUG_PL(StatusText);

    wm.setCustomMenuHTML(menuHtml.c_str());
}

void FahCallBack(FAHESPAPI_EVENT Event, uint64_t FAHID, const char* ptrChannel, const char* ptrDataPoint, void* ptrValue)
{
    if (Event == FAHESPAPI_EVENT::FAHESPAPI_ON_DISPLAYNAME)
    {
        DEBUG_P(F("PARM_DISPLAYNAME "));
        const char* val = ((char*)ptrValue);
        DEBUG_PL(val);
        wm_helper.setSetting(5, val, strlen(val));
    }
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

                DEBUG_PL(F("Create WeatherStation Device"));
                String deviceName = String(F("ESP32 Weer ")) + String(WIFI_getChipId(), HEX);
                const char* val = wm_helper.GetSetting(5);
                if (strlen(val) > 0)
                {
                    deviceName = String(val);
                }
                #ifdef DEBUG
                    espWeer = freeAtHomeESPapi.CreateWeatherStation("TestWeer", deviceName.c_str(), 300);                
                #else
                    espWeer = freeAtHomeESPapi.CreateWeatherStation("WeatherStation", deviceName.c_str(), 300);
                #endif          
                if (espWeer != NULL)
                {                    
                    String FahID = freeAtHomeESPapi.U64toString(espWeer->GetFahDeviceID());
                    espWeer->AddCallback(FahCallBack);
                    SetCustomMenu(String(F("Device Registered: ")) + FahID);
                }
                else
                {
                    SetCustomMenu(String(F("Device Registration Error")));
                    DEBUG_PL(F("Failed to create Virtual device, check user authorizations"));
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
                case 30:
                    oWindspeed->Process();
                    break;
                case 60:
                    oTemperature->Process();
                    break;
                case 90:
                    oBrightness->Process();
                    break;
                default:
                    oBuienradar->Process();
                    delay(1);
                    break;
                }
                handler++;
            }
        }
    }    
}