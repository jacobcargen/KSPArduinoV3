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



#pragma region Private 

struct VirtualPin
{
    bool* value;
    bool lastReadingState; 
    bool lastDebouncedState;
    bool hasBeenRead;
    unsigned long lastDebounceTime;
    unsigned long debounceDelay;
};

// ARDUINO PINS

// Shift in A pins (8 registers)
const int SHIFT_IN_A_LOAD_PIN = 12;
const int SHIFT_IN_A_CLOCK_PIN = 14;
const int SHIFT_IN_A_CLOCK_ENABLE_PIN = 13;
const int SHIFT_IN_A_SERIAL_PIN = 15;
// Shift in B pins (2 registers)
const int SHIFT_IN_B_LOAD_PIN = 16;
const int SHIFT_IN_B_CLOCK_PIN = 17;
const int SHIFT_IN_B_CLOCK_ENABLE_PIN = 18;
const int SHIFT_IN_B_SERIAL_PIN = 19;
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
const byte TEST_BUTTON = 51;
const byte TEST_SWITCH = 50;

const int ARDUINO_PINS[18] = { 
                              32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
                              42, 43, 44, 45, 46, 47, 48, 49}; 
bool arduinoPins[18]; // States of the arduino pins

// Test states
bool testButton, testSwitch;
// Analog states (Only for boolean analog interpretation)
bool translationButton, rotationButton;
// ShiftIn states
bool _sIA[64];
bool _sIB[16];

// Holds all of the virtual pins
VirtualPin* pins = nullptr;
// Size of pins array (dynamically set)
int numPins = 0;

Stream* debugSerial = nullptr;

/// <summary>Add a new virtual pin. This can be checked using getVirtualPin().</summary>
/// <param name="referenceToBoolVal">Boolean varaible that holds the real state of the input.</param>
/// <param name="virtualPin">What index to create this virtual pin at.</param>
/// <param name="debounce">Debounce for this virtual pin.</param>
void AddInput(bool& referenceToBoolVal, int virtualPin, int debounce = 100)
{
    // If the new pin is outside the current array bounds, we need to resize.
    if (virtualPin >= numPins) {
        int newSize = virtualPin + 1;
        VirtualPin* newPins = new VirtualPin[newSize];

        // Copy existing inputs to the new array
        for (int i = 0; i < numPins; i++) {
            newPins[i] = pins[i];
        }
        
        // Initialize the new elements that are not the one we are adding now
        for (int i = numPins; i < newSize; i++) {
            newPins[i].value = nullptr; // Or some other safe default
            newPins[i].lastReadingState = false;
            newPins[i].lastDebouncedState = false;
            newPins[i].lastDebounceTime = 0;
            newPins[i].debounceDelay = 100;
            newPins[i].hasBeenRead = true;
        }

        // Delete the old array
        delete[] pins;
        
        // Update to the new array and size
        pins = newPins;
        numPins = newSize;
    }

    // Now, we can safely add/overwrite the input at the specified index
    pins[virtualPin].value = &referenceToBoolVal;
    pins[virtualPin].lastReadingState = *pins[virtualPin].value;
    pins[virtualPin].lastDebouncedState = *pins[virtualPin].value;
    pins[virtualPin].lastDebounceTime = millis();
    pins[virtualPin].debounceDelay = debounce;
    pins[virtualPin].hasBeenRead = true;
}

bool shouldReset = false;

