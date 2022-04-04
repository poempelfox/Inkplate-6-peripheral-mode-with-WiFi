/*
   Inkplate_Bluetooth_Peripheral_Mode_Example example for e-radionica Inkplate 2
   For this example you will need USB cable and an Inkplate 2
   Select "Inkplate 2(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 2(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   This example shows how to use Inkplate as a peripheral device over Bluetooth.
   Note: for this to work you need to use ESP32 Wroover Board definition, as ours currently has a bug :(

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   29 March 2022 by Soldered
*/

#include "Inkplate.h"

#include "BluetoothSerial.h"

#define BUFFER_SIZE 1000

#include "Peripheral.h"

// Declare Inkplate and SerialBluetooth objects
Inkplate display;
BluetoothSerial SerialBT;

// Temporary buffer to send to Peripheral mode code
char commandBuffer[BUFFER_SIZE + 1];

void setup() // Initialize everything
{
    Serial.begin(115200);  //Initialize UART communication with PC

    if (!SerialBT.begin("ESP32")) //Check if bluetooth is initialized successfully
    {
        Serial.println("An error occurred initializing Bluetooth"); //If not, print error message
    }

    display.begin(); //Initialize e-paper
}

void loop()
{
    while (SerialBT.available()) // When Bluetooth available save it and pass to Peripheral.h code
    {
        for (int i = 0; i < (BUFFER_SIZE - 1); i++) //Loop through all commands
        {
            commandBuffer[i] = commandBuffer[i + 1];
        }
        commandBuffer[BUFFER_SIZE - 1] = SerialBT.read();  //Save incoming command in buffer
        Serial.print(commandBuffer[BUFFER_SIZE - 1]);
    }

    // Function in peripheral.h
    run(commandBuffer, BUFFER_SIZE);

    delay(50);
}
