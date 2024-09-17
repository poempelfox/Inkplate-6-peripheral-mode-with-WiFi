/*
   Inkplate5_Hourly_Weather_Station example for Soldered Inkplate 5
   For this example you will need only USB cable and Inkplate 5.
   Select "Soldered Inkplate5" from Tools -> Board menu.
   Don't have "Soldered Inkplate5" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   This example will show you how you can use Inkplate 5 to display API data,
   e.g. OpenWeather public weather API for real time data. It shows the forecast
   weather for 4 hours. What happens here is basically ESP32 connects to WiFi and 
   sends an API call and the server returns data in JSON format containing data 
   about weather, then using the library ArduinoJson we extract only temperature 
   per hour from JSON data and show it on Inkplate 5. After displaying the weather,
   ESP32 goes to sleep and wakes up every DELAY_MS milliseconds to show new weather
   (you can change the time interval).

   IMPORTANT:
   Make sure to change your desired city and wifi credentials below.
   Also have ArduinoJson installed in your Arduino libraries:
   https://github.com/bblanchon/ArduinoJson

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   27 March 2023 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE5
#error "Wrong board selection for this example, please select Soldered Inkplate5 in the boards menu."
#endif

//---------- CHANGE HERE  -------------:

// City name to be displayed on the bottom
char city[128] = "OSIJEK";

// Coordinates sent to the api
char lon[] = "18.5947808";
char lat[] = "45.5510548";

// Change to your wifi ssid and password
char ssid[] = "";
char pass[] = "";

// Change to your api key, if you don't have one, head over to:
// https://openweathermap.org/guide , register and copy the key provided
char apiKey[] = "";

// Uncomment this for MPH and Fahrenheit output, also uncomment it in the begining of Network.cpp
// #define AMERICAN

//----------------------------------

// Include Inkplate library to the sketch
#include "Inkplate.h"

// Header file for easier code readability
#include "Network.h"

// Including fonts used
#include "Fonts/Roboto_Light_120.h"
#include "Fonts/Roboto_Light_36.h"
#include "Fonts/Roboto_Light_48.h"

// Including icons generated by the py file
#include "icons.h"

// Delay between API calls, about 1000 per month, which is the free tier limit
#define DELAY_MS 267800L
#define DELAY_WIFI_RETRY_SECONDS 5

// Inkplate object
Inkplate display(INKPLATE_1BIT);

// All our network functions are in this object, see Network.h
Network network;

// Constants used for drawing icons
char abbrs[32][32] = {"01d", "02d", "03d", "04d", "09d", "10d", "11d", "13d", "50d",
                      "01n", "02n", "03n", "04n", "09n", "10n", "11n", "13n", "50n"};

const uint8_t *logos[18] = {
    icon_01d, icon_02d, icon_03d, icon_04d, icon_09d, icon_10d, icon_11d, icon_13d, icon_50d,
    icon_01n, icon_02n, icon_03n, icon_04n, icon_09n, icon_10n, icon_11n, icon_13n, icon_50n,
};

const uint8_t *s_logos[18] = {
    icon_s_01d, icon_s_02d, icon_s_03d, icon_s_04d, icon_s_09d, icon_s_10d, icon_s_11d, icon_s_13d, icon_s_50d,
    icon_s_01n, icon_s_02n, icon_s_03n, icon_s_04n, icon_s_09n, icon_s_10n, icon_s_11n, icon_s_13n, icon_s_50n,
};

// Variables for storing temperature
RTC_DATA_ATTR char temps[4][8] = {
    "-",
    "-",
    "-",
    "-",
};

// Variables for storing hour strings
RTC_DATA_ATTR char hours[4][8] = {
    "",
    "",
    "",
    "",
};

// Variable for counting partial refreshes
RTC_DATA_ATTR unsigned refreshes = 0;

// Constant to determine when to full update
const int fullRefresh = 10;

// Variables for storing current time and weather info
RTC_DATA_ATTR char currentTemp[16] = "-";
RTC_DATA_ATTR char currentWind[16] = "-";

RTC_DATA_ATTR char currentTime[16] = "--:--";

RTC_DATA_ATTR int timeZone;

RTC_DATA_ATTR char currentWeather[32] = "-";
RTC_DATA_ATTR char currentWeatherAbbr[8] = "01d";

RTC_DATA_ATTR char abbr1[16];
RTC_DATA_ATTR char abbr2[16];
RTC_DATA_ATTR char abbr3[16];
RTC_DATA_ATTR char abbr4[16];

// functions defined below
void drawWeather();
void drawCurrent();
void drawTemps();
void drawCity();
void drawTime();

void setup()
{
    // Begin serial and display
    Serial.begin(115200);
    display.begin();

    // Connect Inkplate to the WiFi network
    // Try connecting to a WiFi network.
    // Parameters are network SSID, password, timeout in seconds and whether to print to serial.
    // If the Inkplate isn't able to connect to a network stop further code execution and print an error message.
    if (!display.connectWiFi(ssid, pass, WIFI_TIMEOUT, true))
    {
        //Can't connect to netowrk
        // Clear display for the error message
        display.clearDisplay();
        // Set the font size;
        display.setTextSize(3);
        // Set the cursor positions and print the text.
        display.setCursor((display.width() / 2) - 200, display.height() / 2);
        display.print(F("Unable to connect to "));
        display.println(F(ssid));
        display.setCursor((display.width() / 2) - 200, (display.height() / 2) + 30);
        display.println(F("Please check SSID and PASS!"));
        // Display the error message on the Inkplate and go to deep sleep
        display.display();
        esp_sleep_enable_timer_wakeup(1000L * DELAY_WIFI_RETRY_SECONDS);
        (void)esp_deep_sleep_start();
    }

    // After connecting to WiFi we need to get internet time from NTP server
    time_t nowSec;
    struct tm timeInfo;
    // Fetch current time in epoch format and store it
    display.getNTPEpoch(&nowSec);
    gmtime_r(&nowSec, &timeInfo);
    Serial.print(F("Current time: "));
    Serial.print(asctime(&timeInfo));

    // Get all relevant data, see Network.cpp for info
    if (refreshes % fullRefresh == 0)
    {
        Serial.print("Retrying fetching data");
        while (!network.getData(lat, lon, apiKey, temps[0], temps[1], temps[2], temps[3], currentTemp, currentWind,
                                currentTime, currentWeather, currentWeatherAbbr, abbr1, abbr2, abbr3, abbr4, &timeZone))
        {
            Serial.print('.');
            delay(500);
        }
    }

    network.getTime(currentTime, timeZone);
    network.getHours(hours[0], hours[1], hours[2], hours[3], timeZone);

    // Draw data, see functions below for info
    drawWeather();
    drawCurrent();
    drawTemps();
    drawCity();
    drawTime();

    // Refresh full screen every fullRefresh times, defined above
    if (refreshes % fullRefresh == 0)
        display.display();
    else
        display.partialUpdate();

    ++refreshes;

    // Go to sleep before checking again
    esp_sleep_enable_timer_wakeup(1000L * DELAY_MS); // Activate wake-up timer
    (void)esp_deep_sleep_start(); // Start deep sleep (this function does not return). Program stops here.
}

void loop()
{
    // Never here! If you are using deep sleep, the whole program should be in setup() because the board restarts each
    // time. loop() must be empty!
}

// Function for drawing weather info
void drawWeather()
{
    // Searching for weather state abbreviation
    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbrs[i], currentWeatherAbbr) == 0)
            display.drawBitmap(70, 50, logos[i], 152, 152, BLACK);
    }

    // Draw weather state
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);
    display.setCursor(70, 210);
    display.println(currentWeather);
}

// Function for drawing current time
void drawTime()
{
    // Drawing current time
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(display.width() - 20 * strlen(currentTime), 35);
    display.println(currentTime);
}

// Function for drawing city name
void drawCity()
{
    // Drawing city name
    display.setTextColor(BLACK, WHITE);
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(display.width() / 2 - 9 * strlen(city), display.height() - 20);
    display.println(city);
}

// Draw celsius degrees if AMERICAN isn't defined or Fahrenheit degrees if it's defined
void drawTempUnit()
{
#ifdef AMERICAN
    display.println(F("F"));
#else
    display.println(F("C"));
#endif
}

// Function for drawing temperatures
void drawTemps()
{
    // Drawing 4 black rectangles in which temperatures will be written
    int rectWidth = 150;
    int rectSpacing = (display.width() - rectWidth * 4) / 5;

    display.fillRect(1 * rectSpacing + 0 * rectWidth, 250, rectWidth, 220, BLACK);
    display.fillRect(2 * rectSpacing + 1 * rectWidth, 250, rectWidth, 220, BLACK);
    display.fillRect(3 * rectSpacing + 2 * rectWidth, 250, rectWidth, 220, BLACK);
    display.fillRect(4 * rectSpacing + 3 * rectWidth, 250, rectWidth, 220, BLACK);

    int textMargin = 6;

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);

    display.setCursor(1 * rectSpacing + 0 * rectWidth + textMargin, 250 + textMargin + 40);
    display.println(hours[0]);

    display.setCursor(2 * rectSpacing + 1 * rectWidth + textMargin, 250 + textMargin + 40);
    display.println(hours[1]);

    display.setCursor(3 * rectSpacing + 2 * rectWidth + textMargin, 250 + textMargin + 40);
    display.println(hours[2]);

    display.setCursor(4 * rectSpacing + 3 * rectWidth + textMargin, 250 + textMargin + 40);
    display.println(hours[3]);

    // Drawing temperature values into black rectangles
    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);
    display.setTextColor(WHITE, BLACK);

    display.setCursor(1 * rectSpacing + 0 * rectWidth + textMargin, 250 + textMargin + 120);
    display.print(temps[0]);
    drawTempUnit();

    display.setCursor(2 * rectSpacing + 1 * rectWidth + textMargin, 250 + textMargin + 120);
    display.print(temps[1]);
    drawTempUnit();

    display.setCursor(3 * rectSpacing + 2 * rectWidth + textMargin, 250 + textMargin + 120);
    display.print(temps[2]);
    drawTempUnit();

    display.setCursor(4 * rectSpacing + 3 * rectWidth + textMargin, 250 + textMargin + 120);
    display.print(temps[3]);
    drawTempUnit();

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr1, abbrs[i]) == 0)
            display.drawBitmap(1 * rectSpacing + 0 * rectWidth + textMargin, 250 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr2, abbrs[i]) == 0)
            display.drawBitmap(2 * rectSpacing + 1 * rectWidth + textMargin, 250 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr3, abbrs[i]) == 0)
            display.drawBitmap(3 * rectSpacing + 2 * rectWidth + textMargin, 250 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }

    for (int i = 0; i < 18; ++i)
    {
        // If found draw specified icon
        if (strcmp(abbr4, abbrs[i]) == 0)
            display.drawBitmap(4 * rectSpacing + 3 * rectWidth + textMargin, 250 + textMargin + 150, s_logos[i], 48, 48,
                               WHITE, BLACK);
    }
}

// Current weather drawing function
void drawCurrent()
{
    // Drawing current information

    // Temperature:
    display.setFont(&Roboto_Light_120);
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);

    display.setCursor(325, 150);
    display.print(currentTemp);

    int x = display.getCursorX();
    int y = display.getCursorY();

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);

    display.setCursor(x, y);
    drawTempUnit();

    // Wind:
    display.setFont(&Roboto_Light_120);
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);

    display.setCursor(610, 150);
    display.print(currentWind);

    x = display.getCursorX();
    y = display.getCursorY();

    display.setFont(&Roboto_Light_48);
    display.setTextSize(1);

    display.setCursor(x, y);

#ifdef AMERICAN
    display.println(F("mph"));
#else
    display.println(F("m/s"));
#endif

    // Labels underneath
    display.setFont(&Roboto_Light_36);
    display.setTextSize(1);

    display.setCursor(285, 210);
    display.println(F("TEMPERATURE"));

    display.setCursor(630, 210);
    display.println(F("WIND SPEED"));
}
