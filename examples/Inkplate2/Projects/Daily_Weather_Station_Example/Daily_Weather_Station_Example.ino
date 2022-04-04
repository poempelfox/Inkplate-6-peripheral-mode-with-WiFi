/*
   Weather station example for e-radionica.com Inkplate 2
   For this example you will need only USB cable and Inkplate 2.
   Select "Inkplate 2(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 2(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   This example will show you how you can use Inkplate 2 to display API data,
   e.g. Metaweather public weather API

   IMPORTANT:
   Make sure to change your desired city, timezone and wifi credentials below
   Also have ArduinoJSON installed in your Arduino libraries

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   30 March 2020 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE2
#error "Wrong board selection for this example, please select Inkplate 5 in the boards menu."
#endif

// ---------- CHANGE HERE  -------------:

// Time zone for adding hours
int timeZone = 2;

// City search query
char city[128] = "ZAGREB";

// Change to your wifi ssid and password
char ssid[] = "";
char pass[] = "";

// ----------------------------------

// Include Inkplate library to the sketch
#include "Inkplate.h"

// Header file for easier code readability
#include "Network.h"

// Including fonts used
#include "Fonts/Roboto_Light8.h"

// Including icons generated by the py file
#include "icons.h"

// Delay between API calls
#define DELAY_MS 60000 * 3

// Inkplate object
Inkplate display;

// All our network functions are in this object, see Network.h
Network network;

// Contants used for drawing icons
char abbrs[32][16] = {"sn", "sl", "h", "t", "hr", "lr", "s", "hc", "lc", "c"};
const uint8_t *s_logos[16] = {icon_s_sn, icon_s_sl, icon_s_h,  icon_s_t,  icon_s_hr,
                              icon_s_lr, icon_s_s,  icon_s_hc, icon_s_lc, icon_s_c
                             };

RTC_DATA_ATTR char abbr1[16];
RTC_DATA_ATTR char abbr2[16];
RTC_DATA_ATTR char abbr3[16];
RTC_DATA_ATTR char abbr4[16];

// Variables for storing temperature
RTC_DATA_ATTR char temps[8][4] = {
  "0F",
  "0F",
  "0F",
  "0F",
};

// Variables for storing days of the week
RTC_DATA_ATTR char days[8][4] = {
  "",
  "",
  "",
  "",
};

// Variable for counting partial refreshes
RTC_DATA_ATTR unsigned refreshes = 0;

// Constant to determine when to full update
const int fullRefresh = 5;

// Variables for storing current time and weather info
RTC_DATA_ATTR char currentTemp[16] = "0F";
RTC_DATA_ATTR char currentWind[16] = "0m/s";

RTC_DATA_ATTR char currentTime[16] = "9:41";

RTC_DATA_ATTR char currentWeather[32] = "-";
RTC_DATA_ATTR char currentWeatherAbbr[8] = "th";

// function defined below
void drawTemps();

void setup()
{
  // Begin serial and display
  Serial.begin(115200);
  display.begin();


  // Welcome screen
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.drawTextWithShadow(0, 20, "Welcome to Inkpl-", RED, BLACK);
  display.drawTextWithShadow(0, 40, "ate Daily Weather", RED, BLACK);
  display.drawTextWithShadow(0, 60, "Station example!", RED, BLACK);
  display.display();

  display.clearDisplay();
  // Wait a bit before proceeding
  delay(5000);

  // Calling our begin from network.h file
  network.begin(city);

  // If city not found, do nothing
  if (network.location == -1)
  {
    display.setCursor(50, 290);
    display.setTextSize(3);
    display.print(F("City not in Metaweather Database"));
    display.display();
    while (1)
      ;


    // Get all relevant data, see Network.cpp for info
    network.getTime(currentTime);
    network.getTime(currentTime);
    network.getDays(days[0], days[1], days[2], days[3]);
    network.getData(city, temps[0], temps[1], temps[2], temps[3], currentTemp, currentWind, currentTime,
                    currentWeather, currentWeatherAbbr, abbr1, abbr2, abbr3, abbr4);

    // Draw data, see functions below for info
    drawTemps();

    display.display();
  }
  else
  {
    // Refresh only the clock
    network.getTime(currentTime);

    display.clearDisplay();
    drawTemps(); //Call function

    display.display();
  }

  // Go to sleep
  esp_sleep_enable_timer_wakeup(1000L * DELAY_MS);
  (void)esp_deep_sleep_start();
}

void loop()
{
  // Never here
}


// Function for drawing temperatures
void drawTemps()
{
  // Drawing 4 black rectangles in which temperatures will be written

  display.fillRect(5, 5, 60, 94, BLACK); //Draw some rectangles
  display.fillRect(75, 5, 60, 94, BLACK);
  display.fillRect(145, 5, 60, 94, BLACK);

  display.setFont(&Roboto_Light8); //Set custom font; it needs to be included as .h file
  display.setTextSize(1); //Set font size
  display.setTextColor(RED, BLACK); // Set text and background color

  display.setCursor(10, 20);  // Set cursor, custom font uses different method for setting cursor
                              // You can find more about that here https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
  display.println("Today");

  display.setCursor(80, 20);
  display.println(days[1]);

  display.setCursor(150, 20);
  display.println(days[2]);

  // Drawing temperature values into black rectangles
  display.setFont(&Roboto_Light8);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);

  display.setCursor(15, 40);
  display.print(temps[0]);
  display.println(F("C"));

  display.setCursor(85, 40);
  display.print(temps[1]);
  display.println(F("C"));

  display.setCursor(155, 40);
  display.print(temps[2]);
  display.println(F("C"));


  for (int i = 0; i < 18; ++i)
  {
    // If found draw specified icon
    if (strcmp(abbr1, abbrs[i]) == 0)
      display.drawBitmap(11, 48, s_logos[i], 48, 48,
                         WHITE, BLACK);
  }

  for (int i = 0; i < 18; ++i)
  {
    // If found draw specified icon
    if (strcmp(abbr2, abbrs[i]) == 0)
      display.drawBitmap(81, 48, s_logos[i], 48, 48,
                         WHITE, BLACK);
  }

  for (int i = 0; i < 18; ++i)
  {
    // If found draw specified icon
    if (strcmp(abbr3, abbrs[i]) == 0)
      display.drawBitmap(151, 48, s_logos[i], 48, 48,
                         WHITE, BLACK);
  }

}
