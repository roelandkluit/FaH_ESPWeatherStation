# FreeAtHome ESP Weather Station

## About ##
ESP32 firmware to create a Free@Home Weather Station.</br>

## Pre-requisites ##
- The Local API must be enable on the Free@Home system access point
- The supported device is a ESP32 with the following perhipals connected:
* WIND SPEED FANE connected to pin 34 -> Interrupt for Wind Speed, i'm using the WH-SP-WS01
* ONE WIRE BUS TEMPERATURE SENSOR connected to pin 27 -> Temperature Outside, i'm using a DS18B20 waterproof probe
* LIGHT SENSOR connected to pin A0 -> Messure Light Intensity, i'm using a LDR Photo Light Sensitive Resistor
- The API uses a GUID instead of a username for authentication; retrieve the user GUID from:
http://sysap/swagger/users


## Notes ##
The firmware is intended for a custom build device.
***
Buienradar (Dutch) is used a source for Rain prediction
***
THE CONTENT AND INSTRUCTIONS ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IT ALSO DOES NOT WARRANT THE ACCURACY OF THE INFORMATION IN THE CONTENT.
***
