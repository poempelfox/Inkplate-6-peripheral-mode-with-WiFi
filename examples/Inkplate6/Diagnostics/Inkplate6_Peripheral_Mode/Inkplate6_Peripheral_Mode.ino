/*
   Note: This is a heavily adapted version of this sketch, with features like different fonts and
   WiFi support added. The comments will not always reflect this. For example, what follows is
   the original header text that has no mention of WiFi support:

   Inkplate6_Peripheral_Mode sketch for Soldered Inkplate 6
   Select "e-radionica Inkplate6" or "Soldered Inkplate6" from Tools -> Board menu.
   Don't have "e-radionica Inkplate6" or "Soldered Inkplate6" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   Using this sketch, you don't have to program and control e-paper using Arduino code.
   Instead, you can send UART command. This give you flexibility that you can use this Inkplate 6 on any platform!

   Because it uses UART, it's little bit slower and it's not recommended to send bunch of
   drawPixel command to draw some image. Instead, load bitmaps and pictures on SD card and load image from SD.
   If we missed some function, you can modify this and make yor own.
   Also, every Inkplate comes with this peripheral mode right from the factory.

   Learn more about Peripheral Mode:
   https://inkplate.readthedocs.io/en/latest/peripheral-mode.html

   UART settings are: 115200 baud, standard parity, ending with "\n\r" (both)
   You can send commands via USB port or by directly connecting to ESP32 TX and RX pins.
   Don't forget you need to send #L(1)* after each command to show it on the display
   (equal to display.display()).

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   15 July 2020 by Soldered
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#if !defined(ARDUINO_ESP32_DEV) && !defined(ARDUINO_INKPLATE6V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate6 or Soldered Inkplate6 in the boards menu."
#endif

#include <Inkplate.h>
#include <WiFi.h>
#include "FreeSans12pt.h"
#include "FreeSans24pt.h"
#include "DSEG722pt.h"
#include "DSEG733pt.h"
#include "DSEG744pt.h"
#include "Terminus14pt.h"
#include "Terminus18pt.h"
#include "TerminusB24pt.h"
#include "TerminusB44pt.h"
#include "TerminusB56pt.h"
/* define WIFI_SSID and WIFI_PASS in secrets.h! */
#include "secrets.h"

Inkplate display(INKPLATE_1BIT);
WiFiServer * tcpserver;

#define BUFFER_SIZE 4000
char commandBuffer[BUFFER_SIZE + 2];
char strTemp[BUFFER_SIZE * 2 + 1];

