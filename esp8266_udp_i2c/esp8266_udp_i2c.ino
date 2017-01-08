#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "myssid.h"

#include <ArduinoJson.h>

#include "../../arduino-t002/shared/joydata.h"
#include "../../arduino-t002/shared/gpsdata.h"

int status = WL_IDLE_STATUS;


unsigned int localPort = 6969;      // local port to listen on
IPAddress remoteIP;
uint16_t remotePort;
bool hasConnection;

#define PACKET_MAX_SIZE 255
char packetBuffer[PACKET_MAX_SIZE]; //buffer to hold incoming packet

#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 9600

WiFiUDP Udp;

#define ARDUINO_I2C_ADDR 0x8


void i2cWrite(const char *buffer)
{
    Wire.beginTransmission(ARDUINO_I2C_ADDR);
    Wire.write(buffer);
    Wire.endTransmission();
}

void i2cWriteError(const char* errMsg)
{
    Wire.beginTransmission(ARDUINO_I2C_ADDR);
    Wire.write("ESP-01> Err: ");
    Wire.write(errMsg);
    Wire.endTransmission();
}

void sendSetupToI2C()
{
    char buffer[255];
    sprintf(buffer, "SSID: %s, UDP %s:%u\0", MYSSID, WiFi.localIP().toString().c_str(), localPort);
    DEBUG_SERIAL.print("ESP-01> ");
    DEBUG_SERIAL.println(buffer);
}


void setup()
{
    // ESP-01 is master
    Wire.begin(0, 2);
    // wait a bit if request is not responded to fast enough
    Wire.setClockStretchLimit(15000);
    //Wire.setClock(400000L);

    //Initialize serial and wait for port to open:
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    delay(10);

    WiFi.begin(MYSSID, MYPASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    Udp.begin(localPort);
    sendSetupToI2C();

    delay(500);
}

int serialCharCount = 0;
char serialData[PACKET_MAX_SIZE];

void sendFromI2CToUDP()
{
    Wire.requestFrom(ARDUINO_I2C_ADDR, gpsDataSize);
    GpsData gpsData;
    size_t numReadBytes = 0;
    while (Wire.available())
    {
        numReadBytes = Wire.readBytes((uint8_t*)&gpsData, gpsDataSize);
    }

    if (numReadBytes != gpsDataSize)
    {
        //DEBUG_SERIAL.println("ESP01> Invalid GPS data");
        return;
    }

    if (gpsData.IsValid)
    {
        char buffer[1024];
        jsonSerializeGpsData(gpsData, buffer, 1024);
        //DEBUG_SERIAL.println(buffer);
        Udp.beginPacket(remoteIP, remotePort);
        Udp.write(buffer, strlen(buffer));
        Udp.endPacket();
    }

    /*
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || serialCharCount == PACKET_MAX_SIZE - 1)
        {
            serialData[serialCharCount] = 0;

            if (hasConnection)
            {
                Serial.print("sending ");
                Serial.print(serialData);
                Serial.print(" to ");
                Serial.print(remoteIP.toString());
                Serial.print(":");
                Serial.println(remotePort);

                Udp.beginPacket(remoteIP, remotePort);
                Udp.write(serialData, serialCharCount);
                Udp.endPacket();
            }

            serialCharCount = 0;
        }
        else
        {
            serialData[serialCharCount] = c;
            serialCharCount++;
        }
    }*/
}

void sendJoyDataToI2C(JoyData& joyData)
{
    Wire.beginTransmission(ARDUINO_I2C_ADDR);
    uint8_t* buffer = (uint8_t*)&joyData;
    Wire.write(buffer, sizeof(JoyData));
    uint8_t result = Wire.endTransmission();
    if (result != 0)
    {
        DEBUG_SERIAL.print(F("ESP-01> I2C transmission error: "));
        DEBUG_SERIAL.println(result);
    }
}

void sendFromUDPToI2C()
{
    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        remoteIP = Udp.remoteIP();
        remotePort = Udp.remotePort();
        hasConnection = true;

        if (packetSize > PACKET_MAX_SIZE)
        {
            DEBUG_SERIAL.print("ESP-01> Err: ");
            DEBUG_SERIAL.println("UDP packet is too big");
            return;
        }
        else
        {
            //DEBUG_SERIAL.print("ESP-01> Got a UDP packet: ");
        }

        // read the packet into packetBufffer
        int len = Udp.read(packetBuffer, PACKET_MAX_SIZE);
        if (len > 0)
        {
			DEBUG_SERIAL.println(packetBuffer);
            packetBuffer[len] = '\0';
            if (packetBuffer[0] == 'j')
            {
                JoyData joyData;
                if (jsonDeserializeJoyData(&packetBuffer[1], joyData))
                {
                    //DEBUG_SERIAL.print(F("JoyData: "));
                    //printlnJoyData(joyData, DEBUG_SERIAL);
                    sendJoyDataToI2C(joyData);
                }
            }
        }
    }
}

void loop()
{
    sendFromI2CToUDP();
    sendFromUDPToI2C();
    delay(1);
}
