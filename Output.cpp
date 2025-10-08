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
/*
enum ShiftOutAPins {
    POWER_LED = 0,
    SF_1 = 1,
    SF_2 = 2,
    SF_3 = 3,
    SF_4 = 4,
    SF_5 = 5,
    SF_6 = 6,
    SF_7 = 7,
    SF_8 = 8,
    SF_9 = 9,
    SF_10 = 10,
    SF_11 = 11,
    SF_12 = 12,
    SF_13 = 13,
    SF_14 = 14,
    SF_15 = 15,
    SF_16 = 16,
    SF_17 = 17,
    SF_18 = 18,
    SF_19 = 19,
    SF_20 = 20,
    LF_1 = 21,
    LF_2 = 22,
    LF_3 = 23,
    LF_4 = 24,
    LF_5 = 25,
    LF_6 = 26,
    LF_7 = 27,
    LF_8 = 28,
    LF_9 = 29,
    LF_10 = 30,
    LF_11 = 31,
    LF_12 = 32,
    LF_13 = 33,
    LF_14 = 34,
    LF_15 = 35,
    LF_16 = 36,
    LF_17 = 37,
    LF_18 = 38,
    LF_19 = 39,
    LF_20 = 40,
    OX_1 = 41,
    OX_2 = 42,
    OX_3 = 43,
    OX_4 = 44,
    OX_5 = 45,
    OX_6 = 46,
    OX_7 = 47,
    OX_8 = 48,
    OX_9 = 49,
    OX_10 = 50,
    OX_11 = 51,
    OX_12 = 52,
    OX_13 = 53,
    OX_14 = 54,
    OX_15 = 55,
    OX_16 = 56,
    OX_17 = 57,
    OX_18 = 58,
    OX_19 = 59,
    OX_20 = 60,
    MP_1 = 61,
    MP_2 = 62,
    MP_3 = 63,
};

enum ShiftOutBPins {
    MP_4 = 0,
    MP_5 = 1,
    MP_6 = 2,
    MP_7 = 3,
    MP_8 = 4,
    MP_9 = 5,
    MP_10 = 6,
    MP_11 = 7,
    MP_12 = 8,
    MP_13 = 9,
    MP_14 = 10,
    MP_15 = 11,
    MP_16 = 12,
    MP_17 = 13,
    MP_18 = 14,
    MP_19 = 15,
    MP_20 = 16,
    EC_1 = 17,
    EC_2 = 18,
    EC_3 = 19,
    EC_4 = 20,
    EC_5 = 21,
    EC_6 = 22,
    EC_7 = 23,
    EC_8 = 24,
    EC_9 = 25,
    EC_10 = 26,
    EC_11 = 27,
    EC_12 = 28,
    EC_13 = 29,
    EC_14 = 30,
    EC_15 = 31,
    EC_16 = 32,
    EC_17 = 33,
    EC_18 = 34,
    EC_19 = 35,
    EC_20 = 36,
    TempWarningLED = 37,
    GeeWarningLED = 38,
    WarpWarningLED = 39,
    BrakeWarningLED = 40,
    SASWarningLED
    RCSWarningLED
    GearWarningLED
    CommsWarningLED
    AltWarningLED
    PitchWarningLED
};

enum ShiftOutCPins {
    // Add enums for ShiftOutCPins here
};
*/
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

const byte TEST_LED = 53;

bool testLEDState = false;

