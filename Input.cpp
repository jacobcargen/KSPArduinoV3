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
    bool lastState;
    bool lastReadingState; // ADDED: Tracks the raw reading to correctly detect changes
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
            newPins[i].lastState = false;
            newPins[i].lastReadingState = false;
            newPins[i].lastDebounceTime = 0;
            newPins[i].debounceDelay = 100;
        }

        // Delete the old array
        delete[] pins;
        
        // Update to the new array and size
        pins = newPins;
        numPins = newSize;
    }

    // Now, we can safely add/overwrite the input at the specified index
    pins[virtualPin].value = &referenceToBoolVal;
    pins[virtualPin].lastState = *pins[virtualPin].value;
    pins[virtualPin].lastReadingState = *pins[virtualPin].value;
    pins[virtualPin].lastDebounceTime = 0;
    pins[virtualPin].debounceDelay = debounce;
}
/// <summary>Read a virtual pin. These are added with AddInput() and stored in pins array.</summary>
/// <param name="virtualPin"></param>
/// <returns>This returns the virtual pin state at index, virtual pin.</returns>
ButtonState InputClass::getVirtualPin(int virtualPin, bool waitForChange)
{
    bool isDebug = false;
   
    if (virtualPin < 0 || virtualPin >= numPins || pins[virtualPin].value == nullptr) {
        if (debugSerial && isDebug) {
            debugSerial->println("Error: Virtual pin " + String(virtualPin) + " is out of range or not initialized.");
        }
        return NOT_READY;
    }

    if (debugSerial && isDebug) {
        debugSerial->print("Processing virtual pin ");
        debugSerial->println(virtualPin);
    }

    bool currentReading = *pins[virtualPin].value;
    unsigned long currentTime = millis();

    if (debugSerial && isDebug) {
        debugSerial->print("Current reading: ");
        debugSerial->println(currentReading ? "HIGH" : "LOW");
    }

    // Detect if the raw input has changed since the last loop
    if (currentReading != pins[virtualPin].lastReadingState) {
        // If it changed, reset the debounce timer
        pins[virtualPin].lastDebounceTime = currentTime;
        if (debugSerial && isDebug) {
            debugSerial->println("Raw input changed, resetting debounce timer");
        }
    }

    // Update the last raw reading for the next loop
    pins[virtualPin].lastReadingState = currentReading;

    // After the debounce delay has passed...
    if ((currentTime - pins[virtualPin].lastDebounceTime) > pins[virtualPin].debounceDelay) {
        if (debugSerial && isDebug) {
            debugSerial->println("Debounce delay passed");
        }
        // ...the reading is now stable. If the stable state needs updating, do it.
        if (pins[virtualPin].lastState != currentReading) {
            pins[virtualPin].lastState = currentReading;
            if (debugSerial && isDebug) {
                debugSerial->println("Stable state updated");
            }

            // If in waitForChange mode, return the new state now
            if (waitForChange) {
                if (debugSerial && isDebug) {
                    debugSerial->print("WaitForChange mode, returning ");
                    debugSerial->println(pins[virtualPin].lastState ? "ON" : "OFF");
                }
                return pins[virtualPin].lastState ? ON : OFF;
            }
        }
    }

    // For waitForChange mode, if we're here, it means no debounced change happened
    if (waitForChange) {
        if (debugSerial && isDebug) {
            debugSerial->println("WaitForChange mode, no change, returning NOT_READY");
        }
        return NOT_READY;
    }

    // For normal mode, always return the last known stable state
    if (debugSerial && isDebug) {
        debugSerial->print("Normal mode, returning ");
        debugSerial->println(pins[virtualPin].lastState ? "ON" : "OFF");
    }
    return pins[virtualPin].lastState ? ON : OFF;
}
/// <summary>Initialize all of the virtual pins.</summary>
void initVirtualPins()
{
    // Calculate total number of pins and allocate memory once
    const int totalPins = 84;
    if (pins != nullptr) {
        delete[] pins;
    }
    pins = new VirtualPin[totalPins];
    numPins = totalPins;

    for (int i = 0; i < 64; i++)
    {
        AddInput(_sIA[i], i, 10);
    }
    for (int i = 0; i < 16; i++)
    {
        AddInput(_sIB[i], i + 64, 25);
    }
    AddInput(testButton, 80, 50);
    AddInput(testSwitch, 81, 50);
    AddInput(translationButton, 82, 50);
    AddInput(rotationButton, 83, 50);
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
    inputA[0] = shiftIn(dataA, clockA, LSBFIRST);  // Changed from MSBFIRST
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
        byte bitIndex = 7 - (i % 8);  // Reverse the bit order for MSBFIRST
        
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
        byte bitIndex = 7 - (i % 8);
        
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
    
    // Set initial states
    digitalWrite(SHIFT_IN_A_LOAD_PIN, HIGH);
    digitalWrite(SHIFT_IN_A_CLOCK_PIN, LOW);
    digitalWrite(SHIFT_IN_A_CLOCK_ENABLE_PIN, HIGH);
    
    // Initialize virtual pins
    initVirtualPins();
    
    debugSerial->println("Input system initialized");
}

