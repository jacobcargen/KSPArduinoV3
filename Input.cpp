/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Input.h"
#include "Output.h"
#include <pins_arduino.h> 
#include <variant.h>


struct InputPin
{
    bool* value;
    bool lastState;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay;
};


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
// Test pins
const byte TEST_BUTTON = 52;
const byte TEST_SWITCH = 50;


// Test states
bool testButton, testSwitch;
// ShiftIn states
bool _sIA[64];
bool _sIB[16];


bool isDebug = false;




InputPin* pins;
int numPins;





void AddInput(bool& referenceToBoolVal, int virtualPin)
{
    InputPin* newPins = new InputPin[numPins + 1];

    // Copy existing inputs to the new array
    for (int i = 0; i < numPins; i++)
    {
        newPins[i] = pins[i];
    }

    // Add the new input
    newPins[numPins].value = &referenceToBoolVal;
    newPins[numPins].lastState = *newPins[numPins].value;
    newPins[numPins].lastDebounceTime = 0;
    newPins[numPins].debounceDelay = 50; // You can adjust the debounce delay if needed

    // Delete the old array and update to the new array
    delete[] pins;
    pins = newPins;

    numPins++;
}

ButtonState getVirtualPinState(int virtualPin)
{
    for (int i = 0; i < numPins; i++)
    {
        if (i == virtualPin)
        {
            bool inputReading = *pins[i].value;

            if (inputReading != pins[i].lastState && millis() - pins[i].lastDebounceTime > pins[i].debounceDelay)
            {
                pins[i].lastState = inputReading;
                pins[i].lastDebounceTime = millis();
                return pins[i].lastState ? ON : OFF;
            }

            return NOT_READY;
        }
    }

    // Return NOT_READY if the virtual pin is not found
    return NOT_READY;
}

void initVirtualPins()
{
    for (int i = 0; i < 82; i++)
    {
        if (i < 64)
            AddInput(_sIA[i], i);
        else if (i < 80)
            AddInput(_sIB[i - 64], i);
        else
        {
            AddInput(testButton, 80);
            AddInput(testSwitch, 81);
            break;
        }
    }


}

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
                _sIA[i] = 1;
            else if (i < 16)
                if (1 == bitRead(inputA[1], i - 8))
                    _sIA[i] = 1;
                else if (i < 24)
                    if (1 == bitRead(inputA[2], i - 16))
                        _sIA[i] = 1;
                    else if (i < 32)
                        if (1 == bitRead(inputA[3], i - 24))
                            _sIA[i] = 1;
                        else if (i < 40)
                            if (1 == bitRead(inputA[4], i - 32))
                                _sIA[i] = 1;
                            else if (i < 48)
                                if (1 == bitRead(inputA[5], i - 40))
                                    _sIA[i] = 1;
                                else if (i < 56)
                                    if (1 == bitRead(inputA[6], i - 48))
                                        _sIA[i] = 1;
                                    else
                                        if (1 == bitRead(inputA[7], i - 56))
                                            _sIA[i] = 1;
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
                _sIB[i] = 1;
            else
                if (1 == bitRead(inputB[1], i - 8))
                    _sIB[i] = 1;
    }
}

#pragma endregion


#pragma region Public


void InputClass::init()
{
    
    for (int i = 0; i < 64; i++)
    {
        _sIA[i] = 0;
        if (i < 16)
            _sIB[i] = 0;
    }
    // Shift register pins
    pinMode(SHIFT_IN_A_SERIAL_PIN, INPUT);
    pinMode(SHIFT_IN_A_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_SERIAL_PIN, INPUT);
    pinMode(SHIFT_IN_B_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_LOAD_PIN, OUTPUT);


    pinMode(TEST_SWITCH, INPUT_PULLUP);
    pinMode(TEST_BUTTON, INPUT_PULLUP);

    initVirtualPins();


}
void InputClass::update()
{
    //_shiftIn(SHIFT_IN_A_SERIAL_PIN, SHIFT_IN_A_CLOCK_ENABLE_PIN, SHIFT_IN_A_CLOCK_PIN, SHIFT_IN_A_LOAD_PIN,
            //SHIFT_IN_B_SERIAL_PIN, SHIFT_IN_B_CLOCK_ENABLE_PIN, SHIFT_IN_B_CLOCK_PIN, SHIFT_IN_B_LOAD_PIN);

    testButton = digitalRead(TEST_BUTTON);
    testSwitch = digitalRead(TEST_SWITCH);
    Output.setPowerLED(pins[81].value);
    


    // reading states

    for (int i = 0; i < numPins; i++)
    {
        bool inputReading = *pins[i].value;

        if (inputReading != pins[i].lastState && millis() - pins[i].lastDebounceTime > pins[i].debounceDelay)
        {
            pins[i].lastState = inputReading;
            pins[i].lastDebounceTime = millis();
        }
    }

    // Analog updated on call (to be as fast as possible)
}

