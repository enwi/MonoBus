#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <OneBitDisplay.h>
#include <TZ.h>
#include <WiFiClient.h>

#include "util.h"
#include "weatherIcons.h"

#include "../../../src/MonoBus.h"
#include "font/wirmo.h"

constexpr static const uint8_t PACKAGE[] = {0x7E, 0x91, 0x01, 0x1F, 0x05, 0x37, 0x05, 0x02, 0x45, 0x7E};
constexpr static const uint8_t DELAY_MS = 30;

constexpr static const uint8_t ROWS = 28;
constexpr static const uint8_t COLUMNS = 28;

constexpr static const char* LAT = "50.92149";
constexpr static const char* LON = "6.36267";
constexpr static const char* TZ = "Europe/Berlin";

constexpr static const char* SSID = "";
constexpr static const char* PASS = "";

// define a framebuffer
static uint8_t backBuffer[COLUMNS * ROWS];

OBDISP display;
Mono::Bus5 monobus(Serial);
Mono::Panel28x28 panel(0x1);

util::Timer weatherTimer(5 * 60 * 1000); // every 5 minutes
util::Timer everySecond(1000); // every second

ESP8266WiFiMulti wifiMulti;
WiFiClientSecure wifiClient;

void writePixels(const uint8_t startColumn = 0, const uint8_t rows = ROWS, const uint8_t columns = COLUMNS)
{
    if (startColumn > columns || columns > COLUMNS)
    {
        return;
    }

    uint8_t pixelBuffer[4] = {0};

    // Write magic packet
    Serial.write(PACKAGE, sizeof(PACKAGE));
    delay(DELAY_MS);

    for (uint8_t column = startColumn; column < columns; ++column)
    {
        for (uint8_t row = 0; row < rows; ++row)
        {
            int byteIndex = row / 7;
            int bitIndex = row % 7;
            if (backBuffer[(row / 8) * columns + column] >> row % 8 & 0x01)
            {
                // black
                pixelBuffer[byteIndex] &= ~(1 << bitIndex);
            }
            else
            {
                // yellow
                pixelBuffer[byteIndex] |= (1 << bitIndex);
            }
        }

        panel.write(monobus, column, pixelBuffer);
        delay(DELAY_MS);
    }
}

double temperature = 0;
double humidity = 0;
double dayLightDurationH = 0;
uint8_t weatherCode = 0;
void getWeather()
{
    JsonDocument doc;
    // https://api.open-meteo.com/v1/forecast?latitude=50.92149&longitude=6.36267&current=temperature_2m,relative_humidity_2m,weather_code&daily=daylight_duration&timezone=Europe%2FBerlin
    String url = "https://api.open-meteo.com/v1/"
                 "forecast?current=temperature_2m,relative_humidity_2m,weather_code&daily=daylight_duration&windspeed_"
                 "unit=ms&latitude=";
    url += LAT;
    url += "&longitude=";
    url += LON;
    url += "&timezone=";
    url += TZ;
    const bool success = util::httpGetJson(wifiClient, url, doc, 5000);
    if (!success)
    {
        obdFill(&display, OBD_BLACK, 0);
        obdWriteStringCustom(&display, &font7px, 0, 0, (char*)"ER W", OBD_WHITE);
        writePixels();
        return;
    }

    const auto& current = doc["current"];
    temperature = current["temperature_2m"].as<double>();
    humidity = current["relative_humidity_2m"].as<double>();
    weatherCode = current["weather_code"].as<uint8_t>();
    dayLightDurationH = doc["daily"]["daylight_duration"][0].as<double>() / (60 * 60);
}