void InputClass::update()
{
    _shiftIn(SHIFT_IN_A_SERIAL_PIN, SHIFT_IN_A_CLOCK_ENABLE_PIN, SHIFT_IN_A_CLOCK_PIN, SHIFT_IN_A_LOAD_PIN,
            SHIFT_IN_B_SERIAL_PIN, SHIFT_IN_B_CLOCK_ENABLE_PIN, SHIFT_IN_B_CLOCK_PIN, SHIFT_IN_B_LOAD_PIN);

    // Arduino Digital Pin reading
    testButton = digitalRead(TEST_BUTTON);
    testSwitch = digitalRead(TEST_SWITCH);

    // Arduino Analog reading (Only for boolean analog interpretation)

    translationButton = analogRead(TRANSLATION_BUTTON_PIN) > 50 ? false : true;
    rotationButton = analogRead(ROTATION_BUTTON_PIN) > 50 ? false : true;
}

void InputClass::setAllVPinsReady()
{
    for (int i = 0; i < numPins; i++)
    {
        pins[i].lastState = *pins[i].value;
        pins[i].lastDebounceTime = millis();
    }
}

// Testing

byte InputClass::getTestButton(bool waitForChange)                
{ 
    return getVirtualPin(80, waitForChange); 
}
byte InputClass::getTestSwitch(bool waitForChange)
{ 
    return getVirtualPin(81, waitForChange); 
}

// Miscellaneous

byte InputClass::getDebugSwitch(bool waitForChange)               
{
    return getVirtualPin(0, waitForChange);  // Assuming debug switch is pin 0
}
byte InputClass::getSoundSwitch(bool waitForChange)               
{ 
    return getVirtualPin(1, waitForChange);
}
byte InputClass::getInputEnableButton(bool waitForChange)         
{ 
    return getVirtualPin(2, waitForChange);
}

// Warnings

byte InputClass::getTempWarningButton(bool waitForChange)         
{ 
    return getVirtualPin(3, waitForChange);
}
byte InputClass::getGeeWarningButton(bool waitForChange)          
{ 
    return getVirtualPin(4, waitForChange);
}
byte InputClass::getWarpWarningButton(bool waitForChange)         
{ 
    return getVirtualPin(5, waitForChange);
}
byte InputClass::getBrakeWarningButton(bool waitForChange)        
{ 
    return getVirtualPin(6, waitForChange);
}
byte InputClass::getSASWarningButton(bool waitForChange)          
{ 
    return getVirtualPin(7, waitForChange);
}
byte InputClass::getRCSWarningButton(bool waitForChange)          
{ 
    return getVirtualPin(8, waitForChange);
}
byte InputClass::getGearWarningButton(bool waitForChange)         
{ 
    return getVirtualPin(9, waitForChange);
}
byte InputClass::getCommsWarningButton(bool waitForChange)        
{ 
    return getVirtualPin(10, waitForChange);
}
byte InputClass::getAltWarningButton(bool waitForChange)          
{ 
    return getVirtualPin(11, waitForChange);
}
byte InputClass::getPitchWarningButton(bool waitForChange)        
{ 
    return getVirtualPin(12, waitForChange);
}

// Display Controls

byte InputClass::getStageViewSwitch(bool waitForChange)           
{ 
return getVirtualPin(13, waitForChange);
}
byte InputClass::getVerticalVelocitySwitch(bool waitForChange)    
{ 
return getVirtualPin(14, waitForChange);
}
byte InputClass::getReferenceModeButton(bool waitForChange)       
{ 
return getVirtualPin(15, waitForChange);
}
byte InputClass::getRadarAltitudeSwitch(bool waitForChange)       
{ 
    return getVirtualPin(16, waitForChange);
}

