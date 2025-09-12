/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	9/8/2025 9:27:30 AM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Wire.h"
#include "Print.h"
#include "Output.h"
#include "Input.h"
#include <LiquidCrystal_I2C.h>

// Minimal loop rate tracker (prints every 3 seconds)
static unsigned long __hz_lastPrintMs = 0;
static unsigned long __hz_loopCount = 0;

void setup()
{
    // Open up the serial port
    Serial.begin(115200);
    // Init I/O
    Output.init();
    Input.init(Serial);  // ✅ Pass Serial to init
    Input.setAllVPinsReady();

    // Additional things to do at start AFTER initialization
    Output.setPowerLED(true);
}

void loop()
{
    delay(1000); // Small delay to avoid overwhelming the CPU
    __hz_loopCount++;
    Input.update();
    //Output.update();

    auto state = Input.getDebugSwitch(true);
    if (state != NOT_READY) {
        Serial.print(" State changed to: " + String(state) + " ");
    }
    for (int i = 0; i < 8; i++) {
        Input.debugInputState(i);
    }

    // Print loop rate every 3 seconds
    unsigned long now = millis();
    if (now - __hz_lastPrintMs >= 3000UL) {
        unsigned long secs = (now - __hz_lastPrintMs) / 1000UL;
        if (secs == 0) secs = 1; // safety
        unsigned long hz = __hz_loopCount / secs;
        Serial.print("Loop rate: ");
        Serial.print(hz);
        Serial.println(" Hz");
        __hz_lastPrintMs = now;
        __hz_loopCount = 0;
    }
}