int hexToChar(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

void setup()
{
    Serial.begin(115200);
    display.begin();
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("Inkpl6Col-123456");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int nrattempts = 0;
    Serial.print("Connecting to Wifi...");
    while ((nrattempts < 300) && (WiFi.status() != WL_CONNECTED)) {
      if ((nrattempts % 10) == 9) {
        Serial.print(".");
      }
      delay(100);
      nrattempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Connected after %.1f seconds\n", nrattempts / 10.0);
      IPAddress ip = WiFi.localIP();
      Serial.print("IP Address: ");
      Serial.println(ip);
    } else {
      Serial.print("WiFi connect timed out. Will keep trying in the background.\n");
    }
    tcpserver = new WiFiServer(23);
    tcpserver->begin();
    display.fillRect(0, 0, E_INK_WIDTH, E_INK_HEIGHT, 0);
    display.setTextWrap(false);
    display.setTextColor(1);
    display.setFont(&FreeSans12pt7b);
    display.setCursor(10, 20);
    display.print("Peripheral Mode with WiFi started");
    display.setCursor(10, 40);
    if (WiFi.status() == WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      display.print(ip);
    } else {
      display.print("got no IP before timeout :(");
    }
    display.display();
#if 0 /* some demo output */
    unsigned char spc[] = { 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x00 };
    display.setFont(&FreeSans12pt7b);
    display.setCursor(10, 20);
    display.print("FreeSans12");
    display.print((const char *)spc);
    display.setTextColor(6);
    display.setFont(&FreeSans24pt7b);
    display.setCursor(10, 60);
    display.print("FreeSans24");
    display.print((const char *)spc);
    display.setTextColor(2);
    // Terminus pairs very well with slightly smaller DSEG7
    display.setFont(&TerminusTTF_4_46_014pt7b);
    display.setCursor(10, 180);
    display.print("Terminus14 - 0123456789 - ");
    display.print((const char *)spc);
    display.setFont(&DSEG7Classic_Regular_mod722pt7b);
    display.print(" 0123456789.");
    // Terminus18 pairs very well with DSEG7-15
    display.setTextColor(3);
    display.setFont(&TerminusTTF_4_46_018pt7b);
    display.setCursor(10, 240);
    display.print("Terminus18 - 0123456789 - ");
    display.print((const char *)spc);
    display.setFont(&DSEG7Classic_Regular_mod733pt7b);
    display.print(" 0123456789.");
    display.setTextColor(4);
    display.setFont(&TerminusTTF_Bold_4_49_324pt7b);
    display.setCursor(10, 300);
    display.print("Terminus24Bold - 0123456789 - ");
    display.print((const char *)spc);
    display.setTextColor(5);
    display.setFont(&TerminusTTF_Bold_4_49_344pt7b);
    display.setCursor(10, 360);
    display.print("Terminus44B ");
    display.print((const char *)spc);
    display.setFont(&DSEG7Classic_Regular_mod733pt7b);
    display.print(" 0123456789.");
    display.display();
#endif
    memset(commandBuffer, 0, BUFFER_SIZE+1);
}

void loop()
{
    // put your main code here, to run repeatedly:
    static int readsource = 0;
    static int bytesinbuf = 0;
    static WiFiClient client;
    // We must not mix input from serial and network.
    // Thus, once we started to fill our buffer from one source,
    // we'll have to ignore the other until the buffer contents
    // have been handled.
    if ((bytesinbuf == 0) || (readsource == 1)) {
      while (Serial.available()) {
        readsource = 1; // We're reading from serial.
        if (bytesinbuf < BUFFER_SIZE) {
          commandBuffer[bytesinbuf] = Serial.read();
          bytesinbuf++;
        } else { // No space - abort read loop
          break;
        }
      }
    }
    // Only try to read from network if there was nothing on the
    // serial port, i.e. serial has priority.
    if ((bytesinbuf == 0) || (readsource == 2)) {
      client = tcpserver->available();
      if (client) {
        client.setTimeout(200); // This might time out reading after 200 ms?
        // Arduino documentation is targeted at idiots and missing quite vital information that advanced users would need, and in the few pieces where such info exists, it's obviously wrong. Everybody copies off examples instead. So do we.
        int c;
        while ((c = client.read()) != -1) {
          readsource = 2; // We're reading from network.
          if (bytesinbuf < BUFFER_SIZE) {
            commandBuffer[bytesinbuf] = c;
            bytesinbuf++;
          } else { // No space - abort read loop
            Serial.print("!OVFL\r\n");
            break;
          }
        }
        if (bytesinbuf >= 3) {
          if ((commandBuffer[bytesinbuf - 1] == '*')
           && (commandBuffer[bytesinbuf - 2] == '?')
           && (commandBuffer[bytesinbuf - 3] == '#')) {
            client.println("OK");
          }
        }
      }
    }
    // Do we have a complete command in the buffer?
    char * e;
    do {
      char * s = &commandBuffer[0];
      e = NULL;
      for (int i = 0; i < BUFFER_SIZE; i++) {
          if (commandBuffer[i] == '*') {
              e = &commandBuffer[i];
              break;
          }
      }
      if (e != NULL) {
              int x, x1, x2, y, y1, y2, x3, y3, l, c, w, h, r, n, rx, ry, xc, yc, yr;
              uint8_t hr, min, sec, wday, day, mon, k, as, am, ah, ad, aw, amc, v;
              bool ie, ip;
              System::rtcCountdownSrcClock sc;
              uint32_t ep, ae;
              char b;
              char temp[150];
              sprintf(temp, ">%c\r\n", (*(s + 1) > 20) ? (*(s + 1)) : '?');
              if ((*(s + 1)) == '?') {
                Serial.print(temp);
              }
              switch (*(s + 1))
              {
              case '?':
                  if (readsource == 1) {
                    Serial.print("OK");
                  }
                  if (readsource == 2) {
                    // The following do not work, and I suspect it't because
                    // the arduino library is badly documented buggy SH*T.
                    // We have frickeled our way around this by just
                    // replying _directly_ after reading, as that seems to work.
                    // Serial.print("[OKtoNET]");
                    // tcpserver->println("OK");
                    // client.println("OK");
                    client.stop();
                  }
                  break;
  
              case '0':
                  sscanf(s + 3, "%d,%d,%d", &x, &y, &c);
                  // sprintf(temp, "display.drawPixel(%d, %d, %d)\n\r", x, y, c);
                  // Serial.print(temp);
                  display.drawPixel(x, y, c);
                  break;
  
              case '1':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &x1, &y1, &x2, &y2, &c);
                  // sprintf(temp, "display.drawLine(%d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, c);
                  // Serial.print(temp);
                  display.drawLine(x1, y1, x2, y2, c);
                  break;
  
              case '2':
                  sscanf(s + 3, "%d,%d,%d,%d", &x, &y, &l, &c);
                  // sprintf(temp, "display.drawFastVLine(%d, %d, %d, %d)\n\r", x, y, l, c);
                  // Serial.print(temp);
                  display.drawFastVLine(x, y, l, c);
                  break;
  
              case '3':
                  sscanf(s + 3, "%d,%d,%d,%d", &x, &y, &l, &c);
                  // sprintf(temp, "display.drawFastHLine(%d, %d, %d, %d)\n\r", x, y, l, c);
                  // Serial.print(temp);
                  display.drawFastHLine(x, y, l, c);
                  break;
  
              case '4':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &x, &y, &w, &h, &c);
                  // sprintf(temp, "display.drawRect(%d, %d, %d, %d, %d)\n\r", x, y, w, h, c);
                  // Serial.print(temp);
                  display.drawRect(x, y, w, h, c);
                  break;
  
              case '5':
                  sscanf(s + 3, "%d,%d,%d,%d", &x, &y, &r, &c);
                  // sprintf(temp, "display.drawCircle(%d, %d, %d, %d)\n\r", x, y, r, c);
                  // Serial.print(temp);
                  display.drawCircle(x, y, r, c);
                  break;
  
              case '6':
                  sscanf(s + 3, "%d,%d,%d,%d,%d,%d,%d", &x1, &y1, &x2, &y2, &x3, &y3, &c);
                  // sprintf(temp, "display.drawTriangle(%d, %d, %d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, x3, y3, c);
                  // Serial.print(temp);
                  display.drawTriangle(x1, y1, x2, y2, x3, y3, c);
                  break;
  
              case '7':
                  sscanf(s + 3, "%d,%d,%d,%d,%d,%d", &x, &y, &w, &h, &r, &c);
                  // sprintf(temp, "display.drawRoundRect(%d, %d, %d, %d, %d, %d)\n\r", x, y, w, h, r, c);
                  // Serial.print(temp);
                  display.drawRoundRect(x, y, w, h, r, c);
                  break;
  
              case '8':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &x, &y, &w, &h, &c);
                  // sprintf(temp, "display.fillRect(%d, %d, %d, %d, %d)\n\r", x, y, w, h, c);
                  // Serial.print(temp);
                  display.fillRect(x, y, w, h, c);
                  break;
  
              case '9':
                  sscanf(s + 3, "%d,%d,%d,%d", &x, &y, &r, &c);
                  // sprintf(temp, "display.fillCircle(%d, %d, %d, %d)\n\r", x, y, r, c);
                  // Serial.print(temp);
                  display.fillCircle(x, y, r, c);
                  break;
  
              case 'A':
                  sscanf(s + 3, "%d,%d,%d,%d,%d,%d,%d", &x1, &y1, &x2, &y2, &x3, &y3, &c);
                  // sprintf(temp, "display.fillTriangle(%d, %d, %d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, x3, y3, c);
                  // Serial.print(temp);
                  display.fillTriangle(x1, y1, x2, y2, x3, y3, c);
                  break;
  
              case 'B':
                  sscanf(s + 3, "%d,%d,%d,%d,%d,%d", &x, &y, &w, &h, &r, &c);
                  // sprintf(temp, "display.fillRoundRect(%d, %d, %d, %d, %d, %d)\n\r", x, y, w, h, r, c);
                  // Serial.print(temp);
                  display.fillRoundRect(x, y, w, h, r, c);
                  break;
  
              case 'C':
                  if (sscanf(s + 3, "\"%2000[^\"]\"", strTemp) == 1) {
                    n = strlen(strTemp);
                    for (int i = 0; i < n; i++)
                    {
                        strTemp[i] = toupper(strTemp[i]);
                    }
                    for (int i = 0; i < n; i += 2)
                    {
                        strTemp[i / 2] = (hexToChar(strTemp[i]) << 4) | (hexToChar(strTemp[i + 1]) & 0x0F);
                    }
                    strTemp[n / 2] = 0;
                    // Serial.print("display.print(\"");
                    // Serial.print(strTemp);
                    // Serial.println("\");");
                    display.print(strTemp);
                  }
                  break;
  
              case 'D':
                  // Two variants: The original
                  // #D(size)*
                  // and our extension:
                  // #D(fontnumber,textcolor,size)*
                  if (sscanf(s + 3, "%d,%d,%d", &l, &r, &c) == 3) { // extended variant
                          display.setTextSize(1);
                          display.setTextColor(r);
                          if (l == 1) { // Terminus
                                  if (c >= 56) {
                                          display.setFont(&TerminusTTF_Bold_4_49_356pt7b);
                                  } else if (c >= 44) {
                                          display.setFont(&TerminusTTF_Bold_4_49_344pt7b);
                                  } else if (c >= 24) {
                                          display.setFont(&TerminusTTF_Bold_4_49_324pt7b);
                                  } else if (c >= 18) {
                                          display.setFont(&TerminusTTF_4_46_018pt7b);
                                  } else {
                                          display.setFont(&TerminusTTF_4_46_014pt7b);
                                  }
                          } else if (l == 2) { // DSEG7
                                  if (c >= 44) {
                                          display.setFont(&DSEG7Classic_Regular_mod744pt7b);
                                  } else if (c >= 33) {
                                          display.setFont(&DSEG7Classic_Regular_mod733pt7b);
                                  } else {
                                          display.setFont(&DSEG7Classic_Regular_mod722pt7b);
                                  }
                          } else if (l == 3) { // FreeSans
                                  if (c >= 24) {
                                          display.setFont(&FreeSans24pt7b);
                                  } else {
                                          display.setFont(&FreeSans12pt7b);
                                  }
                          } else { // return to default-font
                                  display.setFont();
                          }
                  } else if (sscanf(s + 3, "%d", &c) == 1) { // original variant
                          display.setTextSize(c);
                  } else {
                          Serial.print("ERR");
                  }
                  break;
  
              case 'E':
                  sscanf(s + 3, "%d,%d", &x, &y);
                  // sprintf(temp, "display.setCursor(%d, %d)\n", x, y);
                  // Serial.print(temp);
                  display.setCursor(x, y);
                  break;
  
              case 'F':
                  sscanf(s + 3, "%c", &b);
                  // sprintf(temp, "display.setTextWrap(%s)\n", b == 'T' ? "True" : "False");
                  // Serial.print(temp);
                  if (b == 'T')
                      display.setTextWrap(true);
                  if (b == 'F')
                      display.setTextWrap(false);
                  break;
  
              case 'G':
                  sscanf(s + 3, "%d", &c);
                  c &= 3;
                  // sprintf(temp, "display.setRotation(%d)\n", c);
                  // Serial.print(temp);
                  display.setRotation(c);
                  break;
  
              case 'H':
                  sscanf(s + 3, "%d,%d,\"%149[^\"]\"", &x, &y, strTemp);
                  n = strlen(strTemp);
                  for (int i = 0; i < n; i++)
                  {
                      strTemp[i] = toupper(strTemp[i]);
                  }
                  for (int i = 0; i < n; i += 2)
                  {
                      strTemp[i / 2] = (hexToChar(strTemp[i]) << 4) | (hexToChar(strTemp[i + 1]) & 0x0F);
                  }
                  strTemp[n / 2] = 0;
                  r = display.sdCardInit();
                  if (r)
                  {
                      r = display.drawBitmapFromSd(strTemp, x, y);
                      Serial.print("#H(");
                      Serial.print(r, DEC);
                      Serial.println(")*");
                      Serial.flush();
                      // sprintf(temp, "display.drawBitmap(%d, %d, %s)\n", x, y, strTemp);
                      // Serial.print(temp);
                  }
                  else
                  {
                      Serial.println("#H(-1)*");
                      Serial.flush();
                  }
                  break;
  
              case 'I':
                  sscanf(s + 3, "%d", &c);
                  // sprintf(temp, "display.setDisplayMode(%s)\n", c == 0 ? "INKPLATE_1BIT" : "INKPLATE_3BIT");
                  // Serial.print(temp);
                  if (c == 1)
                      display.selectDisplayMode(INKPLATE_1BIT);
                  if (c == 3)
                      display.selectDisplayMode(INKPLATE_3BIT);
                  break;
  
              case 'J':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      // if (0 == 0) {
                      //  Serial.println("#J(0)*");
                      //} else {
                      //  Serial.println("#J(1)*");
                      //}
                      if (display.getDisplayMode() == INKPLATE_1BIT)
                      {
                          Serial.println("#J(0)*");
                          Serial.flush();
                      }
                      if (display.getDisplayMode() == INKPLATE_3BIT)
                      {
                          Serial.println("#J(1)*");
                          Serial.flush();
                      }
                  }
                  break;
  
              case 'K':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      // Serial.print("display.clearDisplay();\n");
                      display.clearDisplay();
                  }
                  break;
  
              case 'L':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      Serial.print("display.display();\n");
                      display.display();
                  }
                  break;
  
              case 'M':
                  sscanf(s + 3, "%d,%d,%d", &y1, &x2, &y2);
                  // sprintf(temp, "display.partialUpdate(%d, %d, %d);\n", y1, x2, y2);
                  // Serial.print(temp);
                  display.partialUpdate();
                  break;
  
              case 'N':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#N(");
                      Serial.print(display.readTemperature(), DEC);
                      // Serial.print(23, DEC);
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
  
              case 'O':
                  sscanf(s + 3, "%d", &c);
                  if (c >= 0 && c <= 2)
                  {
                      Serial.print("#O(");
                      Serial.print(display.readTouchpad(c), DEC);
                      // Serial.print(0, DEC);
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
  
              case 'P':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#P(");
                      Serial.print(display.readBattery(), 2);
                      // Serial.print(3.54, 2);
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
  
              case 'Q':
                  sscanf(s + 3, "%d", &c);
                  c &= 1;
                  // if (c == 0) Serial.print("display.einkOff();\n");
                  // if (c == 1) Serial.print("display.einkOn();\n");
                  if (c == 0)
                      display.einkOff();
                  if (c == 1)
                      display.einkOn();
                  break;
  
              case 'R':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#R(");
                      Serial.print(display.getPanelState(), DEC);
                      // Serial.print(1, DEC);
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
              case 'S':
                  sscanf(s + 3, "%d,%d,\"%149[^\"]\"", &x, &y, strTemp);
                  n = strlen(strTemp);
                  for (int i = 0; i < n; i++)
                  {
                      strTemp[i] = toupper(strTemp[i]);
                  }
                  for (int i = 0; i < n; i += 2)
                  {
                      strTemp[i / 2] = (hexToChar(strTemp[i]) << 4) | (hexToChar(strTemp[i + 1]) & 0x0F);
                  }
                  strTemp[n / 2] = 0;
                  r = display.sdCardInit();
                  if (r)
                  {
                      r = display.drawImage(strTemp, x, y);
                      Serial.print("#H(");
                      Serial.print(r, DEC);
                      Serial.println(")*");
                      Serial.flush();
                      // sprintf(temp, "display.drawBitmap(%d, %d, %s)\n", x, y, strTemp);
                      // Serial.print(temp);
                  }
                  else
                  {
                      Serial.println("#H(-1)*");
                      Serial.flush();
                  }
                  break;
              case 'T':
                  int t;
                  sscanf(s + 3, "%d,%d,%d,%d,%d,%d", &x1, &y1, &x2, &y2, &c, &t);
                  // sprintf(temp, "display.drawLine(%d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, c);
                  // Serial.print(temp);
                  display.drawThickLine(x1, y1, x2, y2, c, t);
                  break;
              case 'U':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &rx, &ry, &xc, &yc, &c);
                  // sprintf(temp, "display.drawLine(%d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, c);
                  // Serial.print(temp);
                  display.drawElipse(rx, ry, xc, yc, c);
                  break;
              case 'V':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &rx, &ry, &xc, &yc, &c);
                  // sprintf(temp, "display.drawLine(%d, %d, %d, %d, %d)\n\r", x1, y1, x2, y2, c);
                  // Serial.print(temp);
                  display.fillElipse(rx, ry, xc, yc, c);
                  break;
              case 'W':
                  sscanf(s + 3, "%d,%d,%d", &hr, &min, &sec);
                  // sprintf(temp, "display.rtcSetTime(%d, %d, %d);\n\r", hr, min, sec);
                  // Serial.println(temp);
                  display.rtcSetTime(hr, min, sec);
                  break;
              case 'X':
                  sscanf(s + 3, "%d,%d,%d,%d", &wday, &day, &mon, &yr);
                  // sprintf(temp, "display.rtcSetDate(%d, %d, %d, %d);\n\r", wday, day, mon, yr);
                  // Serial.println(temp);
                  display.rtcSetDate(wday, day, mon, yr);
                  break;
              case 'Y':
                  sscanf(s + 3, "%d", &ep);
                  // sprintf(temp, "display.rtcSetEpoch(%d);\n\r", ep);
                  // Serial.println(temp);
                  display.rtcSetEpoch(ep);
                  break;
              case 'Z':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#Z(");
                      Serial.print(display.rtcGetEpoch());
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
              case 'a':
                  sscanf(s + 3, "%d", &k);
                  if (k > 6)
                  {
                      Serial.println("INVALID");
                      break;
                  }
                  display.rtcGetRtcData();
                  Serial.print("#a(");
                  switch (k)
                  {
                  case 0:
                      Serial.print(display.rtcGetSecond());
                      break;
                  case 1:
                      Serial.print(display.rtcGetMinute());
                      break;
                  case 2:
                      Serial.print(display.rtcGetHour());
                      break;
                  case 3:
                      Serial.print(display.rtcGetDay());
                      break;
                  case 4:
                      Serial.print(display.rtcGetWeekday());
                      break;
                  case 5:
                      Serial.print(display.rtcGetMonth());
                      break;
                  case 6:
                      Serial.print(display.rtcGetYear());
                      break;
                  }
                  Serial.println(")*");
                  Serial.flush();
                  break;
              case 'b':
                  sscanf(s + 3, "%d,%d,%d,%d,%d", &as, &am, &ah, &ad, &aw);
                  // sprintf(temp, "display.rtcSetAlarm(%d, %d, %d, %d, %d);\n\r", as, am, ah, ad, aw);
                  // Serial.println(temp);
                  display.rtcSetAlarm(as, am, ah, ad, aw);
                  break;
              case 'c':
                  sscanf(s + 3, "%d,%d", &ae, &amc);
                  // sprintf(temp, "display.rtcSetAlarmEpoch(%d, %d);\n\r", ae, amc);
                  // Serial.println(temp);
                  display.rtcSetAlarmEpoch(ae, amc);
                  break;
              case 'd':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#d(");
                      Serial.print(display.rtcCheckAlarmFlag());
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
              case 'e':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      // Serial.println("display.rtcClearAlarmFlag()");
                      display.rtcClearAlarmFlag();
                      Serial.flush();
                  }
                  break;
              case 'f':
                  sscanf(s + 3, "%d", &k);
                  if (k > 4)
                  {
                      Serial.println("INVALID");
                      break;
                  }
                  Serial.print("#f(");
                  switch (k)
                  {
                  case 0:
                      Serial.print(display.rtcGetAlarmSecond());
                      break;
                  case 1:
                      Serial.print(display.rtcGetAlarmMinute());
                      break;
                  case 2:
                      Serial.print(display.rtcGetAlarmHour());
                      break;
                  case 3:
                      Serial.print(display.rtcGetAlarmDay());
                      break;
                  case 4:
                      Serial.print(display.rtcGetAlarmWeekday());
                      break;
                  }
                  Serial.println(")*");
                  Serial.flush();
                  break;
              case 'g':
                  sscanf(s + 3, "%d,%d,%d,%d", &sc, &v, &ie, &ip);
  
                  if (sc > 3)
                  {
                      Serial.println("ERROR");
                      break;
                  }
  
                  // sprintf(temp, "display.rtcTimerSet(%d, %d, %d, %d);\n\r", sc, v, ie, ip);
                  // Serial.println(temp);
                  display.rtcTimerSet(sc, v, ie, ip);
                  break;
              case 'h':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#h(");
                      Serial.print(display.rtcCheckTimerFlag());
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
              case 'i':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      // Serial.println("display.rtcClearTimerFlag()");
                      display.rtcClearTimerFlag();
                  }
                  break;
              case 'j':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      // Serial.println("display.rtcDisableTimer()");
                      display.rtcDisableTimer();
                  }
                  break;
              case 'k':
                  sscanf(s + 3, "%c", &b);
                  if (b == '?')
                  {
                      Serial.print("#k(");
                      Serial.print(display.rtcIsSet());
                      Serial.println(")*");
                      Serial.flush();
                  }
                  break;
              case 'l':
                  sscanf(s + 3, "%c", &b);
                  if (b == '1')
                  {
                      // Serial.println("display.rtcReset()");
                      display.rtcReset();
                  }
                  break;
              case 'm':
                  if (sscanf(s + 3, "%d,%d,%d,%d,\"%2000[^\"]\"", &x1, &y1, &w, &c, strTemp) == 5) {
                    if (w >= 1) {
                      x = 0; y = 0; int bitsremaining = 0; char * sp = &strTemp[0];
                      while (1) {
                        if (bitsremaining <= 0) { // Read next byte
                          if (strlen(sp) < 2) { // not enough characters remaining. Abort.
                            break;
                          }
                          b = (hexToChar(toupper(*sp)) << 4) | (hexToChar(toupper(*(sp + 1))) & 0x0F);
                          bitsremaining = 8;
                          sp += 2;
                        }
                        if (b & 0x80) { // current pixel is set
                          display.drawPixel(x1 + x, y1 + y, c);
                        }
                        b <<= 1;
                        bitsremaining--;
                        x++;
                        if (x >= w) { x = 0; y++; }
                      }
                    }
                    //display.print(strTemp);
                  }
                  break;
              }
              // We have processed this command. Now shift the buffer to the left.
              memmove(s, e+1, (BUFFER_SIZE + 1 - (e - s)));
              bytesinbuf -= ((e - s) + 1);
              // make sure at and after the end of the string are only 0 bytes
              // (note it's BUFFER_SIZE+1 big, so this is correct)
              for (int i = bytesinbuf; i < (BUFFER_SIZE+1); i++) {
                commandBuffer[i] = 0;
              }
      } /* if e != 0 */
    } while (e != NULL);
}

