/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Input.h"
#include <pins_arduino.h> 
#include <variant.h>

#pragma region Private

// Shift in A pins (8 registers)
const int SHIFT_IN_A_SERIAL_PIN = 11;
const int SHIFT_IN_A_CLOCK_ENABLE_PIN = 12;
const int SHIFT_IN_A_CLOCK_PIN = 13;
const int SHIFT_IN_A_LOAD_PIN = 14;
// Shift in B pins (2 registers)
const int SHIFT_IN_B_SERIAL_PIN = 15;
const int SHIFT_IN_B_CLOCK_ENABLE_PIN = 16;
const int SHIFT_IN_B_CLOCK_PIN = 17;
const int SHIFT_IN_B_LOAD_PIN = 18;
// Rotation Joystick X-Axis(Roll)
const int ROTATION_X_AXIS_PIN = A0;
// Rotation Joystick Y-Axis(Pitch)
const int ROTATION_Y_AXIS_PIN = A1;
// Rotation Joystick Z-Axis(Yaw)
const int ROTATION_Z_AXIS_PIN = A2;
// Rotation Joystick Button
const int ROTATION_BUTTON_PIN = A3;
// Translation Joystick X-Axis(Left/Right)
const int TRANSLATION_X_AXIS_PIN = A4;
// Translation Joystick Y-Axis(Forward/Back)
const int TRANSLATION_Y_AXIS_PIN = A5;
// Translation Joystick Z-Axis(Up/Down)
const int TRANSLATION_Z_AXIS_PIN = A6;
// Translation Joystick ButtonTRAN
const int TRANSLATION_BUTTON_PIN = A7;
// Throttle Axis
const int THROTTLE_AXIS_PIN = A8;

bool _sA[64];
bool _sB[16];

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers



/// <summary>Gets shift register inputs.(MSBFIRST)</summary>
void _shiftIn(int dataA, int clockEnableA, int clockA, int loadA,
    int dataB, int clockEnableB, int clockB, int loadB)
{
    // Pulse to A
    digitalWrite(loadA, LOW);
    delayMicroseconds(5);
    digitalWrite(loadA, HIGH);
    delayMicroseconds(5);
    byte inputA[8];
    // Get input A data
    digitalWrite(clockA, HIGH);
    digitalWrite(clockEnableA, LOW);
    inputA[0] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[1] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[2] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[3] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[4] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[5] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[6] = shiftIn(dataA, clockA, MSBFIRST);
    inputA[7] = shiftIn(dataA, clockA, MSBFIRST);
    digitalWrite(clockEnableA, HIGH);

    for (size_t i = 0; i < 64; i++)
    {
        if (i < 8)
            if (1 == bitRead(inputA[0], i))
                _sA[i] = 1;
            else if (i < 16)
                if (1 == bitRead(inputA[1], i - 8))
                    _sA[i] = 1;
                else if (i < 24)
                    if (1 == bitRead(inputA[2], i - 16))
                        _sA[i] = 1;
                    else if (i < 32)
                        if (1 == bitRead(inputA[3], i - 24))
                            _sA[i] = 1;
                        else if (i < 40)
                            if (1 == bitRead(inputA[4], i - 32))
                                _sA[i] = 1;
                            else if (i < 48)
                                if (1 == bitRead(inputA[5], i - 40))
                                    _sA[i] = 1;
                                else if (i < 56)
                                    if (1 == bitRead(inputA[6], i - 48))
                                        _sA[i] = 1;
                                    else
                                        if (1 == bitRead(inputA[7], i - 56))
                                            _sA[i] = 1;
    }

    // Pulse to B
    digitalWrite(loadB, LOW);
    delayMicroseconds(5);
    digitalWrite(loadB, HIGH);
    delayMicroseconds(5);
    byte inputB[2];
    // Get input B data
    digitalWrite(clockB, HIGH);
    digitalWrite(clockEnableB, LOW);
    inputB[0] = shiftIn(dataB, clockB, MSBFIRST);
    inputB[1] = shiftIn(dataB, clockB, MSBFIRST);
    digitalWrite(clockEnableB, HIGH);

    for (size_t i = 0; i < 16; i++)
    {
        if (i < 8)
            if (1 == bitRead(inputB[0], i))
                _sB[i] = 1;
            else
                if (1 == bitRead(inputB[1], i - 8))
                    _sB[i] = 1;
    }
}

