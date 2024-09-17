/*
   Inkplate4_OpenWeather_Station example for Soldered Inkplate 4
   For this example you will need only a USB-C cable and Inkplate 4.
   Select "Soldered Inkplate4" from Tools -> Board menu.
   Don't have "Soldered Inkplate4" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   This example will show you how you can use Inkplate 4 to display API data,
   e.g. OpenWeather public API. Response from server is in JSON format, so that
   will be shown too how it is used. What happens here is basically ESP32 connects
   to the WiFi and sends API call and server returns data in JSON format containing
   information about weather, then using library ArduinoJson we extract weather data
   from JSON data and show it on Inkplate 4.

   IMPORTANT:
   Make sure to change your desired city and wifi credentials below.
   Also have ArduinoJson and TimeLib installed in your Arduino libraries:
   https://arduinojson.org/
   https://github.com/PaulStoffregen/Time

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   4 April 2023 by Soldered

   Code for Moonphase and moon fonts taken from here: https://learn.adafruit.com/epaper-weather-station/arduino-setup
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE4
#error "Wrong board selection for this example, please select Soldered Inkplate4 in the boards menu."
#endif

// Required libraries
#include "ArduinoJson.h"
#include "HTTPClient.h"
#include "Inkplate.h"
#include "OpenWeatherOneCall.h"
#include "TimeLib.h"
#include "WiFi.h"

// ---------- CHANGE HERE  -------------:

// Change to your wifi ssid and password
#define SSID ""
#define PASS ""
// Variable to store WiFi connection timeout
int timeoutSeconds = 100;

// Openweather API key
/**
 * Note: The OneCall API has moved on to version 3.0,
 * In this sketch we are still using 2.5, which is free.
 * The only requirement is that you need to have an API key older than approx. early 2023.
 * Those API keys are still valid for OneCall 2.5
 * 
 * If your key is invalid, you will be notified by the sketch
 * 
*/
char * APIKEY = "";
// Also, declare the function to check if the API key is valid
bool checkIfAPIKeyIsValid(char * APIKEY);

float myLatitude = 45.560001; // I got this from Wikipedia
float myLongitude = 18.675880;

bool metric = true; // <--- true is METRIC, false is IMPERIAL

// Delay between API calls
#define DELAY_MS 59000
#define DELAY_WIFI_RETRY_SECONDS 5
#define DELAY_API_RETRY_SECONDS 5

// ---------------------------------------

// Including fonts used
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/Picopixel.h"
#include "Fonts/Roboto_Light_120.h"
#include "Fonts/Roboto_Light_36.h"
#include "Fonts/Roboto_Light_48.h"
#include "Fonts/moon_Phases12pt7b.h"

// Inkplate object
Inkplate display;

enum alignment
{
    LEFT_BOT,
    CENTRE_BOT,
    RIGHT_BOT,
    LEFT,
    CENTRE,
    RIGHT,
    LEFT_TOP,
    CENTRE_TOP,
    RIGHT_TOP
};

const char *moonphasenames[29] = {
    "New Moon",        "Waxing Crescent", "Waxing Crescent", "Waxing Crescent", "Waxing Crescent", "Waxing Crescent",
    "Waxing Crescent", "Quarter",         "Waxing Gibbous",  "Waxing Gibbous",  "Waxing Gibbous",  "Waxing Gibbous",
    "Waxing Gibbous",  "Waxing Gibbous",  "Full Moon",       "Waning Gibbous",  "Waning Gibbous",  "Waning Gibbous",
    "Waning Gibbous",  "Waning Gibbous",  "Waning Gibbous",  "Last Quarter",    "Waning Crescent", "Waning Crescent",
    "Waning Crescent", "Waning Crescent", "Waning Crescent", "Waning Crescent", "Waning Crescent"};

// Variable for storing temperature unit
char tempUnit;

// Where to start drawing the daily forecast
const int dayOffset = 110;

char Output[200] = {0};

OpenWeatherOneCall OWOC; // Invoke OpenWeather Library
time_t t = now();

