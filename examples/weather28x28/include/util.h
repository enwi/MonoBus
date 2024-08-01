#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

namespace util
{
    /// @brief Do a GET request to the given url and fill the given json with the response
    /// @param url Request URL
    /// @param json Json document to fill
    /// @param timeout Request timeout
    /// @return True on success, false on error (httpCode != 200 || deserialization error)
    static bool httpGetJson(
        WiFiClient& client, String& url, JsonDocument& json, const uint16_t timeout, JsonDocument* filter = nullptr)
    {
        HTTPClient http;
        // logLine("http httpGetJson " + url);
        http.begin(client, url);
        http.setTimeout(timeout);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        const int httpCode = http.GET();
        if (httpCode != 200)
        {
            http.end();
            Serial.printf("[httpGetJson] %s code %d", url.c_str(), httpCode);
            return false;
        }

        DeserializationError error;
        if (filter)
        {
            error = deserializeJson(json, http.getString(), DeserializationOption::Filter(*filter));
        }
        else
        {
            error = deserializeJson(json, http.getString());
        }
        http.end();
        if (error)
        {
            Serial.printf("[httpGetJson] JSON: %s\r\n", error.c_str());
            return false;
        }
        return true;
    }

    /// @brief Timer for kind of scheduling things
    class Timer
    {
    public:
        /// @brief Construct a timer
        /// @param interval Interval in ms at which @ref doRun() returns true
        /// @param delay Delay in ms until first execution to defer start
        Timer(const unsigned long interval, const unsigned long delay = 0)
            : m_interval(interval), m_nextMillis(millis() + delay)
        { }

        /// @brief Change the interval
        /// @param interval New interval in ms
        void setInterval(const unsigned long interval) { m_interval = interval; }

        /// @brief Check if interval is met
        /// @param currentMillis Optionally provide the current time in millis
        /// @return True if interval is met, false if not
        bool doRun(const unsigned long currentMillis = millis())
        {
            if (currentMillis >= m_nextMillis)
            {
                m_nextMillis = currentMillis + m_interval;
                return true;
            }
            return false;
        }

    private:
        /// @brief Timer interval in ms
        unsigned long m_interval;
        /// @brief Next timeer interval in ms
        unsigned long m_nextMillis;
    };
} // namespace util
