#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#pragma pack(1)
typedef struct _JoyData
{
    // id
    uint8_t Id;
    // Joy-X value from -255 to 255
    int16_t X;
    // Joy-Y value from -255 to 255
    int16_t Y;
    // value of the button
    uint8_t Button;
} JoyData;

const size_t joyDataSize = sizeof(JoyData);

void copyJoyDataFromVolatile(volatile JoyData& source, JoyData& target)
{
    target.Id = source.Id;
    target.X = source.X;
    target.Y = source.Y;
    target.Button = source.Button;
}

void printJoyData(JoyData& joyData, Print& print)
{
    print.print(joyData.Id);
    print.print(F(","));
    print.print(joyData.X);
    print.print(F(","));
    print.print(joyData.Y);
    print.print(F(","));
    print.print(joyData.Button);
}

void printlnJoyData(JoyData& joyData, Print& print)
{
    printJoyData(joyData, print);
    print.println();
}

bool jsonDeserializeJoyData(char* data, JoyData& joyData)
{
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(data);
    if (!root.success())
    {
        return false;
    }
    joyData.Id = root["Id"];
    joyData.X = root["X"];
    joyData.Y = root["Y"];
    joyData.Button = root["Button"];
    return true;
}

void zeroDeadZoneJoyData(JoyData& joyData, short xyDeadZone, byte buttonDeadZone)
{
    if (abs(joyData.X) <= xyDeadZone)
    {
        joyData.X = 0;
    }

    if (abs(joyData.Y) <= xyDeadZone)
    {
        joyData.Y = 0;
    }

    if (joyData.Button <= buttonDeadZone)
    {
        joyData.Button = 0;
    }
}

void clampJoyDataXY(JoyData& joyData, short min, short max)
{
    if (joyData.X < min)
    {
        joyData.X = min;
    }

    if (joyData.X > max)
    {
        joyData.X = max;
    }

    if (joyData.Y < min)
    {
        joyData.Y = min;
    }

    if (joyData.Y > max)
    {
        joyData.Y = max;
    }
}

bool areJoyDataEqual(JoyData& oldData, JoyData& newData)
{
    // ignore the id

    if (oldData.X != newData.X)
    {
        return false;
    }

    if (oldData.Y != newData.Y)
    {
        return false;
    }

    if (oldData.Button != newData.Button)
    {
        return false;
    }

    return true;
}