void GetCurrentWeather()
{
    //=================================
    // Get the Weather Forecast
    //=================================

    Serial.println("Getting weather");
    OWOC.parseWeather(APIKEY, NULL, myLatitude, myLongitude, metric, NULL);
    setTime(OWOC.current.dt);
    t = now();

    Serial.println(OWOC.current.timezone_offset);

    //=================================================
    // Today's results
    //=================================================
    Serial.println("");
    Serial.println("Current: ");
    sprintf(Output, "%s: %02d:%02d,%02d:%02d-%02d:%02d,%02.01f%c,%04.0fhPa,%02.01f%%Rh,%s", dayShortStr(weekday(t)),
            hour(t), minute(t), hour(OWOC.current.sunrise), minute(OWOC.current.sunrise), hour(OWOC.current.sunset),
            minute(OWOC.current.sunset), OWOC.current.temp, tempUnit, OWOC.current.pressure, OWOC.current.humidity,
            OWOC.current.icon);
    Serial.println(Output);
    Serial.println("");

    Serial.println("Hourly Forecast:");
    for (int Houry = 0; Houry < (sizeof(OWOC.hourly) / sizeof(OWOC.hourly[0])); Houry++)
    {
        sprintf(Output, "%02d:%02d:%02.02f%c,%02.02fmm,%02.02fmm,%s", hour(OWOC.hourly[Houry].dt),
                minute(OWOC.hourly[Houry].dt), OWOC.hourly[Houry].temp, tempUnit, OWOC.hourly[Houry].rain_1h,
                OWOC.hourly[Houry].snow_1h, OWOC.hourly[Houry].icon);
        Serial.println(Output);
    }
    Serial.println("");

    Serial.println("7 Day Forecast:");
    for (int y = 0; y < 3; y++)
    {
        sprintf(
            Output, "%s:%02d:%02d-%02d:%02d,%02.01f%c,%02.01f%c,%04.0fhPa,%02.01f%%Rh,%03.0f%%,%02.02fmm,%02.02fmm,%s",
            dayShortStr(weekday(OWOC.forecast[y].dt)), hour(OWOC.forecast[y].sunrise), minute(OWOC.forecast[y].sunrise),
            hour(OWOC.forecast[y].sunset), minute(OWOC.forecast[y].sunset), OWOC.forecast[y].temp_min, tempUnit,
            OWOC.forecast[y].temp_max, tempUnit, OWOC.forecast[y].pressure, OWOC.forecast[y].humidity,
            OWOC.forecast[y].pop * 100, OWOC.forecast[y].rain, OWOC.forecast[y].snow, OWOC.forecast[y].icon);
        Serial.println(Output);
    }
    Serial.println("");
}

void setup()
{
    // Begin serial and display
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    // Init Inkplate library (you should call this function ONLY ONCE)
    display.begin();

    // Try connecting to a WiFi network.
    // Parameters are network SSID, password, timeout in seconds and whether to print to serial.
    // If the Inkplate isn't able to connect to a network stop further code execution and print an error message.
    if (!display.connectWiFi(SSID, PASS, timeoutSeconds, true))
    {
        //Can't connect to netowrk
        // Clear display for the error message
        display.clearDisplay();
        // Set the font size;
        display.setTextSize(3);
        // Set the cursor positions and print the text.
        display.setCursor((display.width() / 2) - 200, display.height() / 2);
        display.print(F("Unable to connect to "));
        display.println(F(SSID));
        display.setCursor((display.width() / 2) - 200, (display.height() / 2) + 30);
        display.println(F("Please check SSID and PASS!"));
        // Display the error message on the Inkplate and go to deep sleep
        display.display();
        esp_sleep_enable_timer_wakeup(1000L * DELAY_WIFI_RETRY_SECONDS);
        (void)esp_deep_sleep_start();
    }

    // Check if we have a valid API key:
    Serial.println("Checking if API key is valid...");
    if(!checkIfAPIKeyIsValid(APIKEY))
    {
        // If we don't, notify the user
        Serial.println("API key is invalid!");
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(2);
        display.println("Can't get data from OpenWeatherMaps! Check your API key!");
        display.println("Only older API keys for OneCall 2.5 work in free tier.");
        display.println("See the code comments the example for more info.");
        // Display the error message on the Inkplate and go to deep sleep
        display.display();
        esp_sleep_enable_timer_wakeup(1000L * DELAY_API_RETRY_SECONDS);
        (void)esp_deep_sleep_start();
    }
    Serial.println("API key is valid!");

    // Set the temp. unit
    tempUnit = (metric == 1 ? 'C' : 'F');
}