ButtonState InputClass::getVirtualPin(int virtualPin, bool waitForChange)
{
    // Validate pin
    if (virtualPin < 0 || virtualPin >= numPins || pins[virtualPin].value == nullptr) 
    {
        if (debugSerial) {
            debugSerial->print("\n\n!!! ---- INVALID PIN: ");
            debugSerial->print(virtualPin);
            if (virtualPin < 0 || virtualPin >= numPins) {
                debugSerial->print(" (out of bounds, numPins=");
                debugSerial->print(numPins);
                debugSerial->print(")");
            } else if (pins[virtualPin].value == nullptr) {
                debugSerial->print(" (null value)");
            }
            debugSerial->print(" ---- !!!\n\n");
        }
        return NOT_READY;
    }
    
    unsigned long now = millis();
    bool currentReading = *pins[virtualPin].value;
	unsigned long elapsed = now - pins[virtualPin].lastDebounceTime;
    bool isDebouncePassed = elapsed > pins[virtualPin].debounceDelay;

    if (currentReading != pins[virtualPin].lastDebouncedState && isDebouncePassed)
    {
        pins[virtualPin].lastDebounceTime = now;
        pins[virtualPin].lastDebouncedState = currentReading;
        pins[virtualPin].hasBeenRead = false;
    }
    if (!waitForChange) 
    {
        return pins[virtualPin].lastDebouncedState ? ON : OFF;
    }
    if (!pins[virtualPin].hasBeenRead) 
    {
    	pins[virtualPin].hasBeenRead = true;
    	return pins[virtualPin].lastDebouncedState ? ON : OFF; 
    }
    return NOT_READY;
}