#pragma endregion


#pragma region Public


void InputClass::init()
{
    
    // Shift register pins
    pinMode(SHIFT_IN_A_SERIAL_PIN, INPUT);
    pinMode(SHIFT_IN_A_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_SERIAL_PIN, INPUT);
    pinMode(SHIFT_IN_B_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_LOAD_PIN, OUTPUT);


}
void InputClass::update()
{
    if ((millis() - lastDebounceTime) > debounceDelay) 
    {
        _shiftIn(SHIFT_IN_A_SERIAL_PIN, SHIFT_IN_A_CLOCK_ENABLE_PIN, SHIFT_IN_A_CLOCK_PIN, SHIFT_IN_A_LOAD_PIN,
            SHIFT_IN_B_SERIAL_PIN, SHIFT_IN_B_CLOCK_ENABLE_PIN, SHIFT_IN_B_CLOCK_PIN, SHIFT_IN_B_LOAD_PIN);
    }
    // Analog updated on call (to be as fast as possible)
}


// Misc

bool InputClass::getDebugSwitch()               { return _sA[0]; }
bool InputClass::getSoundSwitch()               { return _sA[1]; }
bool InputClass::getInputEnableButton()         { return _sA[2]; }

// Warnings

bool InputClass::getTempWarningButton()         { return _sA[3]; }
bool InputClass::getGeeWarningButton()          { return _sA[4]; }
bool InputClass::getWarpWarningButton()         { return _sA[5]; }
bool InputClass::getBrakeWarningButton()        { return _sA[6]; }
bool InputClass::getSASWarningButton()          { return _sA[7]; }
bool InputClass::getRCSWarningButton()          { return _sA[8]; }
bool InputClass::getGearWarningButton()         { return _sA[9]; }
bool InputClass::getCommsWarningButton()        { return _sA[10]; }
bool InputClass::getAltWarningButton()          { return _sA[11]; }
bool InputClass::getPitchWarningButton()        { return _sA[12]; }

// Display Controls

bool InputClass::getStageViewSwitch()           { return _sA[13]; }
bool InputClass::getVerticalVelocitySwitch()    { return _sA[14]; }
bool InputClass::getReferenceModeButton()       { return _sA[15]; }
bool InputClass::getRadarAltitudeSwitch()       { return _sA[16]; }
byte InputClass::getInfoMode()
{
    for (int i = 0; i < 12; i++)
    {
        if (_sA[i + 17])
        {
            return i; // Return index
        }
        else
        {
            // ERROR, should return true
        }
    }
}
byte InputClass::getDirectionMode()
{
    for (int i = 0; i < 12; i++)
    {
        if (_sA[i + 29])
        {
            return i; // Return index
        }
        else
        {
            // ERROR, should return true
        }
    }
}

// Staging

bool InputClass::getStageButton()               { return _sA[17]; }
bool InputClass::getStageLockSwitch()           { return _sA[18]; }

// Aborting

bool InputClass::getAbortButton()               { return _sA[19]; }
bool InputClass::getAbortLockSwitch()           { return _sA[20]; }

// Custom Actions Groups

bool InputClass::getCAG1()                      { return _sA[21]; }
bool InputClass::getCAG2()                      { return _sA[22]; }
bool InputClass::getCAG3()                      { return _sA[23]; }
bool InputClass::getCAG4()                      { return _sA[24]; }
bool InputClass::getCAG5()                      { return _sA[25]; }
bool InputClass::getCAG6()                      { return _sA[26]; }
bool InputClass::getCAG7()                      { return _sA[27]; }
bool InputClass::getCAG8()                      { return _sA[28]; }
bool InputClass::getCAG9()                      { return _sA[29]; }
bool InputClass::getCAG10()                     { return _sA[30]; }