void loop()
{
    // Clear display
    t = now();

    if ((minute(t) % 30) == 0) // Also returns 0 when time isn't set
    {
        GetCurrentWeather();
        Serial.println("got weather data");
        display.clearDisplay();
        drawCurrent();
        Serial.println("draw current");
        drawForecast();
        Serial.println("draw forecast");
        drawHourly();
        Serial.println("draw hourly");
        drawTime();
        Serial.println("draw time");
        drawMoon();
        Serial.println("draw moon");
        display.display();
    }

    // Go to sleep before checking again
    esp_sleep_enable_timer_wakeup(1000L * DELAY_MS);
    (void)esp_light_sleep_start();
}

// Function for drawing weather info aligned
void alignText(const char align, const char *text, int16_t x, int16_t y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    switch (align)
    {
    case CENTRE_BOT:
    case CENTRE:
    case CENTRE_TOP:
        x1 = x - w / 2;
        break;
    case RIGHT_BOT:
    case RIGHT:
    case RIGHT_TOP:
        x1 = x - w;
        break;
    case LEFT_BOT:
    case LEFT:
    case LEFT_TOP:
    default:
        x1 = x;
        break;
    }
    switch (align)
    {
    case CENTRE_BOT:
    case RIGHT_BOT:
    case LEFT_BOT:
        y1 = y;
        break;
    case CENTRE:
    case RIGHT:
    case LEFT:
        y1 = y + h / 2;
        break;
    case CENTRE_TOP:
    case LEFT_TOP:
    case RIGHT_TOP:
    default:
        y1 = y + h;
        break;
    }
    display.setCursor(x1, y1);
    display.print(text);
}

// Function for drawing weather info
void drawForecast()
{
    int dayPos;
    int xOffset = 10;
    int startDay = 1;
    int numOfDays = (sizeof(OWOC.forecast) / sizeof(OWOC.forecast[0]));
    int dayPitch = E_INK_WIDTH / (4 - startDay);
    for (int day = startDay; day < 4; day++)
    {
        dayPos = (day - startDay) * dayPitch;
        int textCentre = dayPos + (dayPitch / 2);
        sprintf(Output, "http://openweathermap.org/img/wn/%s@2x.png", OWOC.forecast[day].icon);
        Serial.println(Output);
        display.drawImage(Output, textCentre - 48, dayOffset - 62, true, true);
        display.setTextColor(INKPLATE4_BLACK, INKPLATE4_WHITE);
        display.setTextSize(1);
        display.setFont(&FreeSans9pt7b);
        sprintf(Output, "%s", dayShortStr(weekday(OWOC.forecast[day].dt)));
        alignText(CENTRE_BOT, Output, textCentre, dayOffset + 28);

        display.setFont();
        sprintf(Output, "%2.0f%c/%.0f%c", OWOC.forecast[day].temp_min, tempUnit, OWOC.forecast[day].temp_max, tempUnit);
        alignText(CENTRE_BOT, Output, textCentre, dayOffset + 38);

        sprintf(Output, "%4.0fhPa", OWOC.forecast[day].pressure);
        alignText(CENTRE_BOT, Output, textCentre, dayOffset + 48);

        sprintf(Output, "%3.0f%% Rh", OWOC.forecast[day].humidity);
        alignText(CENTRE_BOT, Output, textCentre, dayOffset + 58);
    }
}

// Function for drawing current date and time
void drawTime()
{
    int textWidth;
    char Output2[200] = {0};

    t = now();

    // Drawing current time
    display.setTextColor(INKPLATE4_BLACK, INKPLATE4_WHITE);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextSize(1);

    // Drawing current date
    sprintf(Output, "%s %02d ", dayShortStr(weekday(t)) + 0, day());
    sprintf(Output2, "%s %04d", monthShortStr(month(t)) + 0, year());
    strcat(Output, Output2);
    display.setCursor(3, 16);
    display.print(Output);

    sprintf(Output, "%02d:%02d", hour(t), minute(t));
    display.setCursor(355, 18);
    display.print(Output);
}

