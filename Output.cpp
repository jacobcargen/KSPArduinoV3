/*
 Name:		Kerbal_Controller_Arduino rev2.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Output.h"
#include <pins_arduino.h> 
#include <variant.h> 
#include <LiquidCrystal_I2C.h>


#pragma region Shift Out Variables

int const _SHIFT_OUT_A_DATA_PIN = 2;
int const _SHIFT_OUT_A_LATCH_PIN = 3;
int const _SHIFT_OUT_A_CLOCK_PIN = 4;
int const _SHIFT_OUT_B_DATA_PIN = 5;
int const _SHIFT_OUT_B_LATCH_PIN = 6;
int const _SHIFT_OUT_B_CLOCK_PIN = 7;
int const _SHIFT_OUT_C_DATA_PIN = 8;
int const _SHIFT_OUT_C_LATCH_PIN = 9;
int const _SHIFT_OUT_C_CLOCK_PIN = 10;

// Shift out pins
bool _shiftOutA[64] = { 0 };
bool _shiftOutB[64] = { 0 };
bool _shiftOutC[16] = { 0 };

#pragma endregion

#pragma region LCD Variables

// Heading LCD
LiquidCrystal_I2C _headingLCD(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
// Speed LCD
LiquidCrystal_I2C _speedLCD(0x26, 16, 2);
// Altitude LCD
LiquidCrystal_I2C _altitudeLCD(0x25, 16, 2);
// Info LCD
LiquidCrystal_I2C _infoLCD(0x23, 16, 2);
// Direction LCD
LiquidCrystal_I2C _directionLCD(0x22, 16, 2);

// Text for speed lcd
String _speedLCDTopTxt, _speedLCDBotTxt;
// Text for altitude lcd
String _altitudeLCDTopTxt, _altitudeLCDBotTxt;
// Text for info lcd
String _infoLCDTopTxt, _infoLCDBotTxt;
// Text for heading lcd
String _headingLCDTopTxt, _headingLCDBotTxt;
// Text for direction lcd
String _directionLCDTopTxt, _directionLCDBotTxt;

#pragma endregion

#pragma region Private Methods

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


/// <summary>
/// Update the I2C LCD displays.
/// </summary>
/// <param name="lcd">LCD to update.</param>
/// <param name="topTxt">String for line 1 on the LCD.</param>
/// <param name="botTxt">String for line 2 on the LCD.</param>
void _sendLCD(LiquidCrystal_I2C lcd, String line1, String line2)
{
    // Clear LCD
    lcd.clear();
    // Print to top line
    lcd.setCursor(0, 0);
    lcd.print(line1);
    // Print to bottom line
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

#pragma endregion

#pragma region Public Methods

void OutputClass::overrideSet(bool x[144])
{
    for (int i = 0; i < 64; i++)
    {
        _shiftOutA[i] = x[i];
    }
    for (int i = 0; i < 64; i++)
    {
        _shiftOutB[i] = x[i + 64];
    }
    for (int i = 0; i < 16; i++)
    {
        _shiftOutC[i] = x[i + 80];
    }
    
}
/// <summary>Initialize the ouotputs for use.</summary>
void OutputClass::init()
{
    bool x[144];
    for (int i = 0; i < 144; i++)
    {
        x[i] = 0;
    }
    overrideSet(x);
    // LCDs
    /*
    _headingLCD.begin();
    _speedLCD.begin();
    _altitudeLCD.begin();
    _infoLCD.begin();
    _directionLCD.begin();

    _speedLCD.print("SPEED TEST");
    _headingLCD.print("HEADING TEST");
    _altitudeLCD.print("ALTITUDE TEST");
    _infoLCD.print("INFO TEST");
    _directionLCD.print("DIRECTION TEST");
    */
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

    _sendShiftOut(_shiftOutA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    
}
/// <summary>Update the controller outputs.</summary>
void OutputClass::update()
{
    //_sendLCD(_speedLCD, _speedLCDTopTxt, _speedLCDBotTxt);
    //_sendLCD(_altitudeLCD, _altitudeLCDTopTxt, _altitudeLCDBotTxt);
    //_sendLCD(_headingLCD, _headingLCDTopTxt, _headingLCDBotTxt);
    //_sendLCD(_infoLCD, _infoLCDTopTxt, _infoLCDBotTxt);
    //_sendLCD(_directionLCD, _directionLCDTopTxt, _directionLCDBotTxt);

    //_shiftOutB[48] = false;

    _sendShiftOut(_shiftOutA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    //_sendShiftOut(_shiftOutB, 64, _SHIFT_OUT_B_DATA_PIN, _SHIFT_OUT_B_LATCH_PIN, _SHIFT_OUT_B_CLOCK_PIN);
    //_sendShiftOut(_shiftOutC, 64, _SHIFT_OUT_C_DATA_PIN, _SHIFT_OUT_C_LATCH_PIN, _SHIFT_OUT_C_CLOCK_PIN);
}


// Misc

void OutputClass::setPowerLED(bool state)
{
    _shiftOutA[0] = state;
}
void OutputClass::setSpeaker(bool state)
{

}

// Warnings

void OutputClass::setTempWarningLED(bool state)
{
    _shiftOutB[37] = state;
}
void OutputClass::setGeeWarningLED(bool state)
{
    _shiftOutB[38] = state;
}
void OutputClass::setWarpWarningLED(bool state)
{
    _shiftOutB[39] = state;
}
void OutputClass::setBrakeWarningLED(bool state)
{
    _shiftOutB[40] = state;
}
void OutputClass::setSASWarningLED(bool state)
{
    _shiftOutB[41] = state;
}
void OutputClass::setRCSWarningLED(bool state)
{
    _shiftOutB[42] = state;
}
void OutputClass::setGearWarningLED(bool state)
{
    _shiftOutB[43] = state;
}
void OutputClass::setCommsWarningLED(bool state)
{
    _shiftOutB[44] = state;
}
void OutputClass::setAltWarningLED(bool state)
{
    _shiftOutB[45] = state;
}
void OutputClass::setPitchWarningLED(bool state)
{
    _shiftOutB[46] = state;
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

// Resources

void OutputClass::setSolidFuelLEDs(bool states[20])
{
    _shiftOutA[1] = states[0];
    _shiftOutA[2] = states[1];
    _shiftOutA[3] = states[2];
    _shiftOutA[4] = states[3];
    _shiftOutA[5] = states[4];
    _shiftOutA[6] = states[5];
    _shiftOutA[7] = states[6];
    _shiftOutA[8] = states[7];
    _shiftOutA[9] = states[8];
    _shiftOutA[10] = states[9];
    _shiftOutA[11] = states[10];
    _shiftOutA[12] = states[11];
    _shiftOutA[13] = states[12];
    _shiftOutA[14] = states[13];
    _shiftOutA[15] = states[14];
    _shiftOutA[16] = states[15];
    _shiftOutA[17] = states[16];
    _shiftOutA[18] = states[17];
    _shiftOutA[19] = states[18];
    _shiftOutA[20] = states[19];
}
void OutputClass::setLiquidFuelLEDs(bool states[20])
{
    _shiftOutA[21] = states[0];
    _shiftOutA[22] = states[1];
    _shiftOutA[23] = states[2];
    _shiftOutA[24] = states[3];
    _shiftOutA[25] = states[4];
    _shiftOutA[26] = states[5];
    _shiftOutA[27] = states[6];
    _shiftOutA[28] = states[7];
    _shiftOutA[29] = states[8];
    _shiftOutA[30] = states[9];
    _shiftOutA[31] = states[10];
    _shiftOutA[32] = states[11];
    _shiftOutA[33] = states[12];
    _shiftOutA[34] = states[13];
    _shiftOutA[35] = states[14];
    _shiftOutA[36] = states[15];
    _shiftOutA[37] = states[16];
    _shiftOutA[38] = states[17];
    _shiftOutA[39] = states[18];
    _shiftOutA[40] = states[19];
}
void OutputClass::setOxidizerLEDs(bool states[20])
{
    _shiftOutA[41] = states[0];
    _shiftOutA[42] = states[1];
    _shiftOutA[43] = states[2];
    _shiftOutA[44] = states[3];
    _shiftOutA[45] = states[4];
    _shiftOutA[46] = states[5];
    _shiftOutA[47] = states[6];
    _shiftOutA[48] = states[7];
    _shiftOutA[49] = states[8];
    _shiftOutA[50] = states[9];
    _shiftOutA[51] = states[10];
    _shiftOutA[52] = states[11];
    _shiftOutA[53] = states[12];
    _shiftOutA[54] = states[13];
    _shiftOutA[55] = states[14];
    _shiftOutA[56] = states[15];
    _shiftOutA[57] = states[16];
    _shiftOutA[58] = states[17];
    _shiftOutA[59] = states[18];
    _shiftOutA[60] = states[19];
}
void OutputClass::setMonopropellantLEDs(bool states[20])
{
    _shiftOutA[61] = states[0];
    _shiftOutA[62] = states[1];
    _shiftOutA[63] = states[2];

    _shiftOutB[0] = states[3];
    _shiftOutB[1] = states[4];
    _shiftOutB[2] = states[5];
    _shiftOutB[3] = states[6];
    _shiftOutB[4] = states[7];
    _shiftOutB[5] = states[8];
    _shiftOutB[6] = states[9];
    _shiftOutB[7] = states[10];
    _shiftOutB[8] = states[11];
    _shiftOutB[9] = states[12];
    _shiftOutB[10] = states[13];
    _shiftOutB[11] = states[14];
    _shiftOutB[12] = states[15];
    _shiftOutB[13] = states[16];
    _shiftOutB[14] = states[17];
    _shiftOutB[15] = states[18];
    _shiftOutB[16] = states[19];
}
void OutputClass::setElectricityLEDs(bool states[20])
{
    _shiftOutB[17] = states[0];
    _shiftOutB[18] = states[1];
    _shiftOutB[19] = states[2];
    _shiftOutB[20] = states[3];
    _shiftOutB[21] = states[4];
    _shiftOutB[22] = states[5];
    _shiftOutB[23] = states[6];
    _shiftOutB[24] = states[7];
    _shiftOutB[25] = states[8];
    _shiftOutB[26] = states[9];
    _shiftOutB[27] = states[10];
    _shiftOutB[28] = states[11];
    _shiftOutB[29] = states[12];
    _shiftOutB[30] = states[13];
    _shiftOutB[31] = states[14];
    _shiftOutB[32] = states[15];
    _shiftOutB[33] = states[16];
    _shiftOutB[34] = states[17];
    _shiftOutB[35] = states[18];
    _shiftOutB[36] = states[19];
}

// Staging

void OutputClass::setStageLED(bool state)
{
    _shiftOutB[47] = state;
}

// Aborting

void OutputClass::setAbortLED(bool state)
{
    _shiftOutB[49] = state;
}

// Custom Action Groups

void OutputClass::setCAG1LED(bool state)
{
    _shiftOutB[51] = state;
}
void OutputClass::setCAG2LED(bool state)
{
    _shiftOutB[52] = state;
}
void OutputClass::setCAG3LED(bool state)
{
    _shiftOutB[53] = state;
}
void OutputClass::setCAG4LED(bool state)
{
    _shiftOutB[54] = state;
}
void OutputClass::setCAG5LED(bool state)
{
    _shiftOutB[55] = state;
}
void OutputClass::setCAG6LED(bool state)
{
    _shiftOutB[56] = state;
}
void OutputClass::setCAG7LED(bool state)
{
    _shiftOutB[57] = state;
}
void OutputClass::setCAG8LED(bool state)
{
    _shiftOutB[58] = state;
}
void OutputClass::setCAG9LED(bool state)
{
    _shiftOutB[59] = state;
}
void OutputClass::setCAG10LED(bool state)
{
    _shiftOutB[60] = state;
}

// SAS

void OutputClass::setSASStabilityAssistLED(bool state)
{
    _shiftOutB[63] = state;
}
void OutputClass::setSASManeuverLED(bool state)
{
    _shiftOutC[0] = state;
}
void OutputClass::setSASProgradeLED(bool state)
{
    _shiftOutC[1] = state;
}
void OutputClass::setSASRetrogradeLED(bool state)
{
    _shiftOutC[2] = state;
}
void OutputClass::setSASNormalLED(bool state)
{
    _shiftOutC[3] = state;
}
void OutputClass::setSASAntiNormalLED(bool state)
{
    _shiftOutC[4] = state;
}
void OutputClass::setSASRadialInLED(bool state)
{
    _shiftOutC[5] = state;
}
void OutputClass::setSASRadialOutLED(bool state)
{
    _shiftOutC[6] = state;
}
void OutputClass::setSASTargetLED(bool state)
{
    _shiftOutC[7] = state;
}
void OutputClass::setSASAntiTargetLED(bool state)
{
    _shiftOutC[8] = state;
}

#pragma endregion

OutputClass Output;
