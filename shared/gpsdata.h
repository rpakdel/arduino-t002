#pragma once

#include <Arduino.h>

#pragma pack(1)
typedef struct _GpsData
{
    // bot id
    uint8_t Id;
    // whether gps data is valid
    uint8_t IsValid;
    // longtitude from -180 to 180
    float Lon;
    // latitude from -90 to 90
    float Lat;
    // altitude in meters
    float Alt;
} GpsData;

const size_t gpsDataSize = sizeof(GpsData);

void copyGpsDataToVolatile(GpsData& source, volatile GpsData& target)
{
    target.Id = source.Id;
    target.IsValid = source.IsValid;
    target.Lon = source.Lon;
    target.Lat = source.Lat;
    target.Alt = source.Alt;
}

void printGpsData(GpsData& gpsData, Print& print)
{
    print.print(gpsData.Id);
    print.print(F(","));
    print.print(gpsData.IsValid);
    print.print(F(","));
    print.print(gpsData.Lon);
    print.print(F(","));
    print.print(gpsData.Lat);
    print.print(F(","));
    print.print(gpsData.Alt);
}

void printlnGpsData(GpsData& gpsData, Print& print)
{
    printGpsData(gpsData, print);
    print.println();
}

size_t jsonSerializeGpsData(GpsData &gpsData, char* buffer, size_t bufferSize)
{
    if (!gpsData.IsValid)
    {
        buffer[0] = '\0';
        return 0;
    }

    char latStr[10 + 1 + 1];
    dtostrf(gpsData.Lat, 6, 5, latStr);

    char lonStr[10 + 1 + 1];
    dtostrf(gpsData.Lon, 6, 5, lonStr);

    char altStr[6 + 1 + 1];
    dtostrf(gpsData.Alt, 6, 2, altStr);

    sprintf(buffer, "{ \"Id\": %d, \"IsValid\": %d, \"Lat\": %s, \"Lon\": %s, \"Alt\": %s }\0",
        gpsData.Id,
        gpsData.IsValid,
        latStr,
        lonStr,
        altStr);

    return strlen(buffer);
}