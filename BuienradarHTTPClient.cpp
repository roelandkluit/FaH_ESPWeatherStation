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
#include "BuienradarHTTPClient.h"

HTTPREQUEST_STATUS BuienradarHTTPClient::GetAsyncStatus()
{
	return AsyncStatus;
}

void BuienradarHTTPClient::ReleaseAsync()
{
	if (this->GetState() >= HTTPCLIENT_STATE::HTTPCLIENT_STATE_CONNECTED)
	{
		this->abort();
	}

	Async_URI = "";
	AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_NONE;	
}

BuienradarHTTPClient::BuienradarHTTPClient() :HTTPClient(true)
{
}

bool BuienradarHTTPClient::ConnectToHost(const String &HTTPHost, const int &port)
{
	if (this->GetState() <= HTTPCLIENT_STATE::HTTPCLIENT_STATE_CLOSED)
	{
		if (!this->Connect(HTTPHost.c_str(), port))
		{
			this->abort();
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool BuienradarHTTPClient::HTTPRequestAsync(const String &HostName, const int &port, const String& URI)
{
	if (AsyncStatus != HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
	{
		if (!ConnectToHost(HostName, port))
		{
			DEBUG_PL(F("Failed to Connect"));
			return false;
		}
		ConnectedHostName = HostName;
		Async_URI = URI;
		AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING;
	}
	return true;
}

void BuienradarHTTPClient::ProcessAsync()
{
	HTTPREQUEST_STATUS reqStatus;

	switch (this->GetState())
	{
		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_INITIAL:
		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_CLOSED:
			if (AsyncStatus == HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
			{
				//Todo Check payload (getBody) size?
				AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS;
			}
			return;

		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_FAILED:
			AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;

		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_CONNECTED:
			if (!PutHTTPRequest(ConnectedHostName, Async_URI))
			{
				DEBUG_PL(F("ASYNC_HTTP Failed"));
			}
			return;

		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_REQUESTED:
			reqStatus = GetHTTPRequestResult();
			if (reqStatus ==  HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
			{
				return;
			}
			else if (reqStatus != HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS)
			{
				DEBUG_PL(F("ASYNC_HTTP Response Failed"));
			}
			return;

		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_HEADERS:
			reqStatus = ProcessHTTPHeaders();
			if (reqStatus == HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
			{
				return;
			}
			else if(reqStatus != HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS)
			{
				DEBUG_PL(F("ASYNC_HTTP Headers Failed"));
			}
			return;

		case HTTPCLIENT_STATE::HTTPCLIENT_STATE_DATA:
			reqStatus = ProcessHTTPBody();

			if (reqStatus == HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING)
			{
				return;
			}
			else if (reqStatus != HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS)
			{
				DEBUG_PL(F("ASYNC_HTTP Data Failed"));
			}
			else
			{
				AsyncStatus = HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS;				
				DEBUG_PL("SuccesAsyncA");				
			}
			return;

		default:
			return;
	}
}

bool BuienradarHTTPClient::PutHTTPRequest(const String& Host, const String& URI)
{
	if (this->GetState() == HTTPCLIENT_STATE::HTTPCLIENT_STATE_CONNECTED)
	{
		//String appjson = String(F("application/json"));
		//this->AddRequestHeader(String(F("Content-Type")), appjson);
		//this->AddRequestHeader(String(F("Accept")), appjson);
		this->AddRequestHeader(String(F("Host")), Host);

		if (!this->Request(String(F("GET")), URI, ""))
		{
			this->abort();
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

HTTPREQUEST_STATUS BuienradarHTTPClient::ProcessHTTPHeaders()
{
	if (this->GetState() == HTTPCLIENT_STATE::HTTPCLIENT_STATE_HEADERS)
	{
		String Key;
		String Value;
		if (this->ReadHeaders(Key, Value))
		{
			//DEBUG_P("hdr: "); DEBUG_P(Key);	DEBUG_P("-->"); DEBUG_PL(Value);
			return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING;
		}
		else
		{
			return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS;
		}
	}
	return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;
}

HTTPREQUEST_STATUS BuienradarHTTPClient::ProcessHTTPBody()
{
	if (this->GetState() == HTTPCLIENT_STATE::HTTPCLIENT_STATE_DATA)
	{
		this->ReadPayload();
		return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING;
	}

	if (this->GetState() == HTTPCLIENT_STATE::HTTPCLIENT_STATE_CLOSED)
	{
		return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS;
	}
	return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;
}

HTTPREQUEST_STATUS BuienradarHTTPClient::GetHTTPRequestResult()
{
	if (this->GetState() == HTTPCLIENT_STATE::HTTPCLIENT_STATE_REQUESTED)
	{
		uint16_t resultcode = 0xFFFF;
		if (this->ReadResult(&resultcode))
		{
			if (resultcode != 200)
			{
				DEBUG_P(F("HTTP_CLIENT_FAILED: HTTP_STATUS_"));
				DEBUG_PL(resultcode);
				this->abort();
				return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;
			}
			else
			{
				return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_SUCCESS;
			}
		}
		else
		{
			if(resultcode == 0)
				return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_PENDING;
			else
				return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;
		}
	}
	return HTTPREQUEST_STATUS::HTTPREQUEST_STATUS_FAILED;
}

BuienradarHTTPClient::~BuienradarHTTPClient()
{
}