// Testing

byte InputClass::getTestButton()                 { return getVirtualPinState(80); }
byte InputClass::getTestSwitch()                 { return getVirtualPinState(81); }

// Miscellaneous

byte InputClass::getDebugSwitch()               { return getVirtualPinState(0); }
byte InputClass::getSoundSwitch()               { return getVirtualPinState(1); }
byte InputClass::getInputEnableButton()         { return getVirtualPinState(2); }

// Warnings

byte InputClass::getTempWarningButton()         { return getVirtualPinState(3); }
byte InputClass::getGeeWarningButton()          { return getVirtualPinState(4); }
byte InputClass::getWarpWarningButton()         { return getVirtualPinState(5); }
byte InputClass::getBrakeWarningButton()        { return getVirtualPinState(6); }
byte InputClass::getSASWarningButton()          { return getVirtualPinState(7); }
byte InputClass::getRCSWarningButton()          { return getVirtualPinState(8); }
byte InputClass::getGearWarningButton()         { return getVirtualPinState(9); }
byte InputClass::getCommsWarningButton()        { return getVirtualPinState(10); }
byte InputClass::getAltWarningButton()          { return getVirtualPinState(11); }
byte InputClass::getPitchWarningButton()        { return getVirtualPinState(12); }

// Display Controls

byte InputClass::getStageViewSwitch()           { return getVirtualPinState(13); }
byte InputClass::getVerticalVelocitySwitch()    { return getVirtualPinState(14); }
byte InputClass::getReferenceModeButton()       { return getVirtualPinState(15); }
byte InputClass::getRadarAltitudeSwitch()       { return getVirtualPinState(16); }

byte InputClass::getInfoMode()
{
    for (int i = 1; i < 13; i++)
    {
        if (getVirtualPinState(i + 17))
        {
            return i; // Return index
        }
        else
        {
            // ERROR, should return true
        }
    }
    return NOT_READY; // Add a default return value
}

byte InputClass::getDirectionMode()
{
    for (int i = 1; i < 13; i++)
    {
        if (getVirtualPinState(i + 29))
        {
            return i; // Return index
        }
        else
        {
            // ERROR, should return true
        }
    }
    return NOT_READY; // Add a default return value
}

// Staging

byte InputClass::getStageButton()                { return getVirtualPinState(17); }
byte InputClass::getStageLockSwitch()            { return getVirtualPinState(18); }

// Aborting

byte InputClass::getAbortButton()                { return getVirtualPinState(19); }
byte InputClass::getAbortLockSwitch()            { return getVirtualPinState(20); }

// Custom Action Groups

byte InputClass::getCAG1()                       { return getVirtualPinState(21); }
byte InputClass::getCAG2()                       { return getVirtualPinState(22); }
byte InputClass::getCAG3()                       { return getVirtualPinState(23); }
byte InputClass::getCAG4()                       { return getVirtualPinState(24); }
byte InputClass::getCAG5()                       { return getVirtualPinState(25); }
byte InputClass::getCAG6()                       { return getVirtualPinState(26); }
byte InputClass::getCAG7()                       { return getVirtualPinState(27); }
byte InputClass::getCAG8()                       { return getVirtualPinState(28); }
byte InputClass::getCAG9()                       { return getVirtualPinState(29); }
byte InputClass::getCAG10()                      { return getVirtualPinState(30); }

// Other Action Groups

byte InputClass::getDockingSwitch()              { return getVirtualPinState(31); }
byte InputClass::getPercisionSwitch()            { return getVirtualPinState(32); }
byte InputClass::getLightsSwitch()               { return getVirtualPinState(33); }
byte InputClass::getGearSwitch()                 { return getVirtualPinState(34); }
byte InputClass::getBrakeSwitch()                { return getVirtualPinState(35); }