byte InputClass::getInfoMode()
{
    /*
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
    */
    return NOT_READY; // Add a default return value
}

byte InputClass::getDirectionMode()
{
    /*
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
    */
    return NOT_READY; // Add a default return value
}

// Staging

byte InputClass::getStageButton(bool waitForChange)                
{ 
    return getVirtualPin(17, waitForChange);
}
byte InputClass::getStageLockSwitch(bool waitForChange)            
{ 
    return getVirtualPin(18, waitForChange);
}

// Aborting

byte InputClass::getAbortButton(bool waitForChange)                
{ 
    return getVirtualPin(19, waitForChange); 
}
byte InputClass::getAbortLockSwitch(bool waitForChange)            
{ 
    return getVirtualPin(20, waitForChange);
}

// Custom Action Groups

byte InputClass::getCAG1(bool waitForChange)                       
{ 
    return getVirtualPin(21, waitForChange); 
}
byte InputClass::getCAG2(bool waitForChange)                       
{ 
    return getVirtualPin(22, waitForChange); 
}
byte InputClass::getCAG3(bool waitForChange)                       
{ 
    return getVirtualPin(23, waitForChange); 
}
byte InputClass::getCAG4(bool waitForChange)                       
{ 
    return getVirtualPin(24, waitForChange); 
}
byte InputClass::getCAG5(bool waitForChange)                       
{ 
    return getVirtualPin(25, waitForChange); 
}
byte InputClass::getCAG6(bool waitForChange)                       
{ 
    return getVirtualPin(26, waitForChange); 
}
byte InputClass::getCAG7(bool waitForChange)                       
{ 
    return getVirtualPin(27, waitForChange); 
}
byte InputClass::getCAG8(bool waitForChange)                       
{ 
    return getVirtualPin(28, waitForChange); 
}
byte InputClass::getCAG9(bool waitForChange)                       
{ 
    return getVirtualPin(29, waitForChange); 
}
byte InputClass::getCAG10(bool waitForChange)                      
{ 
    return getVirtualPin(30, waitForChange); 
}

// Other Action Groups

byte InputClass::getDockingSwitch(bool waitForChange)              
{   
    return getVirtualPin(31, waitForChange); 
}
byte InputClass::getPercisionSwitch(bool waitForChange)            
{   
    return getVirtualPin(32, waitForChange); 
}
byte InputClass::getLightsSwitch(bool waitForChange)               
{   
    return getVirtualPin(33, waitForChange); 
}
byte InputClass::getGearSwitch(bool waitForChange)                 
{   
    return getVirtualPin(34, waitForChange); 
}
byte InputClass::getBrakeSwitch(bool waitForChange)                
{   
    return getVirtualPin(35, waitForChange); 
}

// View

byte InputClass::getScreenshotButton(bool waitForChange)           
{ 
    return getVirtualPin(36, waitForChange); 
}
byte InputClass::getUISwitch(bool waitForChange)                   
{ 
    return getVirtualPin(37, waitForChange); 
}
byte InputClass::getNavSwitch(bool waitForChange)                  
{ 
    return getVirtualPin(38, waitForChange); 
}
byte InputClass::getViewSwitch(bool waitForChange)                 
{ 
    return getVirtualPin(39, waitForChange); 
}
byte InputClass::getFocusButton(bool waitForChange)                
{ 
    return getVirtualPin(40, waitForChange); 
}
byte InputClass::getCamModeButton(bool waitForChange)              
{ 
    return getVirtualPin(41, waitForChange); 
}
byte InputClass::getCamResetButton(bool waitForChange)             
{ 
    return getVirtualPin(42, waitForChange); 
}
byte InputClass::getEnableLookButton(bool waitForChange)           
{ 
    return getVirtualPin(83, waitForChange); 
}

// Warping & Pause

