/*************************************************************************************************************
*
* Title			    : Free-ESPatHome
* Description:      : Library that implements the Busch-Jeager / ABB Free@Home API for ESP8266 and ESP32.
* Version		    : v 0.4
* Last updated      : 2023.10.29
* Target		    : ESP32, ESP8266, ESP8285
* Author            : Roeland Kluit
* Web               : https://github.com/roelandkluit/Free-ESPatHome
* License           : GPL-3.0 license
*
**************************************************************************************************************/
#pragma once
#include "HTTPClient.h"

namespace HTTPREQUEST_STATUSUS
{
	enum HTTPREQUEST_STATUS :uint8_t
	{
		HTTPREQUEST_STATUS_NONE = 0,
		HTTPREQUEST_STATUS_SUCCESS = 1,
		HTTPREQUEST_STATUS_FAILED = 2,
		HTTPREQUEST_STATUS_TIMEOUT = 3,
		HTTPREQUEST_STATUS_PENDING = 4,
	};
}
typedef HTTPREQUEST_STATUSUS::HTTPREQUEST_STATUS HTTPREQUEST_STATUS;

class BuienradarHTTPClient : public HTTPClient
{
private:
	bool PutHTTPRequest(const String& Host, const String& URI);
	bool ConnectToHost(const String& HTTPHost, const int& port);
	HTTPREQUEST_STATUS ProcessHTTPHeaders();
	HTTPREQUEST_STATUS ProcessHTTPBody();
	HTTPREQUEST_STATUS GetHTTPRequestResult();
	String Async_URI;
	String ConnectedHostName;
	//String Async_PostData;
	//String Async_Method;
	HTTPREQUEST_STATUS AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_NONE;
public:
	HTTPREQUEST_STATUS GetAsyncStatus();
	BuienradarHTTPClient();
	bool HTTPRequestAsync(const String& HostName, const int& port, const String& URI);
	void ProcessAsync();
	void ReleaseAsync();
	//bool HTTPRequest(const String& URI, const String& Method, const String& PostData);
	~BuienradarHTTPClient();
};