void initVirtualPins()
{
    const int totalPins = 102; // 84+18 = 102
     // Clean up existing pins array if it exists
    if (pins != nullptr) {
        delete[] pins;
    }
    pins = new VirtualPin[totalPins];
    numPins = totalPins;

    for (int i = 0; i < 64; i++)
    {
        AddInput(_sIA[i], i, 150);
    }
    for (int i = 0; i < 16; i++)
    {
        AddInput(_sIB[i], i + 64, 150);
    }

    AddInput(testButton, 80, 150);
    AddInput(testSwitch, 81, 150);
    AddInput(translationButton, 82, 150);
    AddInput(rotationButton, 83, 150);
    for (int i = 0; i < 18; i++)
    {
        AddInput(arduinoPins[i], i + 84, 150);
    }
}
/// <summary>Gets shift register inputs.</summary>
void _shiftIn(int dataA, int clockEnableA, int clockA, int loadA,
              int dataB, int clockEnableB, int clockB, int loadB)
{
    // Clear values first
    for (size_t i = 0; i < 64; i++) {
        _sIA[i] = 0;
    }
    for (size_t i = 0; i < 16; i++) {
        _sIB[i] = 0;
    }
    // Then read and set values

    // Pulse to A
    digitalWrite(loadA, LOW);
    delayMicroseconds(5);
    digitalWrite(loadA, HIGH);
    delayMicroseconds(5);
    byte inputA[8];
    // Get input A data
    digitalWrite(clockA, HIGH);
    digitalWrite(clockEnableA, LOW);
    inputA[0] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[1] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[2] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[3] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[4] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[5] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[6] = shiftIn(dataA, clockA, LSBFIRST);
    inputA[7] = shiftIn(dataA, clockA, LSBFIRST);
    digitalWrite(clockEnableA, HIGH);

    for (size_t i = 0; i < 64; i++)
    {
        byte byteIndex = i / 8;
        byte bitIndex = 7 - (i % 8);
        
        if (byteIndex < 8 && bitRead(inputA[byteIndex], bitIndex)) {
            _sIA[i] = 1;
        }
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
    inputB[0] = shiftIn(dataB, clockB, LSBFIRST);
    inputB[1] = shiftIn(dataB, clockB, LSBFIRST);
    digitalWrite(clockEnableB, HIGH);

    for (size_t i = 0; i < 16; i++)
    {
        byte byteIndex = i / 8;
        byte bitIndex = i % 8;
        
        if (byteIndex < 2 && bitRead(inputB[byteIndex], bitIndex)) {
            _sIB[i] = 1;
        }
    }
}

#pragma endregion


#pragma region Public

void InputClass::init(Stream& serial)
{
    debugSerial = &serial;  // Store Serial reference
    
    // Set pin modes
    pinMode(SHIFT_IN_A_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_A_SERIAL_PIN, INPUT);

    pinMode(SHIFT_IN_B_LOAD_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_CLOCK_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_CLOCK_ENABLE_PIN, OUTPUT);
    pinMode(SHIFT_IN_B_SERIAL_PIN, INPUT);
    
    for (auto pin : ARDUINO_PINS)
    {
        pinMode(pin, INPUT_PULLUP);
    }
	
    // Set initial states
    digitalWrite(SHIFT_IN_A_LOAD_PIN, HIGH);
    digitalWrite(SHIFT_IN_A_CLOCK_PIN, LOW);
    digitalWrite(SHIFT_IN_A_CLOCK_ENABLE_PIN, HIGH);

    digitalWrite(SHIFT_IN_B_LOAD_PIN, HIGH);
    digitalWrite(SHIFT_IN_B_CLOCK_PIN, LOW);
    digitalWrite(SHIFT_IN_B_CLOCK_ENABLE_PIN, HIGH);
    
    // Initialize virtual pins
    initVirtualPins();
    
    debugSerial->println("Input.cpp initialized.");
}

void InputClass::update()
{
    _shiftIn(SHIFT_IN_A_SERIAL_PIN, SHIFT_IN_A_CLOCK_ENABLE_PIN, SHIFT_IN_A_CLOCK_PIN, SHIFT_IN_A_LOAD_PIN,
            SHIFT_IN_B_SERIAL_PIN, SHIFT_IN_B_CLOCK_ENABLE_PIN, SHIFT_IN_B_CLOCK_PIN, SHIFT_IN_B_LOAD_PIN);

    // Arduino Digital Pin reading
    testButton = digitalRead(TEST_BUTTON);
    testSwitch = digitalRead(TEST_SWITCH);
    for (int i = 0; i < 18; i++)
    {
        arduinoPins[i] = digitalRead(ARDUINO_PINS[i]);
    }

    // Arduino Analog reading (Only for boolean analog interpretation)

    translationButton = analogRead(TRANSLATION_BUTTON_PIN) > 50 ? false : true;
    rotationButton = analogRead(ROTATION_BUTTON_PIN) > 50 ? false : true;
}

void InputClass::setAllVPinsReady()
{
    for (int i = 0; i < numPins; i++)
    {
        pins[i].lastReadingState = *pins[i].value; // ADDED: keep aligned
        pins[i].lastDebouncedState = *pins[i].value;
        pins[i].lastDebounceTime = millis();
        pins[i].hasBeenRead = true;
    }
}

/*
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
*/

// Throttle

int  InputClass::getThrottleAxis()               
{
    return analogRead(THROTTLE_AXIS_PIN); 
}

// Translation

int  InputClass::getTranslationXAxis()           
{ 
    return analogRead(TRANSLATION_X_AXIS_PIN); 
}
int  InputClass::getTranslationYAxis()           
{
    return analogRead(TRANSLATION_Y_AXIS_PIN); 
}
int  InputClass::getTranslationZAxis()           
{ 
    return analogRead(TRANSLATION_Z_AXIS_PIN); 
}

// Rotation

int  InputClass::getRotationXAxis()             
{ 
    return analogRead(ROTATION_X_AXIS_PIN); 
}
int  InputClass::getRotationYAxis()              
{
    return analogRead(ROTATION_Y_AXIS_PIN); 
}
int  InputClass::getRotationZAxis()              
{ 
    return analogRead(ROTATION_Z_AXIS_PIN); 
}

// Debugging

void InputClass::debugInputState(int virtualPinNumber) {
    if (!debugSerial) return;  // Safety check
    
    if (virtualPinNumber >= numPins || virtualPinNumber < 0) {
        debugSerial->print("Pin ");
        debugSerial->print(virtualPinNumber);
        debugSerial->println(": Invalid pin");
        return;
    }
    
    ButtonState state = getVirtualPin(virtualPinNumber, false);
    debugSerial->println(state == ON ? "ON" : state == OFF ? "OFF" : "NOT_READY");
}

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


