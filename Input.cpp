/*
 Name:		Kerbal_Controller_Arduino rev2.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Input.h"


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

bool _shiftInA[64];
bool _shiftInB[16];

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
                _shiftInA[i] = 1;
            else if (i < 16)
                if (1 == bitRead(inputA[1], i - 8))
                    _shiftInA[i] = 1;
                else if (i < 24)
                    if (1 == bitRead(inputA[2], i - 16))
                        _shiftInA[i] = 1;
                    else if (i < 32)
                        if (1 == bitRead(inputA[3], i - 24))
                            _shiftInA[i] = 1;
                        else if (i < 40)
                            if (1 == bitRead(inputA[4], i - 32))
                                _shiftInA[i] = 1;
                            else if (i < 48)
                                if (1 == bitRead(inputA[5], i - 40))
                                    _shiftInA[i] = 1;
                                else if (i < 56)
                                    if (1 == bitRead(inputA[6], i - 48))
                                        _shiftInA[i] = 1;
                                    else
                                        if (1 == bitRead(inputA[7], i - 56))
                                            _shiftInA[i] = 1;
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
                _shiftInB[i] = 1;
            else
                if (1 == bitRead(inputB[1], i - 8))
                    _shiftInB[i] = 1;
    }
}

#pragma endregion


#pragma region Public

InputClass::InputClass()
{

}

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
}


// Misc

bool InputClass::getDebugSwitch()               { return _shiftInA[0]; }
bool InputClass::getSoundSwitch()               { return _shiftInA[1]; }
bool InputClass::getInputEnableButton()         { return _shiftInA[2]; }

// Warnings

bool InputClass::getTempWarningButton()         { return _shiftInA[3]; }
bool InputClass::getGeeWarningButton()          { return _shiftInA[4]; }
bool InputClass::getWarpWarningButton()         { return _shiftInA[5]; }
bool InputClass::getBrakeWarningButton()        { return _shiftInA[6]; }
bool InputClass::getSASWarningButton()          { return _shiftInA[7]; }
bool InputClass::getRCSWarningButton()          { return _shiftInA[8]; }
bool InputClass::getGearWarningButton()         { return _shiftInA[9]; }
bool InputClass::getCommsWarningButton()        { return _shiftInA[10]; }
bool InputClass::getAltWarningButton()          { return _shiftInA[11]; }
bool InputClass::getPitchWarningButton()        { return _shiftInA[12]; }

// Display Controls

bool InputClass::getStageViewSwitch()           { return _shiftInA[13]; }
bool InputClass::getVerticalVelocitySwitch()    { return _shiftInA[14]; }
bool InputClass::getReferenceModeButton()       { return _shiftInA[15]; }
bool InputClass::getRadarAltitudeSwitch()       { return _shiftInA[16]; }
byte InputClass::getInfoMode()
{
    for (int i = 0; i < 12; i++)
    {
        if (_shiftInA[i + 17])
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
        if (_shiftInA[i + 29])
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

bool InputClass::getStageButton()               { return _shiftInA[40]; }
bool InputClass::getStageLockSwitch()           { return _shiftInA[41]; }

// Aborting

bool InputClass::getAbortButton()               { return _shiftInA[42]; }
bool InputClass::getAbortLockSwitch()           { return _shiftInA[43]; }

// Custom Actions Groups

bool InputClass::getCAG1()                      {  }
bool InputClass::getCAG2()                      {  }
bool InputClass::getCAG3()                      {  }
bool InputClass::getCAG4()                      {  }
bool InputClass::getCAG5()                      {  }
bool InputClass::getCAG6()                      {  }
bool InputClass::getCAG7()                      {  }
bool InputClass::getCAG8()                      {  }
bool InputClass::getCAG9()                      {  }
bool InputClass::getCAG10()                     {  }

// Other Action Groups

bool InputClass::getDockingSwitch()             { return _shiftInA[44]; }
bool InputClass::getPercisionSwitch()           { return _shiftInA[45]; }
bool InputClass::getLightsSwitch()              { return _shiftInA[46]; }
bool InputClass::getGearSwitch()                { return _shiftInA[47]; }
bool InputClass::getBrakeSwitch()               { return _shiftInA[48]; }

// View

bool InputClass::getScreenshotButton()          { return _shiftInA[49]; }
bool InputClass::getUISwitch()                  { return _shiftInA[50]; }
bool InputClass::getNavSwitch()                 { return _shiftInA[51]; }
bool InputClass::getViewSwitch()                { return _shiftInA[52]; }
bool InputClass::getFocusButton()               { return _shiftInA[53]; }
bool InputClass::getCamModeButton()             { return _shiftInA[54]; }
bool InputClass::getCamResetButton()            { return _shiftInA[55]; }
bool InputClass::getEnableLookButton()          { return analogRead(TRANSLATION_BUTTON_PIN) > 50 ? false : true; }

// Warping & Pause

bool InputClass::getWarpLockSwitch()            { return _shiftInA[57]; }
bool InputClass::getPhysWarpSwitch()            { return _shiftInA[58]; }
bool InputClass::getCancelWarpButton()          { return _shiftInA[59]; }
bool InputClass::getDecreaseWarpButton()        { return _shiftInA[60]; }
bool InputClass::getIncreaseWarpButton()        { return _shiftInA[61]; }
bool InputClass::getPauseButton()               { return _shiftInA[62]; }

// SAS & RCS

bool InputClass::getSASStabilityAssistButton()  { return _shiftInB[5]; }
bool InputClass::getSASManeuverButton()         { return _shiftInB[6]; }
bool InputClass::getSASProgradeButton()         { return _shiftInB[7]; }
bool InputClass::getSASRetrogradeButton()       { return _shiftInB[8]; }
bool InputClass::getSASNormalButton()           { return _shiftInB[9]; }
bool InputClass::getSASAntiNormalButton()       { return _shiftInB[10]; }
bool InputClass::getSASRadialInButton()         { return _shiftInB[11]; }
bool InputClass::getSASRadialOutButton()        { return _shiftInB[12]; }
bool InputClass::getSASTargetButton()           { return _shiftInB[13]; }
bool InputClass::getSASAntiTargetButton()       { return _shiftInB[14]; }
bool InputClass::getSASSwitch()                 { return _shiftInB[15]; }
bool InputClass::getRCSSwitch()                 { return _shiftInB[16]; }

// EVA Specific Controls

bool InputClass::getBoardButton()               { return _shiftInA[63]; }
bool InputClass::getGrabButton()                { return _shiftInB[0]; }
bool InputClass::getJumpButton()                { return analogRead(ROTATION_BUTTON_PIN) > 50 ? false : true; }

// Throttle

int  InputClass::getThrottleAxis()              { return analogRead(THROTTLE_AXIS_PIN); }
bool InputClass::getThrottleLockSwitch()        { return _shiftInA[56]; }

// Translation

int  InputClass::getTranslationXAxis()          { return analogRead(TRANSLATION_X_AXIS_PIN); }
int  InputClass::getTranslationYAxis()          { return analogRead(TRANSLATION_Y_AXIS_PIN); }
int  InputClass::getTranslationZAxis()          { return analogRead(TRANSLATION_Z_AXIS_PIN); }
bool InputClass::getTransHoldButton()           { return _shiftInB[1]; }
bool InputClass::getTransResetButton()          { return _shiftInB[2]; }

// Rotation

int  InputClass::getRotationXAxis()             { return analogRead(ROTATION_X_AXIS_PIN); }
int  InputClass::getRotationYAxis()             { return analogRead(ROTATION_Y_AXIS_PIN); }
int  InputClass::getRotationZAxis()             { return analogRead(ROTATION_Z_AXIS_PIN); }
bool InputClass::getRotHoldButton()             { return _shiftInB[3]; }
bool InputClass::getRotResetButton()            { return _shiftInB[4]; }

#pragma endregion

InputClass Input;