// Draw graph which shows hourly weather
void drawHourly()
{
    const int yTop = 185;
    const int yHeight = 105;
    const int xLeft = 18;
    const int xWidth = 360;
    const int hoursDisplay = 8;
    const int hourPitch = xWidth / hoursDisplay;
    const float minPrec = 0;
    float maxPrec = 1;
    float maxTemp = -100;
    float minTemp = 100;

    display.setFont(&Picopixel);

    for (int Houry = 0; Houry < hoursDisplay; Houry++)
    {
        float thisPrec = OWOC.hourly[Houry].rain_1h + OWOC.hourly[Houry].snow_1h;
        if (maxPrec < thisPrec)
            maxPrec = thisPrec;
        if (maxTemp < OWOC.hourly[Houry].temp)
            maxTemp = OWOC.hourly[Houry].temp;
        if (minTemp > OWOC.hourly[Houry].temp)
            minTemp = OWOC.hourly[Houry].temp;
    }

    for (int Houry = 0; Houry <= hoursDisplay; Houry++)
    {
        display.drawLine(xLeft + (Houry * hourPitch), yTop, xLeft + (Houry * hourPitch), yTop + yHeight,
                         INKPLATE4_BLACK);
        if (Houry % 2)
        {
            sprintf(Output, "%2d", hour(OWOC.hourly[Houry].dt));
            alignText(CENTRE_TOP, Output, xLeft + (Houry * hourPitch), yTop + yHeight + 2);
        }
    }

    display.drawLine(xLeft, yTop + yHeight, xLeft + xWidth, yTop + yHeight, INKPLATE4_BLACK);
    display.drawLine(xLeft, yTop, xLeft + xWidth, yTop, INKPLATE4_BLACK);

    sprintf(Output, "%2.0f%c", (minTemp - 0.499), tempUnit);
    alignText(RIGHT, Output, xLeft - 5, yTop + yHeight);
    sprintf(Output, "%2.0f%c", (maxTemp + .5), tempUnit);
    alignText(RIGHT, Output, xLeft - 5, yTop);

    sprintf(Output, "%2.0f mm", minPrec);
    alignText(LEFT, Output, xLeft + xWidth + 2, yTop + yHeight);
    sprintf(Output, "%2.0f mm", (maxPrec + .499));
    alignText(LEFT, Output, xLeft + xWidth + 2, yTop);

    float yTempScale = (yHeight / (round(maxTemp + 0.499) - round(minTemp - 0.5)));

    for (int Houry = 0; Houry <= (round(maxTemp + 0.499) - round(minTemp - 0.5)); Houry++)
    {
        display.drawLine(xLeft, yTop + (Houry * yTempScale), xLeft + xWidth, yTop + (Houry * yTempScale),
                         INKPLATE4_BLACK);
    }

    for (int Houry = 0; Houry <= (hoursDisplay - 1); Houry++)
    {
        display.drawThickLine(xLeft + (Houry * hourPitch),
                              yTop + (int)(yTempScale * (round(maxTemp + 0.499) - (OWOC.hourly[Houry].temp))),
                              xLeft + ((Houry + 1) * hourPitch),
                              yTop + (int)(yTempScale * (round(maxTemp + 0.499) - (OWOC.hourly[Houry + 1].temp))),
                              INKPLATE4_BLACK, 3);
        float yPrecScale = (yHeight / (round(maxPrec + 0.499)));
        float thisPrec = OWOC.hourly[Houry].rain_1h + OWOC.hourly[Houry].snow_1h;
        display.fillRect(xLeft + (Houry * hourPitch) + round(hourPitch / 3),
                         yTop + (int)(yPrecScale * (round(maxPrec + .499) - thisPrec)), round(hourPitch / 3),
                         (int)(yPrecScale * thisPrec), INKPLATE4_RED);
    }
}

