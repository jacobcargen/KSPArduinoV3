/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Output.h"
#include <pins_arduino.h> 
#include <variant.h> 
#include <LiquidCrystal_I2C.h>

int const _SHIFT_OUT_A_DATA_PIN = 2;
int const _SHIFT_OUT_A_LATCH_PIN = 3;
int const _SHIFT_OUT_A_CLOCK_PIN = 4;
int const _SHIFT_OUT_B_DATA_PIN = 5;
int const _SHIFT_OUT_B_LATCH_PIN = 6;
int const _SHIFT_OUT_B_CLOCK_PIN = 7;
int const _SHIFT_OUT_C_DATA_PIN = 8;
int const _SHIFT_OUT_C_LATCH_PIN = 9;
int const _SHIFT_OUT_C_CLOCK_PIN = 10;

int const ARDUINO_PINS[10] = {22,23,24,25,26,27,28,29,30,31};

// Shift out pins
bool _sA[64] = { 0 };
bool _sB[64] = { 0 };
bool _sC[8]  = { 0 };

bool arduinoPinsOutput[10] = { 0 };

// Heading LCD
LiquidCrystal_I2C _headingLCD(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
// Speed LCD
LiquidCrystal_I2C _speedLCD(0x26, 16, 2);
// Altitude LCD
LiquidCrystal_I2C _altitudeLCD(0x25, 16, 2);
// Info LCD
LiquidCrystal_I2C _infoLCD(0x22, 16, 2);
// Direction LCD
LiquidCrystal_I2C _directionLCD(0x23, 16, 2);

// Text for speed lcd
String _speedLCDTopTxt, _speedLCDBotTxt;
String _lastSpeedTop, _lastSpeedBot;
// Text for altitude lcd
String _altitudeLCDTopTxt, _altitudeLCDBotTxt;
String _lastAltitudeTop, _lastAltitudeBot;
// Text for info lcd
String _infoLCDTopTxt, _infoLCDBotTxt;
String _lastInfoTop, _lastInfoBot;
// Text for heading lcd
String _headingLCDTopTxt, _headingLCDBotTxt;
String _lastHeadingTop, _lastHeadingBot;
// Text for direction lcd
String _directionLCDTopTxt, _directionLCDBotTxt;
String _lastDirectionTop, _lastDirectionBot;


void _sendShiftOut(bool states[], int size, int dataPin, int latchPin, int clockPin)
{
    // Define outputA (4 bytes)
    unsigned long outputA = 0;
    // Define outputB (4 bytes)
    unsigned long outputB = 0;

    // For each pin/bit
    for (int pin = 0; pin < size; pin++)
    {
        // First 4 bytes
        if (pin <= 31 && states[pin] == 1)
        {
            // Set the value for THIS pin/bit to 1 in reverse order
            bitSet(outputA, pin);
        }
        // Last 4 bytes
        else if (states[pin] == 1)
        {
            // Set the value for THIS pin/bit to 1 in reverse order
            bitSet(outputB, pin - 32);
        }
    }

    // Break down the bytes 4-2 bytes
    unsigned int b0_1 = int(outputA);
    unsigned int b2_3 = int(outputA >> 16);
    unsigned int b4_5 = int(outputB);
    unsigned int b6_7 = int(outputB >> 16);

    // Break down the bytes 2-1 bytes
    byte b0 = lowByte(b0_1);
    byte b1 = highByte(b0_1);
    byte b2 = lowByte(b2_3);
    byte b3 = highByte(b2_3);
    byte b4 = lowByte(b4_5);
    byte b5 = highByte(b4_5);
    byte b6 = lowByte(b6_7);
    byte b7 = highByte(b6_7);
    // Disable
    digitalWrite(latchPin, LOW);
    // Shift the values into the register starting from the first register
    shiftOut(dataPin, clockPin, MSBFIRST, b7);
    shiftOut(dataPin, clockPin, MSBFIRST, b6);
    shiftOut(dataPin, clockPin, MSBFIRST, b5);
    shiftOut(dataPin, clockPin, MSBFIRST, b4);
    shiftOut(dataPin, clockPin, MSBFIRST, b3);
    shiftOut(dataPin, clockPin, MSBFIRST, b2);
    shiftOut(dataPin, clockPin, MSBFIRST, b1);
    shiftOut(dataPin, clockPin, MSBFIRST, b0);
    // Enable
    digitalWrite(latchPin, HIGH);
}

void _sendLCD(LiquidCrystal_I2C &lcd, String &lastLine1, String &lastLine2, String newLine1, String newLine2)
{
    // Only update if text changed
    if (lastLine1 != newLine1 || lastLine2 != newLine2)
    {
        // Clear LCD only if needed
        lcd.clear();
        // Print to top line
        lcd.setCursor(0, 0);
        lcd.print(newLine1);
        // Print to bottom line
        lcd.setCursor(0, 1);
        lcd.print(newLine2);
        
        // Update last values
        lastLine1 = newLine1;
        lastLine2 = newLine2;
    }
}

/// <summary>Initialize the ouotputs for use.</summary>
void OutputClass::init()
{
    // LCDs
    _headingLCD.begin();
    _speedLCD.begin();
    _altitudeLCD.begin();
    _infoLCD.begin();
    _directionLCD.begin();

    setSpeedLCD("SPEED TEST", "SPEED TEST");
    setAltitudeLCD("ALTITUDE TEST", "ALTITUDE TEST");
    setHeadingLCD("HEADING TEST", "HEADING TEST");
    setDirectionLCD("DIRECTION TEST", "DIRECTION TEST");
    setInfoLCD("INFO TEST", "INFO TEST");

    // Shift register pins
    pinMode(_SHIFT_OUT_A_DATA_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_A_LATCH_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_A_CLOCK_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_B_DATA_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_B_LATCH_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_B_CLOCK_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_C_DATA_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_C_LATCH_PIN, OUTPUT);
    pinMode(_SHIFT_OUT_C_CLOCK_PIN, OUTPUT);

	for (auto pin : ARDUINO_PINS)
	{
		pinMode(pin, OUTPUT);
	}

    _sendShiftOut(_sA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    _sendShiftOut(_sB, 64, _SHIFT_OUT_B_DATA_PIN, _SHIFT_OUT_B_LATCH_PIN, _SHIFT_OUT_B_CLOCK_PIN);
    _sendShiftOut(_sC, 8, _SHIFT_OUT_C_DATA_PIN, _SHIFT_OUT_C_LATCH_PIN, _SHIFT_OUT_C_CLOCK_PIN);
    
}
/// <summary>Update the controller outputs.</summary>
void OutputClass::update()
{
    
    _sendLCD(_speedLCD, _lastSpeedTop, _lastSpeedBot, _speedLCDTopTxt, _speedLCDBotTxt);
    _sendLCD(_altitudeLCD, _lastAltitudeTop, _lastAltitudeBot, _altitudeLCDTopTxt, _altitudeLCDBotTxt);
    _sendLCD(_headingLCD, _lastHeadingTop, _lastHeadingBot, _headingLCDTopTxt, _headingLCDBotTxt);
    _sendLCD(_infoLCD, _lastInfoTop, _lastInfoBot, _infoLCDTopTxt, _infoLCDBotTxt);
    _sendLCD(_directionLCD, _lastDirectionTop, _lastDirectionBot, _directionLCDTopTxt, _directionLCDBotTxt);
    
    _sendShiftOut(_sA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    _sendShiftOut(_sB, 64, _SHIFT_OUT_B_DATA_PIN, _SHIFT_OUT_B_LATCH_PIN, _SHIFT_OUT_B_CLOCK_PIN);
    _sendShiftOut(_sC, 8, _SHIFT_OUT_C_DATA_PIN, _SHIFT_OUT_C_LATCH_PIN, _SHIFT_OUT_C_CLOCK_PIN);
    for (auto pin : ARDUINO_PINS)
	{
		digitalWrite(pin, arduinoPinsOutput[pin - 22]);
	}
}

void OutputClass::setLED(int pin, bool state)
{
	// edge case, need to flip dont ask why. It was easier to do this then try and dig through the wires...
	if (pin == 111 || pin == 112)
		state = !state;
		
	if (pin < 64)
	    _sA[pin] = state;
	else if (pin < 128)
		_sB[pin - 64] = state;
	else if (pin < 136)
	    _sC[pin - 128] = state;
	else if (pin <= TOTAL_LEDS) // Not on shift register, on arduino
		arduinoPinsOutput[pin - 136] = state;
}

// Displays
void OutputClass::setSpeedLCD(String top, String bot)
{
    _speedLCDTopTxt = top;
    _speedLCDBotTxt = bot;
}
void OutputClass::setAltitudeLCD(String top, String bot)
{
    _altitudeLCDTopTxt = top;
    _altitudeLCDBotTxt = bot;
}
void OutputClass::setHeadingLCD(String top, String bot)
{
    _headingLCDTopTxt = top;
    _headingLCDBotTxt = bot;
}
void OutputClass::setDirectionLCD(String top, String bot)
{
    _directionLCDTopTxt = top;
    _directionLCDBotTxt = bot;
}
void OutputClass::setInfoLCD(String top, String bot)
{
    _infoLCDTopTxt = top;
    _infoLCDBotTxt = bot;
}

OutputClass Output;