void showWeather()
{
    char buf[32];

    sprintf(buf, "%.1fC", temperature);
    obdWriteStringCustom(&display, &font7px, 0, 7, buf, OBD_WHITE);
    // obdWriteString(&display, 0, 0, 7, buf, FONT_6x8, OBD_WHITE, 1);

    sprintf(buf, "%.1f%%", humidity);
    obdWriteStringCustom(&display, &font7px, 0, 17, buf, OBD_WHITE);
    // obdWriteString(&display, 0, 0, 17, buf, FONT_6x8, OBD_WHITE, 1);

    const time_t now = time(nullptr);
    const struct tm* timeinfo = localtime(&now);
    const int hour = timeinfo->tm_hour;
    const int minute = timeinfo->tm_min;
    sprintf(buf, "%02d:%02d", hour, minute);
    // sprintf(buf, "%.1fH", dayLightDurationH);
    obdWriteStringCustom(&display, &font7px, 21, 27, buf, OBD_WHITE);
}

String hourToWords(const uint8_t hour)
{
    constexpr static const char* hours[]
        = {"twelve", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven"};
    return hours[hour % 12];
}

String minuteToWords(const uint8_t minute)
{
    constexpr static const char* minutes[] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight",
        "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen",
        "nineteen", "twenty", "twenty-one", "twenty-two", "twenty-three", "twenty-four", "twenty-five", "twenty-six",
        "twenty-seven", "twenty-eight", "twenty-nine"};
    return minutes[minute];
}

String generateTimeString(const uint8_t hour, const uint8_t minute)
{
    String timeString = "It is ";

    if (minute == 0)
    {
        timeString += String(hourToWords(hour)) + " o'clock";
    }
    else if (minute == 30)
    {
        timeString += "half past " + String(hourToWords(hour));
    }
    else if (minute == 15)
    {
        timeString += "quarter past " + String(hourToWords(hour));
    }
    else if (minute == 45)
    {
        timeString += "quarter to " + String(hourToWords((hour + 1) % 24));
    }
    else if (minute < 30)
    {
        timeString += String(minuteToWords(minute)) + " past " + String(hourToWords(hour));
    }
    else
    {
        timeString += String(minuteToWords(60 - minute)) + " to " + String(hourToWords((hour + 1) % 24));
    }

    return timeString;
}

void showTime()
{
    const time_t now = time(nullptr);
    const struct tm* timeinfo = localtime(&now);
    const int hour = timeinfo->tm_hour;
    const int minute = timeinfo->tm_min;

    String timeString = generateTimeString(hour, minute);
    Serial.println(timeString);
    obdWriteStringCustom(&display, &font7px, 28, 7, timeString.c_str(), OBD_WHITE);
}

void connectWifi()
{
    wifiMulti.addAP(SSID, PASS);
    while (wifiMulti.run() != WL_CONNECTED)
    {
        delay(250);
        // Serial.print('.');
    }
}

void timeSync()
{
    configTime(TZ_Europe_Berlin, "pool.ntp.org"); // Configure NTP

    Serial.print("NTP Time Syny...");
    while (!time(nullptr))
    {
        delay(1000);
    }
    Serial.println("Done");
}

void setup()
{
    // needs to be inverted if used with NPN transistor like 2n2222a
    Serial.begin(19200, SERIAL_8N1, SERIAL_FULL, 1, false);
    wifiClient.setInsecure();
    obdCreateVirtualDisplay(&display, 189, 28, backBuffer);

    connectWifi();
    timeSync();

    // Give us some time to breathe
    delay(2000);
}

void everyMinute()
{
    obdFill(&display, OBD_BLACK, 0);
    showWeather();
    // showTime();
    writePixels();
}

uint8_t lastMinute = 255;
void loop()
{
    const auto currentMillis = millis();
    if (weatherTimer.doRun(currentMillis))
    {
        getWeather();
    }

    if (everySecond.doRun(currentMillis))
    {
        const time_t now = time(nullptr);
        const struct tm* timeinfo = localtime(&now);
        if (lastMinute != timeinfo->tm_min)
        {
            lastMinute = timeinfo->tm_min;
            everyMinute();
        }
    }
}