// Current weather drawing function
void drawCurrent()
{
    sprintf(Output, "http://openweathermap.org/img/wn/%s.png", OWOC.current.icon);
    Serial.println("CURRENT ");
    Serial.println(Output);
    display.drawImage(Output, -5, 18, true, true);

    display.setFont(&FreeSans12pt7b);
    sprintf(Output, "%2.01f%c", OWOC.current.temp, tempUnit);
    display.setCursor(42, 50);
    display.print(Output);

    display.setFont(&FreeSans9pt7b);
    sprintf(Output, "%2.0f%c/%.0f%c", OWOC.forecast[0].temp_min, tempUnit, OWOC.forecast[0].temp_max, tempUnit);
    display.setCursor(110, 45);
    display.print(Output);

    sprintf(Output, "%4.0fhPa", OWOC.current.pressure);
    display.setCursor(185, 45);
    display.print(Output);

    sprintf(Output, "%3.0f%%", OWOC.current.humidity);
    display.setCursor(272, 45);
    display.print(Output);

    // Set font to default and print labels
    display.setFont();
    display.setCursor(108, 55);
    display.print("Temps. Today");

    display.setCursor(195, 55);
    display.print("Pressure");

    display.setCursor(272, 55);
    display.print("Humidity");
}

float getMoonPhase(time_t tdate)
{
    time_t newmoonref = 1263539460; // known new moon date (2010-01-15 07:11)
    // moon phase is 29.5305882 days, which is 2551442.82048 seconds
    float phase = abs(tdate - newmoonref) / (double)2551442.82048;
    phase -= (int)phase; // leave only the remainder
    if (newmoonref > tdate)
        phase = 1 - phase;

    /*
    return value is percent of moon cycle ( from 0.0 to 0.999999), i.e.:

    0.0: New Moon
    0.125: Waxing Crescent Moon
    0.25: Quarter Moon
    0.375: Waxing Gibbous Moon
    0.5: Full Moon
    0.625: Waning Gibbous Moon
    0.75: Last Quarter Moon
    0.875: Waning Crescent Moon
    */
    return phase;
}

// Function for drawing the moon
void drawMoon()
{
    const int MoonCentreX = 15;
    const int MoonCentreY = 15;
    const int MoonBox = 2;
    float moonphase = getMoonPhase(now());
    int moonage = 29.5305882 * moonphase;
    // Convert to appropriate icon
    display.setFont(&moon_phases12pt7b);
    sprintf(Output, "%c", (char)((int)'A' + (int)(moonage * 25. / 30)));
    display.setCursor(353, 52);
    display.print(Output);

    // Set default font and print Moon Phase label
    display.setFont();
    display.setCursor(339, 55);
    display.print("Moon Phase");
}

/**
 * Make a test API call to see if we have a valid API key
 *
 * Older keys made for OpenWeather 2.5 OneCall API work, while newer ones won't work, due to the service becoming
 * deprecated.
 */
bool checkIfAPIKeyIsValid(char *APIKEY)
{
    bool failed = false;

    // Create the buffer for holding the API response, make it large enough just in case
    char * data;
    data = (char *)ps_malloc(50000LL);

    // Http object used to make GET request
    HTTPClient http;
    http.getStream().setTimeout(10);
    http.getStream().flush();
    http.getStream().setNoDelay(true);

    // Combine the base URL and the API key to do a test call
    char * baseURL = "https://api.openweathermap.org/data/2.5/onecall?lat=45.560001&lon=18.675880&units=metric&appid=";
    char apiTestURL[200];
    sprintf(apiTestURL, "%s%s", baseURL, APIKEY);

    // Begin http by passing url to it
    http.begin(apiTestURL);

    delay(300);
    
    // Download data until it's a verified complete download
    // Actually do request
    int httpCode = http.GET();

    if (httpCode == 200)
    {
        long n = 0;

        long now = millis();

        while (millis() - now < 1000)
        {
            while (http.getStream().available())
            {
                data[n++] = http.getStream().read();
                now = millis();
            }
        }

        data[n++] = 0;

        // If the reply constains this string - it's invalid
        if(strstr(data, "Invalid API key.") != NULL) failed = true;
    }
    else
    {
        // In case there was another HTTP code, it failed
        Serial.print("Error! HTTP Code: ");
        Serial.println(httpCode);
        failed = true;
    }

    // End http
    http.end();

    // Free the memory
    free(data);

    return !failed;
}
