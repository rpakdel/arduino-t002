#include <Wire.h>

#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>
#include <nRF24L01.h>

#include <ArduinoJson.h>

#include "../../arduino-t002/shared/joydata.h"
#include "../../arduino-t002/shared/gpsdata.h"

#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

RF24 radio(9, 10);
byte addresses[][6] = { "1Node", "2Node" };

#define ARDUINO_I2C_ADDRESS 0x8

void sendJoystickDataToBot(JoyData& data)
{
    //DEBUG_SERIAL.print(F("Now sending "));
    //DEBUG_SERIAL.print(data.X);
    //DEBUG_SERIAL.print(F(", "));
    //DEBUG_SERIAL.print(data.Y);
    //DEBUG_SERIAL.print(F("..."));

    if (radio.write(&data, sizeof(data)))
    {
        //DEBUG_SERIAL.println(F("ok"));
    }
    else
    {
        //DEBUG_SERIAL.println(F("no ack"));
    }
}

GpsData readGpsFromBot()
{
    if (!radio.isAckPayloadAvailable())
    {
        //DEBUG_SERIAL.println(F("Base> No GPS data"));
        GpsData gpsData;
        gpsData.Id = 0;
        gpsData.IsValid = 0;
        gpsData.Lon = 0;
        gpsData.Lat = 0;
        gpsData.Alt = 0;
        return gpsData;
    }

    //DEBUG_SERIAL.println(F("Base> Got GPS data"));
    GpsData gpsData;
    radio.read(&gpsData, gpsDataSize);
    //printlnGpsData(gpsData, DEBUG_SERIAL);
    return gpsData;
}

void setupRadio()
{
    radio.begin();
    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.setPayloadSize(gpsDataSize);
    radio.setRetries(15, 15);
    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_2MBPS);

    radio.openWritingPipe(addresses[0]);
    radio.stopListening();
}

volatile JoyData joystickData;
volatile bool hasNewJoystickData = false;

void I2CTOnReceive(int howMany)
{
    int readIndex = 0;
    while (Wire.available())
    {
        size_t numBytes = Wire.readBytes((uint8_t*)&joystickData, joyDataSize);
        if (numBytes != joyDataSize)
        {
            return;
        }
        JoyData j;
        copyJoyDataFromVolatile(joystickData, j);
        printlnJoyData(j, DEBUG_SERIAL);
    }

    hasNewJoystickData = true;
}

volatile GpsData gpsData;

void I2CTOnRequest()
{
    uint8_t* buffer = (uint8_t*)&gpsData;
    Wire.write(buffer, gpsDataSize);
    //printlnGpsData(const_cast<GpsData&>(gpsData), DEBUG_SERIAL);
}

void ResetEsp()
{
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
    delay(100);
    digitalWrite(4, HIGH);
}

void setup(void)
{
    ResetEsp();
    // ESP-01 is master, Arduino is slave
    Wire.begin(ARDUINO_I2C_ADDRESS);
    Wire.onReceive(I2CTOnReceive);
    Wire.onRequest(I2CTOnRequest);
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    DEBUG_SERIAL.println(F("Base> Begin setup"));

    setupRadio();

    DEBUG_SERIAL.println(F("Base> Ready"));
}

unsigned long last = 0;

void printLoop()
{
    unsigned long m = millis();
    if (m - last > (5 * 1000))
    {
        last = m;
        DEBUG_SERIAL.println("Base> Loop");
    }
}

unsigned long lastJoystickToBot = 0;

void loop(void)
{
    if (hasNewJoystickData)
    {
        JoyData newJoyData;
        copyJoyDataFromVolatile(joystickData, newJoyData);
        hasNewJoystickData = false;
        //DEBUG_SERIAL.print(F("Base> JoyData: "));
        //printlnJoyData(newJoyData, DEBUG_SERIAL);
        sendJoystickDataToBot(newJoyData);
    }
    else
    {
        unsigned long m = millis();
        if ((m - lastJoystickToBot) > 5000)
        {
            lastJoystickToBot = m;
            JoyData emptyJoyData;
            emptyJoyData.Id = 0;
            emptyJoyData.X = 0;
            emptyJoyData.Y = 0;
            emptyJoyData.Button = 0;
            DEBUG_SERIAL.println("Base> Sending empty JoyData");
            sendJoystickDataToBot(emptyJoyData);
        }
    }

    GpsData newGpsData = readGpsFromBot();
    if (newGpsData.IsValid)
    {
        copyGpsDataToVolatile(newGpsData, gpsData);
    }

    printLoop();
}