// Other Action Groups

bool InputClass::getDockingSwitch()             { return _sA[31]; }
bool InputClass::getPercisionSwitch()           { return _sA[32]; }
bool InputClass::getLightsSwitch()              { return _sA[33]; }
bool InputClass::getGearSwitch()                { return _sA[34]; }
bool InputClass::getBrakeSwitch()               { return _sA[35]; }

// View

bool InputClass::getScreenshotButton()          { return _sA[36]; }
bool InputClass::getUISwitch()                  { return _sA[37]; }
bool InputClass::getNavSwitch()                 { return _sA[38]; }
bool InputClass::getViewSwitch()                { return _sA[39]; }
bool InputClass::getFocusButton()               { return _sA[40]; }
bool InputClass::getCamModeButton()             { return _sA[41]; }
bool InputClass::getCamResetButton()            { return _sA[42]; }
bool InputClass::getEnableLookButton()          { return analogRead(TRANSLATION_BUTTON_PIN) > 50 ? false : true; }

// Warping & Pause

bool InputClass::getWarpLockSwitch()            { return _sA[43]; }
bool InputClass::getPhysWarpSwitch()            { return _sA[44]; }
bool InputClass::getCancelWarpButton()          { return _sA[45]; }
bool InputClass::getDecreaseWarpButton()        { return _sA[46]; }
bool InputClass::getIncreaseWarpButton()        { return _sA[47]; }
bool InputClass::getPauseButton()               { return _sA[48]; }

// SAS & RCS

bool InputClass::getSASStabilityAssistButton()  { return _sA[49]; }
bool InputClass::getSASManeuverButton()         { return _sA[50]; }
bool InputClass::getSASProgradeButton()         { return _sA[51]; }
bool InputClass::getSASRetrogradeButton()       { return _sA[52]; }
bool InputClass::getSASNormalButton()           { return _sA[53]; }
bool InputClass::getSASAntiNormalButton()       { return _sA[54]; }
bool InputClass::getSASRadialInButton()         { return _sA[55]; }
bool InputClass::getSASRadialOutButton()        { return _sA[56]; }
bool InputClass::getSASTargetButton()           { return _sA[57]; }
bool InputClass::getSASAntiTargetButton()       { return _sA[58]; }
bool InputClass::getSASSwitch()                 { return _sA[59]; }
bool InputClass::getRCSSwitch()                 { return _sA[60]; }

// EVA Specific Controls

bool InputClass::getBoardButton()               { return _sA[61]; }
bool InputClass::getGrabButton()                { return _sA[62]; }
bool InputClass::getJumpButton()                { return analogRead(ROTATION_BUTTON_PIN) > 50 ? false : true; }

// Throttle

int  InputClass::getThrottleAxis()              { return analogRead(THROTTLE_AXIS_PIN); }
bool InputClass::getThrottleLockSwitch()        { return _sA[63]; }

// Translation

int  InputClass::getTranslationXAxis()          { return analogRead(TRANSLATION_X_AXIS_PIN); }
int  InputClass::getTranslationYAxis()          { return analogRead(TRANSLATION_Y_AXIS_PIN); }
int  InputClass::getTranslationZAxis()          { return analogRead(TRANSLATION_Z_AXIS_PIN); }
bool InputClass::getTransHoldButton()           { return _sB[0]; }
bool InputClass::getTransResetButton()          { return _sB[1]; }

// Rotation

int  InputClass::getRotationXAxis()             { return analogRead(ROTATION_X_AXIS_PIN); }
int  InputClass::getRotationYAxis()             { return analogRead(ROTATION_Y_AXIS_PIN); }
int  InputClass::getRotationZAxis()             { return analogRead(ROTATION_Z_AXIS_PIN); }
bool InputClass::getRotHoldButton()             { return _sB[2]; }
bool InputClass::getRotResetButton()            { return _sB[3]; }

#pragma endregion

InputClass Input;