byte InputClass::getWarpLockSwitch(bool waitForChange)             
{ 
    return getVirtualPin(43, waitForChange); 
}
byte InputClass::getPhysWarpSwitch(bool waitForChange)             
{ 
    return getVirtualPin(44, waitForChange); 
}
byte InputClass::getCancelWarpButton(bool waitForChange)           
{ 
    return getVirtualPin(45, waitForChange); 
}
byte InputClass::getDecreaseWarpButton(bool waitForChange)         
{ 
    return getVirtualPin(46, waitForChange); 
}
byte InputClass::getIncreaseWarpButton(bool waitForChange)         
{ 
    return getVirtualPin(47, waitForChange); 
}
byte InputClass::getPauseButton(bool waitForChange)                
{ 
    return getVirtualPin(48, waitForChange); 
}

// SAS & RCS

byte InputClass::getSASStabilityAssistButton(bool waitForChange)    
{ 
    return getVirtualPin(49, waitForChange); 
}
byte InputClass::getSASManeuverButton(bool waitForChange)           
{ 
    return getVirtualPin(50, waitForChange); 
}
byte InputClass::getSASProgradeButton(bool waitForChange)           
{ 
    return getVirtualPin(51, waitForChange); 
}
byte InputClass::getSASRetrogradeButton(bool waitForChange)         
{ 
    return getVirtualPin(52, waitForChange); 
}
byte InputClass::getSASNormalButton(bool waitForChange)             
{ 
    return getVirtualPin(53, waitForChange); 
}
byte InputClass::getSASAntiNormalButton(bool waitForChange)         
{ 
    return getVirtualPin(54, waitForChange); 
}
byte InputClass::getSASRadialInButton(bool waitForChange)           
{ 
    return getVirtualPin(55, waitForChange); 
}
byte InputClass::getSASRadialOutButton(bool waitForChange)          
{ 
    return getVirtualPin(56, waitForChange); 
}
byte InputClass::getSASTargetButton(bool waitForChange)             
{ 
    return getVirtualPin(57, waitForChange); 
}
byte InputClass::getSASAntiTargetButton(bool waitForChange)         
{ 
    return getVirtualPin(58, waitForChange); 
}
byte InputClass::getSASSwitch(bool waitForChange)                   
{ 
    return getVirtualPin(59, waitForChange); 
}
byte InputClass::getRCSSwitch(bool waitForChange)                   
{ 
    return getVirtualPin(60, waitForChange); 
}

// EVA Specific Controls

byte InputClass::getBoardButton(bool waitForChange)                
{
    return getVirtualPin(61, waitForChange); 
}
byte InputClass::getGrabButton(bool waitForChange)                 
{
    return getVirtualPin(62, waitForChange); 
}
byte InputClass::getJumpButton(bool waitForChange)                 
{ 
    return getVirtualPin(82, waitForChange); 
} //Move this logic into a state

// Throttle

int  InputClass::getThrottleAxis()               
{
    return analogRead(THROTTLE_AXIS_PIN); 
}
byte InputClass::getThrottleLockSwitch(bool waitForChange)         
{
    return getVirtualPin(63, waitForChange); 
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
byte InputClass::getTransHoldButton(bool waitForChange)            
{ 
    return getVirtualPin(64, waitForChange); 
}
byte InputClass::getTransResetButton(bool waitForChange)           
{ 
    return getVirtualPin(65, waitForChange); 
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
byte InputClass::getRotHoldButton(bool waitForChange)            
{ 
    return getVirtualPin(66, waitForChange); 
}
byte InputClass::getRotResetButton(bool waitForChange)       
{ 
    return getVirtualPin(67, waitForChange); 
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
    
    debugSerial->print("Pin ");
    debugSerial->print(virtualPinNumber);
    debugSerial->print(": Raw=");
    debugSerial->print(*pins[virtualPinNumber].value ? "HIGH" : "LOW");
    debugSerial->print(", LastState=");
    debugSerial->print(pins[virtualPinNumber].lastState ? "HIGH" : "LOW");
    debugSerial->print(", Time since change=");
    debugSerial->print(millis() - pins[virtualPinNumber].lastDebounceTime);
    debugSerial->print("ms, Debounce delay=");
    debugSerial->print(pins[virtualPinNumber].debounceDelay);
    debugSerial->print("ms, Result=");
    
    ButtonState state = getVirtualPin(virtualPinNumber, false);
    debugSerial->println(state == ON ? "ON" : state == OFF ? "OFF" : "NOT_READY");
}

void InputClass::debugSASWarningButton() {
    if (debugSerial) {
        debugSerial->println("SAS Warning Button (Pin 7):");
    }
    debugInputState(7);
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