// View

byte InputClass::getScreenshotButton()           { return getVirtualPinState(36); }
byte InputClass::getUISwitch()                   { return getVirtualPinState(37); }
byte InputClass::getNavSwitch()                  { return getVirtualPinState(38); }
byte InputClass::getViewSwitch()                 { return getVirtualPinState(39); }
byte InputClass::getFocusButton()                { return getVirtualPinState(40); }
byte InputClass::getCamModeButton()              { return getVirtualPinState(41); }
byte InputClass::getCamResetButton()             { return getVirtualPinState(42); }
byte InputClass::getEnableLookButton()           { return analogRead(TRANSLATION_BUTTON_PIN) > 50 ? false : true; } //Move this logic into a state

// Warping & Pause

byte InputClass::getWarpLockSwitch()             { return getVirtualPinState(43); }
byte InputClass::getPhysWarpSwitch()             { return getVirtualPinState(44); }
byte InputClass::getCancelWarpButton()           { return getVirtualPinState(45); }
byte InputClass::getDecreaseWarpButton()         { return getVirtualPinState(46); }
byte InputClass::getIncreaseWarpButton()         { return getVirtualPinState(47); }
byte InputClass::getPauseButton()                { return getVirtualPinState(48); }

// SAS & RCS

byte InputClass::getSASStabilityAssistButton()   { return getVirtualPinState(49); }
byte InputClass::getSASManeuverButton()          { return getVirtualPinState(50); }
byte InputClass::getSASProgradeButton()          { return getVirtualPinState(51); }
byte InputClass::getSASRetrogradeButton()        { return getVirtualPinState(52); }
byte InputClass::getSASNormalButton()            { return getVirtualPinState(53); }
byte InputClass::getSASAntiNormalButton()        { return getVirtualPinState(54); }
byte InputClass::getSASRadialInButton()          { return getVirtualPinState(55); }
byte InputClass::getSASRadialOutButton()         { return getVirtualPinState(56); }
byte InputClass::getSASTargetButton()            { return getVirtualPinState(57); }
byte InputClass::getSASAntiTargetButton()        { return getVirtualPinState(58); }
byte InputClass::getSASSwitch()                  { return getVirtualPinState(59); }
byte InputClass::getRCSSwitch()                  { return getVirtualPinState(60); }

// EVA Specific Controls

byte InputClass::getBoardButton()                { return getVirtualPinState(61); }
byte InputClass::getGrabButton()                 { return getVirtualPinState(62); }
byte InputClass::getJumpButton()                 { return analogRead(ROTATION_BUTTON_PIN) > 50 ? false : true; } //Move this logic into a state

// Throttle

int  InputClass::getThrottleAxis()                      { return analogRead(THROTTLE_AXIS_PIN); }
byte InputClass::getThrottleLockSwitch()         { return getVirtualPinState(63); }

// Translation

int  InputClass::getTranslationXAxis()                  { return analogRead(TRANSLATION_X_AXIS_PIN); }
int  InputClass::getTranslationYAxis()                  { return analogRead(TRANSLATION_Y_AXIS_PIN); }
int  InputClass::getTranslationZAxis()                  { return analogRead(TRANSLATION_Z_AXIS_PIN); }
byte InputClass::getTransHoldButton()            { return getVirtualPinState(64); }
byte InputClass::getTransResetButton()           { return getVirtualPinState(65); }

// Rotation

int  InputClass::getRotationXAxis()                     { return analogRead(ROTATION_X_AXIS_PIN); }
int  InputClass::getRotationYAxis()                     { return analogRead(ROTATION_Y_AXIS_PIN); }
int  InputClass::getRotationZAxis()                     { return analogRead(ROTATION_Z_AXIS_PIN); }
byte InputClass::getRotHoldButton()              { return getVirtualPinState(66); }
byte InputClass::getRotResetButton()             { return getVirtualPinState(67); }

#pragma endregion


InputClass::~InputClass()
{
    if (pins != nullptr)
    {
        delete[] pins;
        pins = nullptr; // Set to nullptr after deletion to avoid double deletion
    }
}


InputClass Input;