// Shift out pins
bool _sA[64] = { 0 };
bool _sB[64] = { 0 };
bool _sC[16] = { 0 };

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
// Test LCD
LiquidCrystal_I2C _testLCD(0x24, 16, 2);

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
// Text for test lcd
//String _testLCDTopTxt, _testLCDBotTxt;

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
        _sA[i] = x[i];
    }
    for (int i = 0; i < 64; i++)
    {
        _sB[i] = x[i + 64];
    }
    for (int i = 0; i < 16; i++)
    {
        _sC[i] = x[i + 80];
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

    
    pinMode(TEST_LED, OUTPUT);

    _sendShiftOut(_sA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    _sendShiftOut(_sB, 64, _SHIFT_OUT_B_DATA_PIN, _SHIFT_OUT_B_LATCH_PIN, _SHIFT_OUT_B_CLOCK_PIN);
    _sendShiftOut(_sC, 64, _SHIFT_OUT_C_DATA_PIN, _SHIFT_OUT_C_LATCH_PIN, _SHIFT_OUT_C_CLOCK_PIN);
    
}
/// <summary>Update the controller outputs.</summary>
void OutputClass::update()
{
    
    _sendLCD(_speedLCD, _speedLCDTopTxt, _speedLCDBotTxt);
    _sendLCD(_altitudeLCD, _altitudeLCDTopTxt, _altitudeLCDBotTxt);
    _sendLCD(_headingLCD, _headingLCDTopTxt, _headingLCDBotTxt);
    _sendLCD(_infoLCD, _infoLCDTopTxt, _infoLCDBotTxt);
    _sendLCD(_directionLCD, _directionLCDTopTxt, _directionLCDBotTxt);
    
    // Update test LCD
    //_sendLCD(_testLCD, _testLCDTopTxt, _testLCDBotTxt);

    //_shiftOutB[48] = false;
    
    _sendShiftOut(_sA, 64, _SHIFT_OUT_A_DATA_PIN, _SHIFT_OUT_A_LATCH_PIN, _SHIFT_OUT_A_CLOCK_PIN);
    _sendShiftOut(_sB, 64, _SHIFT_OUT_B_DATA_PIN, _SHIFT_OUT_B_LATCH_PIN, _SHIFT_OUT_B_CLOCK_PIN);
    _sendShiftOut(_sC, 64, _SHIFT_OUT_C_DATA_PIN, _SHIFT_OUT_C_LATCH_PIN, _SHIFT_OUT_C_CLOCK_PIN);
    
       
    // Update test LED
    if (testLEDState)
        digitalWrite(TEST_LED, HIGH);
    else
        digitalWrite(TEST_LED, LOW);
        
}

void OutputClass::setTestLED(bool state)
{
    testLEDState = state;
}
/*
void OutputClass::setTestLCD(String top, String bot)
{
    _testLCDTopTxt = top;
    _testLCDBotTxt = bot;
}
*/
// Misc

void OutputClass::setStateManual(char reg, int pin, bool state)
{
    if (reg == 'A')
        _sA[pin] = state;
    else if (reg == 'B')
        _sB[pin] = state;
    else if (reg == 'C')
        _sC[pin] = state;
}
void OutputClass::setPowerLED(bool state)
{
    _sA[0] = state;
}
// Warnings

void OutputClass::setTempWarningLED(bool state)
{
    _sB[37] = state;
}
void OutputClass::setGeeWarningLED(bool state)
{
    _sB[38] = state;
}
void OutputClass::setWarpWarningLED(bool state)
{
    _sB[39] = state;
}
void OutputClass::setBrakeWarningLED(bool state)
{
    _sB[40] = state;
}
void OutputClass::setSASWarningLED(bool state)
{
    _sB[41] = state;
}
void OutputClass::setRCSWarningLED(bool state)
{
    _sB[42] = state;
}
void OutputClass::setGearWarningLED(bool state)
{
    _sB[43] = state;
}
void OutputClass::setCommsWarningLED(bool state)
{
    _sB[44] = state;
}
void OutputClass::setAltWarningLED(bool state)
{
    _sB[45] = state;
}
void OutputClass::setPitchWarningLED(bool state)
{
    _sB[46] = state;
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
    _sA[1] = states[0];
    _sA[2] = states[1];
    _sA[3] = states[2];
    _sA[4] = states[3];
    _sA[5] = states[4];
    _sA[6] = states[5];
    _sA[7] = states[6];
    _sA[8] = states[7];
    _sA[9] = states[8];
    _sA[10] = states[9];
    _sA[11] = states[10];
    _sA[12] = states[11];
    _sA[13] = states[12];
    _sA[14] = states[13];
    _sA[15] = states[14];
    _sA[16] = states[15];
    _sA[17] = states[16];
    _sA[18] = states[17];
    _sA[19] = states[18];
    _sA[20] = states[19];
}
void OutputClass::setLiquidFuelLEDs(bool states[20])
{
    _sA[21] = states[0];
    _sA[22] = states[1];
    _sA[23] = states[2];
    _sA[24] = states[3];
    _sA[25] = states[4];
    _sA[26] = states[5];
    _sA[27] = states[6];
    _sA[28] = states[7];
    _sA[29] = states[8];
    _sA[30] = states[9];
    _sA[31] = states[10];
    _sA[32] = states[11];
    _sA[33] = states[12];
    _sA[34] = states[13];
    _sA[35] = states[14];
    _sA[36] = states[15];
    _sA[37] = states[16];
    _sA[38] = states[17];
    _sA[39] = states[18];
    _sA[40] = states[19];
}
void OutputClass::setOxidizerLEDs(bool states[20])
{
    _sA[41] = states[0];
    _sA[42] = states[1];
    _sA[43] = states[2];
    _sA[44] = states[3];
    _sA[45] = states[4];
    _sA[46] = states[5];
    _sA[47] = states[6];
    _sA[48] = states[7];
    _sA[49] = states[8];
    _sA[50] = states[9];
    _sA[51] = states[10];
    _sA[52] = states[11];
    _sA[53] = states[12];
    _sA[54] = states[13];
    _sA[55] = states[14];
    _sA[56] = states[15];
    _sA[57] = states[16];
    _sA[58] = states[17];
    _sA[59] = states[18];
    _sA[60] = states[19];
}
void OutputClass::setMonopropellantLEDs(bool states[20])
{
    _sA[61] = states[0];
    _sA[62] = states[1];
    _sA[63] = states[2];

    _sB[0] = states[3];
    _sB[1] = states[4];
    _sB[2] = states[5];
    _sB[3] = states[6];
    _sB[4] = states[7];
    _sB[5] = states[8];
    _sB[6] = states[9];
    _sB[7] = states[10];
    _sB[8] = states[11];
    _sB[9] = states[12];
    _sB[10] = states[13];
    _sB[11] = states[14];
    _sB[12] = states[15];
    _sB[13] = states[16];
    _sB[14] = states[17];
    _sB[15] = states[18];
    _sB[16] = states[19];
}
void OutputClass::setElectricityLEDs(bool states[20])
{
    _sB[17] = states[0];
    _sB[18] = states[1];
    _sB[19] = states[2];
    _sB[20] = states[3];
    _sB[21] = states[4];
    _sB[22] = states[5];
    _sB[23] = states[6];
    _sB[24] = states[7];
    _sB[25] = states[8];
    _sB[26] = states[9];
    _sB[27] = states[10];
    _sB[28] = states[11];
    _sB[29] = states[12];
    _sB[30] = states[13];
    _sB[31] = states[14];
    _sB[32] = states[15];
    _sB[33] = states[16];
    _sB[34] = states[17];
    _sB[35] = states[18];
    _sB[36] = states[19];
}

// Staging

void OutputClass::setStageLED(bool state)
{
    _sB[47] = state;
}

// Aborting

void OutputClass::setAbortLED(bool state)
{
    _sB[48] = state;
}

// Custom Action Groups

void OutputClass::setCAG1LED(bool state)
{
    _sB[51] = state;
}
void OutputClass::setCAG2LED(bool state)
{
    _sB[52] = state;
}
void OutputClass::setCAG3LED(bool state)
{
    _sB[53] = state;
}
void OutputClass::setCAG4LED(bool state)
{
    _sB[54] = state;
}
void OutputClass::setCAG5LED(bool state)
{
    _sB[55] = state;
}
void OutputClass::setCAG6LED(bool state)
{
    _sB[56] = state;
}
void OutputClass::setCAG7LED(bool state)
{
    _sB[57] = state;
}
void OutputClass::setCAG8LED(bool state)
{
    _sB[58] = state;
}
void OutputClass::setCAG9LED(bool state)
{
    _sB[59] = state;
}
void OutputClass::setCAG10LED(bool state)
{
    _sB[60] = state;
}

// SAS

void OutputClass::setSASStabilityAssistLED(bool state)
{
    _sB[63] = state;
}
void OutputClass::setSASManeuverLED(bool state)
{
    _sC[0] = state;
}
void OutputClass::setSASProgradeLED(bool state)
{
    _sC[1] = state;
}
void OutputClass::setSASRetrogradeLED(bool state)
{
    _sC[2] = state;
}
void OutputClass::setSASNormalLED(bool state)
{
    _sC[3] = state;
}
void OutputClass::setSASAntiNormalLED(bool state)
{
    _sC[4] = state;
}
void OutputClass::setSASRadialInLED(bool state)
{
    _sC[5] = state;
}
void OutputClass::setSASRadialOutLED(bool state)
{
    _sC[6] = state;
}
void OutputClass::setSASTargetLED(bool state)
{
    _sC[7] = state;
}
void OutputClass::setSASAntiTargetLED(bool state)
{
    _sC[8] = state;
}

#pragma endregion

OutputClass Output;
