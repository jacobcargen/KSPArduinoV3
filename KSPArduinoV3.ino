/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Wire.h"
#include "Output.h"
#include "Input.h"
#include <PayloadStructs.h>
#include <KerbalSimpitMessageTypes.h>
#include <KerbalSimpit.h>
#include <LiquidCrystal_I2C.h>


#define SOUND_PIN 11

class Timer
{
private:

    unsigned long previousMillis = 0;
    unsigned long delayMillis = 0;

public:

    void start(unsigned long delayMillis)
    {
        this->delayMillis = delayMillis;
        previousMillis = millis();
    }
    // Check 
    bool check() // Check time and reset if ready
    {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= delayMillis) {  
            previousMillis = currentMillis;
            return true;
        }
        return false;
    }

};


class BeepSound {
    
private:

    Timer _timer;

    bool _soundEnabled = true;
    bool _isBeeping = false;

    int* _pattern = nullptr;
    int _patternLength = 0;
    int _patternIndex = 0;
    int _beepInterval = 0;

    

public:
    BeepSound() {}

    // Mute/unmute this sound instance
    void muteSound(bool isMute) {
        _soundEnabled = !isMute;
        if (isMute) {
            Output.setSound(0, false);
        }
    }

    // Start or stop a custom beep pattern
    void beep(int* pattern, int patternLength, bool isEnabled, int beepRateMs) {
        if (!_soundEnabled) return;

        if (isEnabled) {
            _pattern = pattern;
            _patternLength = patternLength;
            _isBeeping = true;
            _beepInterval = beepRateMs;
            _patternIndex = 0;
            _timer.start(_beepInterval);
        } else {
            _isBeeping = false;
            setSound(0, false);
        }
    }

    // Non-blocking update function
    void updateBeep() {
        if (!_soundEnabled || !_isBeeping || _pattern == nullptr) return;

        // When timer elapses, move to next tone
        if (_timer.check()) {
            int freq = _pattern[_patternIndex];
            setSound(freq, (freq > 0));

            _patternIndex++;
            if (_patternIndex >= _patternLength) {
                _patternIndex = 0; // Loop pattern
            }

            _timer.start(_beepInterval);
        }
    }

    void setSound(int frequency, bool enabled)
    {
        if (enabled && frequency > 0) {
            // Use analogWrite with 50% duty cycle to generate tone
            // Arduino Due supports analogWrite on most pins
            analogWrite(SOUND_PIN, 200); // 127 = 50% of 255
            
            // Note: This produces a fixed frequency PWM, not a true tone
            // For proper tone generation, we'd need timer interrupts
            // But this should produce audible sound on a buzzer
        } else {
            analogWrite(SOUND_PIN, 0);
        }
    }
};

// Global beep controller
BeepSound beepSound;
BeepSound beepSound2;  // Second beep controller

int PATTERN_COMMS[] = {1000};
int PATTERN_TEMP[] = {2000, 0};

struct actionGroups
{
public:
    bool isStage;
    bool isAbort;
    bool isSAS;
    bool isRCS;
    bool isLights;
    bool isGear;
    bool isBrake;
};

enum speedMode
{
    SPEED_SURFACE_MODE,
    SPEED_ORBIT_MODE,
    SPEED_TARGET_MODE,
    SPEED_VERTICAL_MODE
};

// Create insatance of Simpit
KerbalSimpit mySimpit(Serial);
// Inbound
resourceMessage liquidFuelMsg;
resourceMessage liquidFuelStageMsg;
resourceMessage oxidizerMsg;
resourceMessage oxidizerStageMsg;
resourceMessage solidFuelMsg;
resourceMessage solidFuelStageMsg;
resourceMessage xenonGasMsg;
resourceMessage xenonGasStageMsg;
resourceMessage monopropellantMsg;
resourceMessage evaMonopropellantMsg;
resourceMessage electricityMsg;
resourceMessage oreMsg;
resourceMessage ablatorMsg;
resourceMessage ablatorStageMsg;
CustomResourceMessage customResource1Msg;
altitudeMessage altitudeMsg;
velocityMessage velocityMsg;
airspeedMessage airspeedMsg;
apsidesMessage apsidesMsg;
apsidesTimeMessage apsidesTimeMsg;
maneuverMessage maneuverMsg;
SASInfoMessage sasInfoMsg;
orbitInfoMessage orbitInfoMsg;
vesselPointingMessage vesselPointingMsg;
flightStatusMessage flightStatusMsg;
atmoConditionsMessage atmoConditionsMsg;
deltaVMessage deltaVMsg;
deltaVEnvMessage deltaVEnvMsg;
burnTimeMessage burnTimeMsg;
cagStatusMessage cagStatusMsg;
tempLimitMessage tempLimitMsg;
targetMessage targetMsg;
// Custom
actionGroups ag;
String soi = "";

// Track vessel changes (use multiple properties to distinguish vessel change from staging)
byte lastVesselStage = 255;  // 255 = uninitialized
byte lastVesselType = 255;   // Track vessel type to detect actual vessel changes
byte lastCrewCount = 255;    // Track crew count as additional identifier

byte infoMode = 0;       // Track which info mode (1-12)
byte directionMode = 0;  // Track which direction mode (1-12)


/////////////////////////////////////////////////////////////////
/////////////////////// Configing stuff /////////////////////////
/////////////////////////////////////////////////////////////////

// Degree character for the lcd
const char DEGREE_CHAR_LCD = 223;

// Timer intervals (milliseconds)
const unsigned long MAIN_LOOP_INTERVAL = 1000;
const unsigned long LCD_UPDATE_INTERVAL = 250;
const unsigned long TWO_SECOND_INTERVAL = 2000;
const unsigned long THROTTLE_DEBUG_INTERVAL = 500;
const unsigned long MANUAL_REFRESH_INTERVAL = 1000;
const unsigned long CAMERA_UPDATE_INTERVAL = 50;
const unsigned long HOLD_OVERRIDE_DELAY = 2000;

// LED blink intervals (milliseconds)
const unsigned long TEMP_WARNING_BLINK_INTERVAL = 250;
const unsigned long GEE_WARNING_BLINK_INTERVAL = 250;
const unsigned long GEAR_WARNING_BLINK_INTERVAL = 250;
const unsigned long PITCH_WARNING_BLINK_INTERVAL = 200;
const unsigned long AUTOPILOT_LED_BLINK_INTERVAL = 1000;

// Startup delays (milliseconds)
const unsigned long STARTUP_BEEP_DELAY = 200;

// Warning thresholds
const byte COMMS_WARNING_THRESHOLD = 50; // 50%
const int LOW_ALTITUDE_WARNING_THRESHOLD = 200; // TERRAIN warning: below this altitude in meters (match KSP mod)
const float TIME_TO_IMPACT_WARNING_THRESHOLD = 5.0; // PULL UP warning: time to impact in seconds (match KSP mod)
const float HIGH_GEE_WARNING_SOLID_THRESHOLD = 4.5; // 4.5G (slow beeping)
const float HIGH_GEE_WARNING_BLINKING_THRESHOLD = 6.5; // 6.5G (fast beeping)
const byte HIGH_TEMP_WARNING_SOLID_THRESHOLD = 60; // 60%
const byte HIGH_TEMP_WARNING_BLINKING_THRESHOLD = 80; // 80%
const float GEAR_SPEED_WARNING_THRESHOLD = 100.0; // 100 m/s
const float OVERSPEED_THRESHOLD = 900.0; // 900 m/s
const float OVERSPEED_ALTITUDE_THRESHOLD = 15000.0; // 15 km

// Joystick configs
const float JOYSTICK_SMOOTHING_FACTOR = 0.2;  // Adjust this value for more or less smoothing (For Rot and Trans)
const int JOYSTICK_DEADZONE = 50;  // Deadzone range
const int JOYSTICK_DEADZONE_CENTER = 90;  // Snap centering
const int CAMERA_DEADZONE = 150; // Deadzone for camera joystick

// Throttle configs
const int THROTTLE_DEADZONE = 50;   // Deadzone at min and max

// Precision mode
const float DEFAULT_PRECISION_MODIFIER = 0.3;
const float MIN_PRECISION_MODIFIER = 0.1;  // Minimum 10%
const float MAX_PRECISION_MODIFIER = 1.0;  // Maximum 100%
const float PRECISION_STEP = 0.1;          // Increment/decrement by 10%

// Autopilot tuning constants
const float AP_HEADING_K = 0.02;  // proportional gain for heading -> yaw input (deg -> fraction)
const float AP_SPEED_K = 0.08;   // proportional gain for speed -> throttle fraction per m/s
const float AP_THROTTLE_ADAPT_RATE = 0.005; // rate at which base throttle adapts to find equilibrium
const float AP_ROLL_K = 0.0035;    // proportional gain for roll -> roll input (deg -> fraction)
const float THROTTLE_SMOOTH_ALPHA = 0.8; // smoothing alpha for throttle changes (0..1)
const float MIN_THROTTLE_FRACTION = 0.0; // allow throttle to go to zero when slowing down
const float AUTOPILOT_ALT_PRIORITY_THRESHOLD = 5.0; // meters: if altitude error is larger, deprioritize speed matching
const float AUTOPILOT_HEADING_PRIORITY_THRESHOLD = 2.0; // degrees: if heading error is larger, deprioritize speed matching

// Roll sensitivity
const float ROLL_SENSITIVITY = 1;

// Serial baud rate
const unsigned long SERIAL_BAUD_RATE = 115200;

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

speedMode currentSpeedMode;
speedMode navballSpeedMode = SPEED_SURFACE_MODE;

// State variables
float precisionModifier = DEFAULT_PRECISION_MODIFIER;
bool lastGrabButtonState = false;
bool lastBoardButtonState = false;
bool isDebugMode = true;

// Precision display overlay
bool precisionDisplayActive = false;
unsigned long precisionDisplayStartTime = 0;
const unsigned long PRECISION_DISPLAY_DURATION = 500;  // Show for 500ms
bool isConnectedToKSP = false;
bool initializationComplete = false;  // Set to true after passcode and calibration
bool useImperialUnits = false;  // Global flag for imperial/metric units
int loopCount = 0;
int previousMillis;  // For hz calculation

// Autopilot state (repurposed from physical warp switch)
bool autopilotEnabled = false;
float autopilotHeading = 0.0f;
float autopilotSpeed = 0.0f;
float autopilotAltitude = 0.0f;
unsigned long autopilotEngageTime = 0;
bool sasWasOnBeforeAutopilot = false;  // Track SAS state to restore after autopilot
bool viewModeEnabled = false;  // Toggle state for translation button view mode

// Trim offsets (added to joystick input)
int16_t trimTransX = 0;
int16_t trimTransY = 0;
int16_t trimTransZ = 0;
int16_t trimRotX = 0;
int16_t trimRotY = 0;  // No default pitch trim
int16_t trimRotZ = 0;

// Easter egg state for direction mode 12
int easterEggAbortCount = 0;
bool easterEggActive = false;
int easterEggPosition = 0;
unsigned long lastEasterEggUpdate = 0;
const unsigned long EASTER_EGG_ANIMATION_SPEED = 150;  // ms between frames


// Calibration values (set during startup calibration)
int throttleCalMin = 75;
int throttleCalMax = 765;
int rotXCalMin = 0;
int rotXCalMax = 1023;
int rotXCalCenter = 512;
int rotYCalMin = 0;
int rotYCalMax = 1023;
int rotYCalCenter = 512;
int rotZCalMin = 0;
int rotZCalMax = 1023;
int rotZCalCenter = 512;
int transXCalMin = 0;
int transXCalMax = 1023;
int transXCalCenter = 512;
int transYCalMin = 0;
int transYCalMax = 1023;
int transYCalCenter = 512;
int transZCalMin = 0;
int transZCalMax = 1023;
int transZCalCenter = 512;

// Camera control
unsigned long lastCameraUpdate = 0;

// Debug flags for resource messages
bool lfReceived = false;
bool oxReceived = false;
bool sfReceived = false;
bool mpReceived = false;
bool ecReceived = false;

// Timers
Timer timer;
Timer lcdTimer;
Timer twoSecondTimer;
Timer throttleDebugTimer;
Timer manualRefreshTimer;
Timer joystickDebugTimer;



/////////////////////////////////////////////////////////////////
////////////////////////// Functions ////////////////////////////
/////////////////////////////////////////////////////////////////

void setup()
{
     
    loopCount = 0;
    timer.start(MAIN_LOOP_INTERVAL);
    lcdTimer.start(LCD_UPDATE_INTERVAL);
    twoSecondTimer.start(TWO_SECOND_INTERVAL);
    throttleDebugTimer.start(THROTTLE_DEBUG_INTERVAL);
    manualRefreshTimer.start(MANUAL_REFRESH_INTERVAL);
    joystickDebugTimer.start(500); // 500ms for joystick debug updates
    // Open up the serial port
    Serial.begin(SERIAL_BAUD_RATE);
    // Init I/O
    initIO();

    // Show initializing messages on the LCDs during startup
    // Show the same initializing message on all LCDs
    Output.setSpeedLCD("Initializing", "Waiting...");
    Output.setAltitudeLCD("Initializing", "Waiting...");
    Output.setHeadingLCD("Initializing", "Waiting...");
    Output.setInfoLCD("Initializing", "Waiting...");
    Output.setDirectionLCD("Initializing", "Waiting...");
    Output.update();
	
    // Test beep on startup
    beepSound.setSound(1000, true);
    beepSound.updateBeep();
    delay(STARTUP_BEEP_DELAY);
    beepSound.setSound(0, false);
    beepSound.updateBeep();

    Output.setLED(POWER_LED, true);
    while (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
    {
        preKSPConnectionLoop();
    }
    Output.setLED(POWER_LED, true);
    
	printDebug("Starting Simpit");
	setAllOutputs(true);
    ///// Initialize Simpit
    initSimpit();
    
    // Passcode entry required after KSP connection: {5, 2, 5, 3}
    waitForPasscode();

    // Additional things to do at start AFTER initialization

    // Run calibration routine
    runCalibration();
    
    //waitForInputEnable();
    mySimpit.update();

    // Initialization complete - now allow vessel change detection
    initializationComplete = true;
    printDebug("Initialization Complete!");
    mySimpit.requestMessageOnChannel(FLIGHT_STATUS_MESSAGE);


} 

void loop() 
{
    loopCount++;
    // Update input from controller (Refresh inputs)
    Input.update();
    // Update simpit (receive messages from KSP including CAG status)
    mySimpit.update();
    // This ensures LEDs stay in sync even if a message is missed
    if (manualRefreshTimer.check() && isConnectedToKSP)
    {
        mySimpit.requestMessageOnChannel(ACTIONSTATUS_MESSAGE);
        mySimpit.requestMessageOnChannel(CAGSTATUS_MESSAGE);
        mySimpit.requestMessageOnChannel(SAS_MODE_INFO_MESSAGE);
        mySimpit.requestMessageOnChannel(SOI_MESSAGE);
        mySimpit.requestMessageOnChannel(FLIGHT_STATUS_MESSAGE);
    }
    
    // Check if precision display overlay should be cleared
    if (precisionDisplayActive && (millis() - precisionDisplayStartTime >= PRECISION_DISPLAY_DURATION))
    {
        precisionDisplayActive = false;
    }
    
    // Refresh logic, I/O, etc. This is all local to KSPArduino.ino
    refresh();
    // Update output to controller (send LED states to hardware)
    Output.update();
} 


void initIO()
{
    // Initialize Output
    Output.init();
    // Initialize Input
    Input.init(Serial);
    Input.setAllVPinsReady();
	
	// Cool startup LED sequence
	printDebug("Testing I/O - LED Startup Sequence");
	
	// Turn all LEDs off first
	setAllOutputs(false);
    Output.update();
	delay(200);
	
	// Light up LEDs one at a time in sequence
	for (int i = 0; i <= TOTAL_LEDS; i++)
	{
		Output.setLED(i, true);
		Output.update();
		delay(8);  // Fast sequence - all LEDs in ~1.2 seconds
	}
	
	// Hold all on briefly
	delay(300);
	
	// Turn off in reverse order
	for (int i = TOTAL_LEDS; i >= 0; i--)
	{
		Output.setLED(i, false);
		Output.update();
		delay(8);
	}
	
	// Brief pause
	delay(100);

    // Input
    Input.update();
	// Done
	printDebug("I/O Initialized");
}

void setAllOutputs(bool state)
{
	for (int i = 0; i <= TOTAL_LEDS; i++)
	{
		Output.setLED(i, state);
	}
}

void waitForPasscode()
{
    const int passcode[4] = {6, 9, 6, 9};
    int currentPosition = 0;
    int enteredCode[4] = {0, 0, 0, 0};
    
    // Display passcode prompt
    Output.setSpeedLCD("ENTER PASSCODE", "Use AG Keys");
    Output.setAltitudeLCD("SECURITY", "LOCKED");
    Output.setHeadingLCD("", "");
    Output.setInfoLCD("Progress:", String(currentPosition) + " of 4");
    Output.setDirectionLCD("Waiting...", "");
    Output.update();
    
    // Blink power LED while waiting for passcode
    unsigned long lastBlinkTime = millis();
    bool blinkState = false;
    
    while (currentPosition < 4)
    {
        Input.update();
        
        // Blink power LED
        if (millis() - lastBlinkTime > 500)
        {
            blinkState = !blinkState;
            Output.setLED(POWER_LED, blinkState);
            lastBlinkTime = millis();
            Output.update();
        }
        
        // Check each AG button
        for (int i = 1; i <= 10; i++)
        {
            int vpin = 0;
            switch(i) {
                case 1: vpin = VPIN_CAG1; break;
                case 2: vpin = VPIN_CAG2; break;
                case 3: vpin = VPIN_CAG3; break;
                case 4: vpin = VPIN_CAG4; break;
                case 5: vpin = VPIN_CAG5; break;
                case 6: vpin = VPIN_CAG6; break;
                case 7: vpin = VPIN_CAG7; break;
                case 8: vpin = VPIN_CAG8; break;
                case 9: vpin = VPIN_CAG9; break;
                case 10: vpin = VPIN_CAG10; break;
            }
            
            if (Input.getVirtualPin(vpin) == ON)
            {
                // Record the button pressed
                enteredCode[currentPosition] = i;
                currentPosition++;
                
                // Acknowledge beep
                beepSound.setSound(1500, true);
                delay(100);
                beepSound.setSound(0, false);
                
                // Update progress
                Output.setInfoLCD("Progress:", String(currentPosition) + " of 4");
                Output.update();
                
                // Wait for button release
                while (Input.getVirtualPin(vpin) != OFF)
                {
                    Input.update();
                    delay(10);
                }
                delay(100); // Debounce
                
                // Check if we've entered all 4 digits
                if (currentPosition == 4)
                {
                    // Verify the complete passcode
                    bool isCorrect = true;
                    for (int j = 0; j < 4; j++)
                    {
                        if (enteredCode[j] != passcode[j])
                        {
                            isCorrect = false;
                            break;
                        }
                    }
                    
                    if (!isCorrect)
                    {
                        // Wrong passcode - reset and show error
                        currentPosition = 0;
                        
                        // Error beep
                        beepSound.setSound(500, true);
                        delay(300);
                        beepSound.setSound(0, false);
                        
                        // Show error
                        Output.setInfoLCD("WRONG!", "Try Again");
                        Output.setDirectionLCD("Reset to 0", "");
                        Output.update();
                        delay(1500);
                        
                        // Restore display
                        Output.setInfoLCD("Progress:", String(currentPosition) + " of 4");
                        Output.setDirectionLCD("Waiting...", "");
                        Output.update();
                    }
                }
                break;
            }
        }
        
        delay(10);
    }
    
    // Passcode correct!
    Output.setSpeedLCD("PASSCODE", "ACCEPTED!");
    Output.setAltitudeLCD("", "");
    Output.setHeadingLCD("Access", "Granted");
    Output.setInfoLCD("", "");
    Output.setDirectionLCD("", "");
    Output.setLED(POWER_LED, true);
    Output.update();
    
    // Success beep sequence
    beepSound.setSound(1000, true);
    delay(100);
    beepSound.setSound(0, false);
    delay(50);
    beepSound.setSound(1500, true);
    delay(100);
    beepSound.setSound(0, false);
    delay(50);
    beepSound.setSound(2000, true);
    delay(200);
    beepSound.setSound(0, false);
    
    delay(1000);
}

void preKSPConnectionLoop()
{
    uint32_t timeStart = millis();

    //////////////////////////////////////////////////////
    
    Input.update();
    // Debug mode: allow LED testing via serial input
    if (Input.getVirtualPin(VPIN_MOD_BUTTON, false) == ON)
    {
        while (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
        {
            serialLedTestDebug();
            Input.update();
        }
    }
    setAllOutputs(millis() % 1000 < 500);  // Blink all LEDs at 1Hz
    
    // Test all inputs, print when any button is pressed
    for (int i = 0; i < 102; i++)
    {
        auto state = Input.getVirtualPin(i);
        if (state == ON)
        {
            printDebug("Input " + String(i) + " pressed");
        }
    }

    /////////////////////////////////////////////////////

    uint32_t loopDelay = millis() - timeStart;
    if (twoSecondTimer.check())
    {
        printDebug("\nLoop Rate: " + String(loopDelay));
        printDebug(" ms\n------------END OF LOOP---------------");
    }
}
void serialLedTestDebug()
{
    // Allow user to serial input pin number and enable that and disable all others
	printDebug("\nEnter pin number to enable (0-145):");
    // Clear any leftover data in serial buffer
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Wait for actual input
    while (Serial.available() == 0) {}
    
    int pinToEnable = Serial.parseInt();
    
    // Clear remaining characters (like newline)
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    if (pinToEnable >= 0 && pinToEnable <= TOTAL_LEDS)
    {
        setAllOutputs(false);
        Output.setLED(pinToEnable, true);
        Output.update();
        printDebug("LED " + String(pinToEnable) + " is now ON");
	}
    else
    {
        printDebug("Invalid pin number.");
    }
}

// Helper function for calibration: waits for any button press
// Returns true if MOD button was pressed (skip to center), false if any other button (continue calibration)
bool waitForAnyButton()
{
    bool modPressed = false;
    
    // List of CAG buttons to check
    const int cagButtons[] = {
        VPIN_CAG1, VPIN_CAG2, VPIN_CAG3, VPIN_CAG4, VPIN_CAG5, 
        VPIN_CAG6, VPIN_CAG7, VPIN_CAG8, VPIN_CAG9, VPIN_CAG10
    };
    const int numButtons = sizeof(cagButtons) / sizeof(cagButtons[0]);
    
    while (true) {
        Input.update();
        
        // Show live value on Heading LCD based on what axis we're calibrating
        // This is a simplified version - could be enhanced to detect which axis
        int liveVal = Input.getThrottleAxis(); // Default to throttle, actual value set by caller
        Output.setHeadingLCD("LIVE VALUE", String(liveVal));
        Output.update();
        
        // Check if MOD button pressed (skip)
        if (Input.getVirtualPin(VPIN_MOD_BUTTON, false) == ON) {
            modPressed = true;
            // Wait for release
            while (Input.getVirtualPin(VPIN_MOD_BUTTON, false) == ON) {
                Input.update();
                delay(10);
            }
            delay(100); // Debounce
            return true; // Skip to center
        }
        
        // Check any CAG button (continue)
        for (int i = 0; i < numButtons; i++) {
            if (Input.getVirtualPin(cagButtons[i], false) == ON) {
                // Wait for release
                while (Input.getVirtualPin(cagButtons[i], false) == ON) {
                    Input.update();
                    delay(10);
                }
                delay(100); // Debounce
                return false; // Continue calibration
            }
        }
        
        delay(50);
    }
}

void runCalibration()
{
    Output.setSpeedLCD("CALIBRATION", "MODE");
    Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
    Output.setHeadingLCD("", "");
    Output.setInfoLCD("Skip uses", "default min/max");
    Output.setDirectionLCD("", "");
    Output.update();
    delay(3000);
    
    bool skipCalibration = false;
    
    // Step 1: Throttle Min
    Output.setSpeedLCD("THROTTLE", "MIN Position");
    Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
    Output.update();
    
    skipCalibration = waitForAnyButton();
    if (!skipCalibration) {
        throttleCalMin = Input.getThrottleAxis();
        Output.setAltitudeLCD("Min: " + String(throttleCalMin), "Recorded!");
        Output.update();
        delay(1000);
    }
    
    // Step 2: Throttle Max
    if (!skipCalibration) {
        Output.setSpeedLCD("THROTTLE", "MAX Position");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            throttleCalMax = Input.getThrottleAxis();
            Output.setAltitudeLCD("Max: " + String(throttleCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 3: Rotation X (Roll) - Left/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION X", "LEFT (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotXCalMin = Input.getRotationXAxis();
            Output.setAltitudeLCD("Min: " + String(rotXCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 4: Rotation X - Right/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION X", "RIGHT (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotXCalMax = Input.getRotationXAxis();
            Output.setAltitudeLCD("Max: " + String(rotXCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 5: Rotation Y (Pitch) - Forward/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION Y", "FORWARD (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotYCalMin = Input.getRotationYAxis();
            Output.setAltitudeLCD("Min: " + String(rotYCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 6: Rotation Y - Back/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION Y", "BACK (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotYCalMax = Input.getRotationYAxis();
            Output.setAltitudeLCD("Max: " + String(rotYCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 7: Rotation Z (Yaw) - Left/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION Z", "LEFT (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotZCalMin = Input.getRotationZAxis();
            Output.setAltitudeLCD("Min: " + String(rotZCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 8: Rotation Z - Right/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("ROTATION Z", "RIGHT (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            rotZCalMax = Input.getRotationZAxis();
            Output.setAltitudeLCD("Max: " + String(rotZCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 9: Translation X - Left/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION X", "LEFT (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transXCalMin = Input.getTranslationXAxis();
            Output.setAltitudeLCD("Min: " + String(transXCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 10: Translation X - Right/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION X", "RIGHT (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transXCalMax = Input.getTranslationXAxis();
            Output.setAltitudeLCD("Max: " + String(transXCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 11: Translation Y - Forward/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION Y", "FORWARD (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transYCalMin = Input.getTranslationYAxis();
            Output.setAltitudeLCD("Min: " + String(transYCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 12: Translation Y - Back/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION Y", "BACK (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transYCalMax = Input.getTranslationYAxis();
            Output.setAltitudeLCD("Max: " + String(transYCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 13: Translation Z - Down/Min
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION Z", "DOWN (Min)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transZCalMin = Input.getTranslationZAxis();
            Output.setAltitudeLCD("Min: " + String(transZCalMin), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 14: Translation Z - Up/Max
    if (!skipCalibration) {
        Output.setSpeedLCD("TRANSLATION Z", "UP (Max)");
        Output.setAltitudeLCD("Any CAG=Calib", "MOD=Skip to Ctr");
        Output.update();
        
        skipCalibration = waitForAnyButton();
        if (!skipCalibration) {
            transZCalMax = Input.getTranslationZAxis();
            Output.setAltitudeLCD("Max: " + String(transZCalMax), "Recorded!");
            Output.update();
            delay(1000);
        }
    }
    
    // Step 15: Center calibration - ALWAYS REQUIRED
    Output.setSpeedLCD("CENTER", "CALIBRATION");
    Output.setAltitudeLCD("Release all", "joysticks");
    Output.setHeadingLCD("DON'T TOUCH!", "");
    Output.setInfoLCD("Press any CAG", "when ready");
    Output.update();
    delay(3000);
    
    waitForAnyButton();
    
    rotXCalCenter = Input.getRotationXAxis();
    rotYCalCenter = Input.getRotationYAxis();
    rotZCalCenter = Input.getRotationZAxis();
    transXCalCenter = Input.getTranslationXAxis();
    transYCalCenter = Input.getTranslationYAxis();
    transZCalCenter = Input.getTranslationZAxis();
    
    Output.setAltitudeLCD("Centers:", "Recorded!");
    Output.setHeadingLCD("Rot: " + String(rotXCalCenter) + "," + String(rotYCalCenter) + "," + String(rotZCalCenter), "");
    Output.setInfoLCD("Trn: " + String(transXCalCenter) + "," + String(transYCalCenter) + "," + String(transZCalCenter), "");
    Output.update();
    delay(3000);
    
    // Calibration complete
    Output.setSpeedLCD("CALIBRATION", "COMPLETE!");
    Output.setAltitudeLCD("Starting", "Simpit...");
    Output.setHeadingLCD("", "");
    Output.setInfoLCD("", "");
    Output.setDirectionLCD("", "");
    Output.update();
    delay(2000);
}

void initSimpit()
{
    //Output.setSpeedLCD("Waiting for KSP", "");
    Output.update();
    // Wait for a connection to ksp
    // Show waiting message and keep LCDs updated while waiting
    // Show same waiting message on all LCDs and keep them updated while waiting
    Output.setSpeedLCD("Waiting for Simpit", "");
    Output.setAltitudeLCD("Waiting for Simpit", "");
    Output.setHeadingLCD("Waiting for Simpit", "");
    Output.setInfoLCD("Waiting for Simpit", "");
    Output.setDirectionLCD("Waiting for Simpit", "");
    Output.update();
    while (!mySimpit.init()) {
        delay(100);
        Output.update();
    }
    
    // Set connection flag
    isConnectedToKSP = true;
    
    // Show that the controller has connected
    printDebug("KSP Controller Connected!");
    // Update all LCDs to show successful connection
    Output.setSpeedLCD("Connected to KSP", "");
    Output.setAltitudeLCD("Connected to KSP", "");
    Output.setHeadingLCD("Connected to KSP", "");
    Output.setInfoLCD("Connected to KSP", "");
    Output.setDirectionLCD("Connected to KSP", "");
    Output.update();
    // Register a method for receiving simpit message from ksp
    mySimpit.inboundHandler(myCallbackHandler);
    // Register the simpit channels
    registerSimpitChannels();
}

void printHz(String customTxt)
{
    // Measure the current time
    unsigned long currentMillis = millis();

    // Calculate the time elapsed since the previous iteration
    unsigned long elapsedTime = currentMillis - previousMillis;

    // Print the loop rate (inverse of the elapsed time)
    float loopRate = 1000.0 / elapsedTime;  // Convert to loops per second (Hz)
    printDebug(String(customTxt) + ": " + String(loopRate) + " Hz");
    // Update the previous timestamp for the next iteration
    previousMillis = currentMillis;
}

void vesselChange()
{
	// Request important states
    mySimpit.requestMessageOnChannel(ACTIONSTATUS_MESSAGE);
    mySimpit.requestMessageOnChannel(CAGSTATUS_MESSAGE);
    mySimpit.requestMessageOnChannel(SAS_MODE_INFO_MESSAGE);
	// Update everything else
	mySimpit.update();

    // Force-refresh CAG and action-group LED state so the controller reflects the
    // new vessel's action availability immediately.
    refreshCAGs();
    setActionGroupLEDs();
    setSASModeLEDs();

    //delay(2000); // Wait a moment to ensure messages are received
    //keyboardEmulatorMessage pauseMessage(0x1B); // ESC key
    //mySimpit.send(KEYBOARD_EMULATOR, pauseMessage); // ESC key
    mySimpit.printToKSP("Vessel change detected!", PRINT_TO_SCREEN);

    // Print to KSP screen
    mySimpit.printToKSP("Input deactivated!", PRINT_TO_SCREEN);
	
	// Blocking calls that must be met before controller continues
	// SAS	
	waitForUserCorrection(VPIN_SAS_SWITCH, (ag.isSAS) ? ON : OFF, "SAS");
	// RCS
	waitForUserCorrection(VPIN_RCS_SWITCH, (ag.isRCS) ? ON : OFF, "RCS");
	// Locks should be off
	//waitForUserCorrection(VPIN_THROTTLE_LOCK_SWITCH, OFF, "Throttle Lock");
	//waitForUserCorrection(VPIN_STAGE_LOCK_SWITCH, OFF, "Stage Lock");
	//waitForUserCorrection(VPIN_ABORT_LOCK_SWITCH, OFF, "Abort Lock");
	//waitForUserCorrection(VPIN_WARP_LOCK_SWITCH, OFF, "Warp Lock");
	// Toggle actions
	// Gear
	waitForUserCorrection(VPIN_GEAR_SWITCH, (!ag.isGear) ? ON : OFF, "Gear");
	// Lights
	waitForUserCorrection(VPIN_LIGHTS_SWITCH, (ag.isLights) ? ON : OFF, "Lights");
	// Brake
	waitForUserCorrection(VPIN_BRAKE_SWITCH, (ag.isBrake) ? ON : OFF, "Brake");
	// Other values that should be off
	//waitForUserCorrection(VPIN_PRECISION_SWITCH, OFF, "Precision");
	waitForUserCorrection(VPIN_DOCKING_SWITCH, OFF, "Docking");
    // UI
    waitForUserCorrection(VPIN_DUAL_SWITCH, OFF, "Dual/Single");
    // Nav
    waitForUserCorrection(VPIN_NAV_SWITCH, OFF, "NAV/FLI");
    // View
    waitForUserCorrection(VPIN_VIEW_SWITCH, OFF, "IVA/EXT");
    // Physical Warp
    waitForUserCorrection(VPIN_AUTO_PILOT_SWITCH, OFF, "Autopilot");
    // 


    mySimpit.printToKSP("Inputs all set.", PRINT_TO_SCREEN);
    // Print to KSP screen
    mySimpit.printToKSP("Input reactivated!", PRINT_TO_SCREEN);
    keyboardEmulatorMessage pauseMsg(0x1B); // ESC key
    mySimpit.send(KEYBOARD_EMULATOR, pauseMsg);
}

void waitForUserCorrection(byte input, bool desiredState, String switchName)
{
    Timer waitTimer;
    waitTimer.start(250);
    String stateText = (desiredState == ON) ? "ON" : "OFF";
    String message = "Set " + switchName + " to " + stateText;
    bool shouldPrintSetMessage = false;
	while (Input.getVirtualPin(input, false) != desiredState)
	{
        if (waitTimer.check())
        {
            mySimpit.printToKSP(message, PRINT_TO_SCREEN);
        }
		delay(50);
		Input.update();
        mySimpit.update();  // Process incoming messages to update LEDs
        Output.update();    // Update LED outputs
        
        // Update warning LEDs while waiting
        setCommsWarning();
        setAltWarning();
        setPitchWarning();
        setActionGroupLEDs();
        // Keep audible warnings updated while waiting
        updateWarningSound();

        // enable sound, ui, pause
        refreshSoundSwitch();
        refreshUI();
        refreshPause();

        if (!shouldPrintSetMessage)
        {
            shouldPrintSetMessage = true;
        }
    }
    if (shouldPrintSetMessage)
    {
        mySimpit.printToKSP(switchName + " set.", PRINT_TO_SCREEN);
    }
}
void refresh()
{
    // Joystick debugging mode: when debug switch AND autopilot switch are both ON
    if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON && 
        Input.getVirtualPin(VPIN_AUTO_PILOT_SWITCH, false) == ON)
    {
        debugJoysticks();
        return; // Exit refresh early in debug mode1111111111111111111111111111111
    }
    
    // Check flight status to determine which controls are active
    bool inFlight = flightStatusMsg.isInFlight();
    bool inEVA = flightStatusMsg.isInEVA();
    
    // Controls that work in both flight and EVA
    refreshWarp();
    refreshAP();
    refreshMod();
    refreshPause();
    refreshCamReset();
    refreshCamMode();
    refreshFocus();
    refreshView();
    refreshNav();
    refreshUI();
    // Toggle sound in KSP when sound switch changed
    refreshSoundSwitch();
    
    // Flight-only controls (not EVA)
    if (inFlight && !inEVA)
    {
        refreshStage();
        refreshAbort();
        refreshLights();
        refreshGear();
        refreshBrake();
        refreshDocking();
        refreshSAS();
        refreshRCS();
        refreshAllSASModes();
        refreshCAGs();
        
        refreshThrottle();
        refreshTranslation();
        refreshRotation();
        
        // Update resource LEDs
        setSFLEDs();
        setLFLEDs();
        setOXLEDs();
        setECLEDs();
        
        // Update warning LEDs
        setTempWarning();
        setGeeWarning();
        setGearWarning();
        setWarpWarning();
        setCommsWarning();
        setAltWarning();
        setPitchWarning();
        
        refreshWarningButtons();// Numpad 0-9

        refreshRotationButton();
        

        // Update audible warning based on above LED conditions
        updateWarningSound();
        
        // Update action group status LEDs (sync with game state)
        setActionGroupLEDs();
    }
    
    // EVA-only controls
    if (inEVA)
    {
        refreshJump();

        
        // EVA uses RCS translation controls for movement
        refreshTranslation();
        refreshRotation();
        
        // Show EVA monopropellant
        setMPLEDs();
    }
    
    // Update displays for both flight and EVA
    if (inFlight)  // In flight or EVA
    {
        updateReferenceMode();
        updateDirectionMode();
        updateInfoMode();
        
        // Update action group LEDs (always keep in sync with game state)
        setActionGroupLEDs();
        setSASModeLEDs();
        
        // Update LCDs on timer (every 250ms) to avoid freezing
        if (lcdTimer.check())
        {
            setSpeedLCD();
            setAltitudeLCD();
            setHeadingLCD();
            setInfoLCD();
            setDirectionLCD();
        }

        refreshGrab();  // EVA grab (F key)
        refreshBoard(); // EVA board (B key)
    }
}

// Choose and play a prioritized warning sound based on current telemetry
void updateWarningSound()
{
    // Sound disabled for GEE, ALT (TERRAIN), and PITCH (PULL UP) warnings
    // Only LEDs are used for these warnings
    
    // Other warnings still use sound
    bool tempBlink = (tempLimitMsg.tempLimitPercentage >= HIGH_TEMP_WARNING_BLINKING_THRESHOLD);
    bool tempSolid = (tempLimitMsg.tempLimitPercentage >= HIGH_TEMP_WARNING_SOLID_THRESHOLD);
    bool commsLow = (flightStatusMsg.commNetSignalStrenghPercentage < COMMS_WARNING_THRESHOLD);

    // Priority: TEMP, COMMS
    if (tempBlink || tempSolid)
    {
        beepSound.beep(PATTERN_TEMP, sizeof(PATTERN_TEMP) / sizeof(int), true, 500);
    }
    else if (commsLow)
    {
        beepSound.beep(PATTERN_COMMS, sizeof(PATTERN_COMMS) / sizeof(int), true, 800);
    }
    else
    {
        // No warnings - stop beeping
        beepSound.beep(nullptr, 0, false, 0);
    }
}



/// <summary>Info from ksp.</summary>
void myCallbackHandler(byte messageType, byte msg[], byte msgSize)
{
    switch (messageType)
    {
    case LF_MESSAGE:
        if (msgSize == sizeof(resourceMessage)) {
            if (!lfReceived) {
                // Print raw bytes
                String rawData = "LF Raw bytes: ";
                for (int i = 0; i < msgSize; i++) {
                    if (msg[i] < 16) rawData += "0";
                    rawData += String(msg[i], HEX) + " ";
                }
                printDebug(rawData);
                
                // Manual float decode to verify
                float manualTotal, manualAvail;
                memcpy(&manualTotal, &msg[0], 4);
                memcpy(&manualAvail, &msg[4], 4);
                printDebug("Manual decode: total=" + String(manualTotal, 2) + " avail=" + String(manualAvail, 2));
            }
            liquidFuelMsg = parseMessage<resourceMessage>(msg);
            if (!lfReceived) {
                lfReceived = true;
                printDebug("LF parsed: total=" + String(liquidFuelMsg.total, 2) + " avail=" + String(liquidFuelMsg.available, 2));
            }
        } else {
            printDebug("LF wrong size: got " + String(msgSize) + " expected " + String(sizeof(resourceMessage)));
        }
        break;
    case LF_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            liquidFuelStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case OX_MESSAGE:
        if (msgSize == sizeof(resourceMessage)) {
            oxidizerMsg = parseMessage<resourceMessage>(msg);
            if (!oxReceived) {
                oxReceived = true;
                printDebug("OX data received: " + String(oxidizerMsg.available) + "/" + String(oxidizerMsg.total));
            }
        }
        break;
    case OX_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            oxidizerStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case SF_MESSAGE:
        if (msgSize == sizeof(resourceMessage)) {
            solidFuelMsg = parseMessage<resourceMessage>(msg);
            if (!sfReceived) {
                sfReceived = true;
                printDebug("SF data received: " + String(solidFuelMsg.available) + "/" + String(solidFuelMsg.total));
            }
        }
        break;
    case SF_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            solidFuelStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case XENON_GAS_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            xenonGasMsg = parseMessage<resourceMessage>(msg);
        break;
    case XENON_GAS_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            xenonGasStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case MONO_MESSAGE:
        if (msgSize == sizeof(resourceMessage)) {
            monopropellantMsg = parseMessage<resourceMessage>(msg);
            if (!mpReceived) {
                mpReceived = true;
                printDebug("MP data received: " + String(monopropellantMsg.available) + "/" + String(monopropellantMsg.total));
            }
        }
        break;
    case EVA_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            evaMonopropellantMsg = parseMessage<resourceMessage>(msg);
        break;
        // MONO_STAGE_MESSAGE ???
    case ELECTRIC_MESSAGE:
        if (msgSize == sizeof(resourceMessage)) {
            electricityMsg = parseMessage<resourceMessage>(msg);
            if (!ecReceived) {
                ecReceived = true;
                printDebug("EC data received: " + String(electricityMsg.available) + "/" + String(electricityMsg.total));
            }
        }
        break;
        // ELECTRIC_STAGE_MESSAGE ???
    case ORE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            oreMsg = parseMessage<resourceMessage>(msg);
        break;
    case AB_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            ablatorMsg = parseMessage<resourceMessage>(msg);
        break;
    case AB_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            ablatorStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case CUSTOM_RESOURCE_1_MESSAGE:
        if (msgSize == sizeof(CustomResourceMessage))
            customResource1Msg = parseCustomResource(msg);
        break;
    case ALTITUDE_MESSAGE:
        if (msgSize == sizeof(altitudeMessage))
            altitudeMsg = parseAltitude(msg);
        break;
    case VELOCITY_MESSAGE:
        if (msgSize == sizeof(velocityMessage))
            velocityMsg = parseMessage<velocityMessage>(msg);
        break;
    case AIRSPEED_MESSAGE:
        if (msgSize == sizeof(airspeedMessage))
            airspeedMsg = parseAirspeed(msg);
        break;
    case APSIDES_MESSAGE:
        if (msgSize == sizeof(apsidesMessage))
            apsidesMsg = parseApsides(msg);
        break;
    case APSIDESTIME_MESSAGE:
        if (msgSize == sizeof(apsidesTimeMessage))
            apsidesTimeMsg = parseApsidesTime(msg);
        break;
    case MANEUVER_MESSAGE:
        if (msgSize == sizeof(maneuverMessage))
            maneuverMsg = parseManeuver(msg);
        break;
    case SAS_MODE_INFO_MESSAGE:
        if (msgSize == sizeof(SASInfoMessage))
            sasInfoMsg = parseSASInfoMessage(msg);
        break;
    case ORBIT_MESSAGE:
        if (msgSize == sizeof(orbitInfoMessage))
            orbitInfoMsg = parseOrbitInfo(msg);
        break;
    case ROTATION_DATA_MESSAGE:
        if (msgSize == sizeof(vesselPointingMessage))
            vesselPointingMsg = parseMessage<vesselPointingMessage>(msg);
        break;
    case ACTIONSTATUS_MESSAGE:
        if (msgSize == 1)
        {
            byte current = msg[0];

            // Stage
            if (current & STAGE_ACTION)
            {
                ag.isStage = true;
            }
            else
            {
                ag.isStage = false;
            }
            // Abort
            if (current & ABORT_ACTION)
            {
                ag.isAbort = true;
            }
            else
            {
                ag.isAbort = false;
            }
            // SAS
            if (current & SAS_ACTION)
            {
                ag.isSAS = true;
            }
            else
            {
                ag.isSAS = false;
            }
            // RCS
            if (current & RCS_ACTION)
            {
                ag.isRCS = true;
            }
            else
            {
                ag.isRCS = false;
            }
            // Gear
            if (current & GEAR_ACTION)
            {
                ag.isGear = true;
            }
            else
            {
                ag.isGear = false;
            }
            // Lights
            if (current & LIGHT_ACTION)
            {
                ag.isLights = true;
            }
            else
            {
                ag.isLights = false;
            }
            // Brakes
            if (current & BRAKES_ACTION)
            {
                ag.isBrake = true;
            }
            else
            {
                ag.isBrake = false;
            }
        }
        break;
    case DELTAV_MESSAGE:
        if (msgSize == sizeof(deltaVMessage))
        {
            deltaVMsg = parseDeltaV(msg);
        }
        break;
    case DELTAVENV_MESSAGE:
        if (msgSize == sizeof(deltaVEnvMessage))
        {
            deltaVEnvMsg = parseDeltaVEnv(msg);
        }
        break;
    case BURNTIME_MESSAGE:
        if (msgSize == sizeof(burnTimeMessage))
        {
            burnTimeMsg = parseBurnTime(msg);
        }
        break;
    case CAGSTATUS_MESSAGE:
        if (msgSize == sizeof(cagStatusMessage))
        {
            cagStatusMsg = parseCAGStatusMessage(msg);
        }
        break;
    case TEMP_LIMIT_MESSAGE:
        if (msgSize == sizeof(tempLimitMessage))
        {
            tempLimitMsg = parseTempLimitMessage(msg);
        }
        break;
    case TARGETINFO_MESSAGE:
        if (msgSize == sizeof(targetMessage))
        {
            targetMsg = parseTarget(msg);
        }
        break;
    case SOI_MESSAGE:
        // SOI strings are null-terminated from KSP
        // Build string from msg up to the null terminator or msgSize
        {
            String s = "";
            for (int i = 0; i < msgSize; i++) {
                if (msg[i] == 0) break;
                s += (char)msg[i];
            }
            soi = s;
        }
        break;
    case SCENE_CHANGE_MESSAGE:

        break;
    case FLIGHT_STATUS_MESSAGE:
        if (msgSize == sizeof(flightStatusMessage))
        {
            flightStatusMsg = parseFlightStatusMessage(msg);
            
            // Detect vessel change (including initial vessel load)
            bool isFirstLoad = (lastVesselType == 255 && lastCrewCount == 255);
            bool vesselTypeChanged = (lastVesselType != 255 && flightStatusMsg.vesselType != lastVesselType);
            bool crewCountChanged = (lastCrewCount != 255 && flightStatusMsg.crewCount != lastCrewCount);
            
            // Trigger vessel change on: initial load OR vessel type/crew changed
            // Only trigger if connected, initialization complete, and in flight
            if (isConnectedToKSP && initializationComplete && flightStatusMsg.isInFlight() && 
                (isFirstLoad || vesselTypeChanged || crewCountChanged))
            {
                // Vessel changed! Pause the game
                mySimpit.update();
                vesselChange();
            }
                    // Update last known vessel properties
            lastVesselStage = flightStatusMsg.currentStage;
            lastVesselType = flightStatusMsg.vesselType;
            lastCrewCount = flightStatusMsg.crewCount;
        }
        break;
    case ATMO_CONDITIONS_MESSAGE:
        if (msgSize == sizeof(atmoConditionsMessage))
        {
            atmoConditionsMsg = parseMessage<atmoConditionsMessage>(msg);
        }
    case VESSEL_NAME_MESSAGE:

        break;
    default:
        break;
    }
}

/// <summary>Register all the needed channels for receiving simpit messages.</summary>
void registerSimpitChannels()
{
    
    // Resources
    mySimpit.registerChannel(LF_MESSAGE);
    mySimpit.registerChannel(LF_STAGE_MESSAGE);
    mySimpit.registerChannel(OX_MESSAGE);
    mySimpit.registerChannel(OX_STAGE_MESSAGE);
    mySimpit.registerChannel(SF_MESSAGE);
    mySimpit.registerChannel(SF_STAGE_MESSAGE);
    //mySimpit.registerChannel(XENON_GAS_MESSAGE);
    //mySimpit.registerChannel(XENON_GAS_STAGE_MESSAGE);
    mySimpit.registerChannel(MONO_MESSAGE);
    mySimpit.registerChannel(EVA_MESSAGE);
    mySimpit.registerChannel(ELECTRIC_MESSAGE);
    mySimpit.registerChannel(ORE_MESSAGE);
    mySimpit.registerChannel(AB_MESSAGE);
    mySimpit.registerChannel(AB_STAGE_MESSAGE);
    //mySimpit.registerChannel(CUSTOM_RESOURCE_1_MESSAGE);
    //// Flight Data
    mySimpit.registerChannel(ALTITUDE_MESSAGE);
    mySimpit.registerChannel(VELOCITY_MESSAGE);
    mySimpit.registerChannel(AIRSPEED_MESSAGE);
    mySimpit.registerChannel(APSIDES_MESSAGE);
    mySimpit.registerChannel(APSIDESTIME_MESSAGE);
    mySimpit.registerChannel(MANEUVER_MESSAGE);
    mySimpit.registerChannel(SAS_MODE_INFO_MESSAGE);
    mySimpit.registerChannel(ORBIT_MESSAGE);
    mySimpit.registerChannel(ROTATION_DATA_MESSAGE);
    mySimpit.registerChannel(ACTIONSTATUS_MESSAGE);
    mySimpit.registerChannel(DELTAV_MESSAGE);
    mySimpit.registerChannel(DELTAVENV_MESSAGE);
    mySimpit.registerChannel(BURNTIME_MESSAGE);
    mySimpit.registerChannel(CAGSTATUS_MESSAGE);
    mySimpit.registerChannel(TEMP_LIMIT_MESSAGE);
    mySimpit.registerChannel(TARGETINFO_MESSAGE);
    mySimpit.registerChannel(SOI_MESSAGE);
    mySimpit.registerChannel(SCENE_CHANGE_MESSAGE);
    mySimpit.registerChannel(FLIGHT_STATUS_MESSAGE);
    mySimpit.registerChannel(ATMO_CONDITIONS_MESSAGE);
    mySimpit.registerChannel(VESSEL_NAME_MESSAGE);
    
}

void refreshMod()
{
    // Track button state to send press/release as modifier key (Right Shift)
    static bool lastEnableState = false;
    static unsigned long modPressStartTime = 0;
    static bool vesselChangeTriggered = false;
    const unsigned long FORCE_VESSEL_CHANGE_DURATION = 10000;  // 10 seconds
    
    bool currentEnableState = (Input.getVirtualPin(VPIN_MOD_BUTTON, false) == ON);
    
    if (currentEnableState && !lastEnableState) // Button just pressed
    {
        printDebug("Mod pressed - Right Shift down");
        keyboardEmulatorMessage msgPress(0xA1, 1);  // Right Shift key - press
        mySimpit.send(KEYBOARD_EMULATOR, msgPress);
        modPressStartTime = millis();
        vesselChangeTriggered = false;
    }
    else if (!currentEnableState && lastEnableState) // Button just released
    {
        printDebug("Mod released - Right Shift up");
        keyboardEmulatorMessage msgRelease(0xA1, 2);  // Right Shift key - release
        mySimpit.send(KEYBOARD_EMULATOR, msgRelease);
        vesselChangeTriggered = false;
    }
    else if (currentEnableState && !vesselChangeTriggered) // Button held
    {
        // Check if held for 10 seconds
        if (millis() - modPressStartTime >= FORCE_VESSEL_CHANGE_DURATION)
        {
            vesselChangeTriggered = true;
            printDebug("Force vessel change - MOD held 10 seconds");
            mySimpit.printToKSP("Force vessel change!", PRINT_TO_SCREEN);
            vesselChange();
        }
    }
    
    lastEnableState = currentEnableState;
}

void updateDirectionMode()
{
    // Array of direction mode VPINs (not sequential)
    const byte directionPins[12] = {
        VPIN_DIRECTION_MODE_1, VPIN_DIRECTION_MODE_2, VPIN_DIRECTION_MODE_3, VPIN_DIRECTION_MODE_4,
        VPIN_DIRECTION_MODE_5, VPIN_DIRECTION_MODE_6, VPIN_DIRECTION_MODE_7, VPIN_DIRECTION_MODE_8,
        VPIN_DIRECTION_MODE_9, VPIN_DIRECTION_MODE_10, VPIN_DIRECTION_MODE_11, VPIN_DIRECTION_MODE_12
    };
    
    byte oldMode = directionMode;
    
    // Check which of the 12 direction mode positions is active
    for (int i = 0; i < 12; i++) 
    {
        ButtonState state = Input.getVirtualPin(directionPins[i], false);
        if (state == ON) 
        {
            directionMode = i + 1;  // Mode is 1-12, array index is 0-11
            
            // Reset easter egg if mode changed away from 12
            if (oldMode == 12 && directionMode != 12)
            {
                easterEggActive = false;
                easterEggAbortCount = 0;
                easterEggPosition = 0;
            }
            
            break;
        }
    }
}

void updateInfoMode()
{
    // Array of info mode VPINs (not sequential)
    const byte infoPins[12] = {
        VPIN_INFO_MODE_1, VPIN_INFO_MODE_2, VPIN_INFO_MODE_3, VPIN_INFO_MODE_4,
        VPIN_INFO_MODE_5, VPIN_INFO_MODE_6, VPIN_INFO_MODE_7, VPIN_INFO_MODE_8,
        VPIN_INFO_MODE_9, VPIN_INFO_MODE_10, VPIN_INFO_MODE_11, VPIN_INFO_MODE_12
    };
    
    // Check which of the 12 info mode positions is active
    for (int i = 0; i < 12; i++) 
    {
        ButtonState state = Input.getVirtualPin(infoPins[i], false);
        if (state == ON) 
        {
            infoMode = i + 1;  // Mode is 1-12, array index is 0-11
            break;
        }
    }
}

void updateReferenceMode()
{
    if (Input.getVirtualPin(VPIN_REFERENCE_MODE_BUTTON) == ON)
    {
        mySimpit.cycleNavBallMode();
        
        // Cycle through Surface -> Orbit -> Target
        switch (navballSpeedMode)
        {
        case SPEED_SURFACE_MODE:
            navballSpeedMode = SPEED_ORBIT_MODE;
            break;
        case SPEED_ORBIT_MODE:
            navballSpeedMode = SPEED_TARGET_MODE;
            break;
        case SPEED_TARGET_MODE:
            navballSpeedMode = SPEED_SURFACE_MODE;
            break;
        default:
            navballSpeedMode = SPEED_SURFACE_MODE;
            break;
        }
    }
    
    // Repurpose vertical velocity switch: ON = imperial, OFF = metric
    useImperialUnits = (Input.getVirtualPin(VPIN_VERTICAL_VELOCITY_SWITCH, false) == ON);
    currentSpeedMode = navballSpeedMode;
}

void refreshWarningButtons()
{
    if (Input.getVirtualPin(VPIN_PITCH_WARNING_BUTTON) == ON)
    {
        printDebug("PITCH warning cancel - Numpad 9");
        keyboardEmulatorMessage msg(0x69);  // Numpad 9
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_ALT_WARNING_BUTTON) == ON)
    {
        printDebug("ALT warning cancel - Numpad 8");
        keyboardEmulatorMessage msg(0x68); // Numpad 8
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_COMMS_WARNING_BUTTON) == ON)
    {
        printDebug("COMMS warning cancel - Numpad 7");
        keyboardEmulatorMessage msg(0x67); // Numpad 7
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_GEAR_WARNING_BUTTON) == ON)
    {
        printDebug("GEAR warning cancel - Numpad 6");
        keyboardEmulatorMessage msg(0x66);  // Numpad 6
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_RCS_WARNING_BUTTON) == ON)
    {
        printDebug("RCS warning cancel - Numpad 5");
        keyboardEmulatorMessage msg(0x65);  // Numpad 5
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_SAS_WARNING_BUTTON) == ON)
    {
        printDebug("SAS warning cancel - Numpad 4");
        keyboardEmulatorMessage msg(0x64);  // Numpad 4
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_BRAKE_WARNING_BUTTON) == ON)
    {
        printDebug("BRAKE warning cancel - Numpad 3");
        keyboardEmulatorMessage msg(0x63);  // Numpad 3
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_WARP_WARNING_BUTTON) == ON)
    {
        printDebug("WARP warning cancel - Numpad 2");
        keyboardEmulatorMessage msg(0x62);  // Numpad 2
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_GEE_WARNING_BUTTON) == ON)
    {
        printDebug("GEE warning cancel - Numpad 1");
        keyboardEmulatorMessage msg(0x61);  // Numpad 1
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
    if (Input.getVirtualPin(VPIN_TEMP_WARNING_BUTTON) == ON)
    {
        printDebug("TEMP warning cancel - Numpad 0");
        keyboardEmulatorMessage msg(0x60);  // Numpad 0
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}

void setTempWarning()
{
    // Check for overspeed condition too
    float surfaceSpeed = velocityMsg.surface;
    float radarAlt = altitudeMsg.surface;
    bool isOverspeed = (surfaceSpeed > OVERSPEED_THRESHOLD && radarAlt < OVERSPEED_ALTITUDE_THRESHOLD);
    
    if (tempLimitMsg.tempLimitPercentage >= HIGH_TEMP_WARNING_BLINKING_THRESHOLD || isOverspeed)
    {
        // Blinking for extreme temperature or overspeed
        Output.setLED(TEMP_WARNING_LED, (millis() / TEMP_WARNING_BLINK_INTERVAL) % 2);
    }
    else if (tempLimitMsg.tempLimitPercentage >= HIGH_TEMP_WARNING_SOLID_THRESHOLD)
    {
        // Solid for high temperature
        Output.setLED(TEMP_WARNING_LED, true);
    }
    else
    {
        Output.setLED(TEMP_WARNING_LED, false);
    }
}
void setGeeWarning()
{
    float gforce = airspeedMsg.gForces;
    Output.setLED(GEE_WARNING_LED, gforce >= HIGH_GEE_WARNING_BLINKING_THRESHOLD ? (millis() / GEE_WARNING_BLINK_INTERVAL) % 2 : 
                                   gforce >= HIGH_GEE_WARNING_SOLID_THRESHOLD);
}
void setGearWarning()
{
    float surfaceSpeed = velocityMsg.surface;
    Output.setLED(GEAR_WARNING_LED, !ag.isGear ? false :
                                    surfaceSpeed > GEAR_SPEED_WARNING_THRESHOLD ? (millis() / GEAR_WARNING_BLINK_INTERVAL) % 2 : true);
}
void setWarpWarning()
{
    byte warpIndex = flightStatusMsg.currentTWIndex;
    
    if (warpIndex <= 1)  // 1x time warp or slower
    {
        Output.setLED(WARP_WARNING_LED, false);
    }
    else  // 2x, 4x, 10x, etc.
    {
        // Blink at rate proportional to warp index: 2x=2Hz, 4x=4Hz, etc.
        // Convert Hz to milliseconds: period = 1000ms / (warpIndex * 2) for 50% duty cycle
        unsigned long blinkPeriod = 1000 / warpIndex;
        Output.setLED(WARP_WARNING_LED, (millis() / blinkPeriod) % 2);
    }
}
void setCommsWarning()
{
    Output.setLED(COMMS_WARNING_LED, flightStatusMsg.commNetSignalStrenghPercentage < COMMS_WARNING_THRESHOLD);
}
void setAltWarning()
{
    float surfaceAlt = max(0.0f, altitudeMsg.surface);
    Output.setLED(ALT_WARNING_LED, !ag.isGear && surfaceAlt > 0.0f && surfaceAlt < LOW_ALTITUDE_WARNING_THRESHOLD);
}
void setPitchWarning()
{
    float verticalSpeed = velocityMsg.vertical;
    float surfaceAlt = max(0.0f, altitudeMsg.surface);
    
    if (verticalSpeed >= 0 || ag.isGear || surfaceAlt <= 0.0f)
    {
        Output.setLED(PITCH_WARNING_LED, false);
        return;
    }

    float timeToImpact = surfaceAlt / -verticalSpeed;
    Output.setLED(PITCH_WARNING_LED, timeToImpact < TIME_TO_IMPACT_WARNING_THRESHOLD ? (millis() / PITCH_WARNING_BLINK_INTERVAL) % 2 : false);
}
void setActionGroupLEDs()
{
    Output.setLED(BRAKE_WARNING_LED, ag.isBrake);
    Output.setLED(SAS_WARNING_LED, autopilotEnabled ? ((millis() / AUTOPILOT_LED_BLINK_INTERVAL) % 2) : ag.isSAS);
    Output.setLED(RCS_WARNING_LED, ag.isRCS);
}

void setSFLEDs()
{
    bool newLEDs[20];
    const int sfPins[20] = {SOLID_FUEL_LED_1, SOLID_FUEL_LED_2, SOLID_FUEL_LED_3, SOLID_FUEL_LED_4, 
                            SOLID_FUEL_LED_5, SOLID_FUEL_LED_6, SOLID_FUEL_LED_7, SOLID_FUEL_LED_8,
                            SOLID_FUEL_LED_9, SOLID_FUEL_LED_10, SOLID_FUEL_LED_11, SOLID_FUEL_LED_12,
                            SOLID_FUEL_LED_13, SOLID_FUEL_LED_14, SOLID_FUEL_LED_15, SOLID_FUEL_LED_16,
                            SOLID_FUEL_LED_17, SOLID_FUEL_LED_18, SOLID_FUEL_LED_19, SOLID_FUEL_LED_20};
    
    
    // Default to total if switch not ready
    if (Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false) == ON) 
    {
        calcResource(solidFuelStageMsg.total, solidFuelStageMsg.available, newLEDs);
    } 
    else 
    {
        calcResource(solidFuelMsg.total, solidFuelMsg.available, newLEDs);
    }
    
    for (int i = 0; i < 20; i++) 
    {
        Output.setLED(sfPins[i], newLEDs[i]);
    }
}
void setLFLEDs()
{
    bool newLEDs[20];
    const int lfPins[20] = {LIQUID_FUEL_LED_1, LIQUID_FUEL_LED_2, LIQUID_FUEL_LED_3, LIQUID_FUEL_LED_4,
                            LIQUID_FUEL_LED_5, LIQUID_FUEL_LED_6, LIQUID_FUEL_LED_7, LIQUID_FUEL_LED_8,
                            LIQUID_FUEL_LED_9, LIQUID_FUEL_LED_10, LIQUID_FUEL_LED_11, LIQUID_FUEL_LED_12,
                            LIQUID_FUEL_LED_13, LIQUID_FUEL_LED_14, LIQUID_FUEL_LED_15, LIQUID_FUEL_LED_16,
                            LIQUID_FUEL_LED_17, LIQUID_FUEL_LED_18, LIQUID_FUEL_LED_19, LIQUID_FUEL_LED_20};
    
    // Default to total if switch not ready
    if (Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false) == ON) 
    {
        calcResource(liquidFuelStageMsg.total, liquidFuelStageMsg.available, newLEDs);
    } 
    else 
    {
        calcResource(liquidFuelMsg.total, liquidFuelMsg.available, newLEDs);
    }
    
    for (int i = 0; i < 20; i++) 
    {
        Output.setLED(lfPins[i], newLEDs[i]);
    }
}
void setOXLEDs()
{
    bool newLEDs[20];
    const int oxPins[20] = {OXIDIZER_LED_1, OXIDIZER_LED_2, OXIDIZER_LED_3, OXIDIZER_LED_4,
                            OXIDIZER_LED_5, OXIDIZER_LED_6, OXIDIZER_LED_7, OXIDIZER_LED_8,
                            OXIDIZER_LED_9, OXIDIZER_LED_10, OXIDIZER_LED_11, OXIDIZER_LED_12,
                            OXIDIZER_LED_13, OXIDIZER_LED_14, OXIDIZER_LED_15, OXIDIZER_LED_16,
                            OXIDIZER_LED_17, OXIDIZER_LED_18, OXIDIZER_LED_19, OXIDIZER_LED_20};
    
    // Default to total if switch not ready
    if (Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false) == ON) 
    {
        calcResource(oxidizerStageMsg.total, oxidizerStageMsg.available, newLEDs);
    } 
    else 
    {
        calcResource(oxidizerMsg.total, oxidizerMsg.available, newLEDs);
    }
    
    for (int i = 0; i < 20; i++) 
    {
        Output.setLED(oxPins[i], newLEDs[i]);
    }
}
void setMPLEDs()
{
    bool newLEDs[20];
    const int mpPins[20] = {MONOPROPELLANT_LED_1, MONOPROPELLANT_LED_2, MONOPROPELLANT_LED_3, MONOPROPELLANT_LED_4,
                            MONOPROPELLANT_LED_5, MONOPROPELLANT_LED_6, MONOPROPELLANT_LED_7, MONOPROPELLANT_LED_8,
                            MONOPROPELLANT_LED_9, MONOPROPELLANT_LED_10, MONOPROPELLANT_LED_11, MONOPROPELLANT_LED_12,
                            MONOPROPELLANT_LED_13, MONOPROPELLANT_LED_14, MONOPROPELLANT_LED_15, MONOPROPELLANT_LED_16,
                            MONOPROPELLANT_LED_17, MONOPROPELLANT_LED_18, MONOPROPELLANT_LED_19, MONOPROPELLANT_LED_20};
    
    if (flightStatusMsg.isInEVA()) 
    {
        calcResource(evaMonopropellantMsg.total, evaMonopropellantMsg.available, newLEDs);
    }
    else 
    {
        calcResource(monopropellantMsg.total, monopropellantMsg.available, newLEDs);
    }
    
    for (int i = 0; i < 20; i++) 
    {
        Output.setLED(mpPins[i], newLEDs[i]);
    }
}
void setECLEDs()
{
    bool newLEDs[20];
    const int ecPins[20] = {ELECTRICITY_LED_1, ELECTRICITY_LED_2, ELECTRICITY_LED_3, ELECTRICITY_LED_4,
                            ELECTRICITY_LED_5, ELECTRICITY_LED_6, ELECTRICITY_LED_7, ELECTRICITY_LED_8,
                            ELECTRICITY_LED_9, ELECTRICITY_LED_10, ELECTRICITY_LED_11, ELECTRICITY_LED_12,
                            ELECTRICITY_LED_13, ELECTRICITY_LED_14, ELECTRICITY_LED_15, ELECTRICITY_LED_16,
                            ELECTRICITY_LED_17, ELECTRICITY_LED_18, ELECTRICITY_LED_19, ELECTRICITY_LED_20};
    
    calcResource(electricityMsg.total, electricityMsg.available, newLEDs);
    
    for (int i = 0; i < 20; i++) 
    {
        Output.setLED(ecPins[i], newLEDs[i]);
    }
}

void refreshStage()
{
    ButtonState stageLock = Input.getVirtualPin(VPIN_STAGE_LOCK_SWITCH, false);
    if (stageLock == ON)
    {
        Output.setLED(STAGE_LED, true);
        if (Input.getVirtualPin(VPIN_STAGE_BUTTON) == ON)
        {
            printDebug("Stage button pressed");
            mySimpit.activateAction(STAGE_ACTION);
        }
    }
    else if (stageLock == OFF)
    {
        Output.setLED(STAGE_LED, false);
    }
}
void refreshAbort()
{
    ButtonState abortLock = Input.getVirtualPin(VPIN_ABORT_LOCK_SWITCH, false);
    if (abortLock == ON)
    {
        Output.setLED(ABORT_LED, true);
        if (Input.getVirtualPin(VPIN_ABORT_BUTTON) == ON)
        {
            printDebug("Abort button pressed");
            
            // Easter egg: Count abort presses when in direction mode 12
            if (directionMode == 12)
            {
                easterEggAbortCount++;
                if (easterEggAbortCount >= 5)
                {
                    easterEggActive = true;
                    easterEggPosition = 0;
                    lastEasterEggUpdate = millis();
                }
            }
            else
            {
                mySimpit.activateAction(ABORT_ACTION);
            }
        }
    }
    else if (abortLock == OFF)
    {
        Output.setLED(ABORT_LED, false);
    }
}
void refreshLights()
{
    switch (Input.getVirtualPin(VPIN_LIGHTS_SWITCH))
    {
    case NOT_READY:
        break;
    case ON:
        printDebug("Lights ON");
        mySimpit.activateAction(LIGHT_ACTION);
        break;
    case OFF:
        printDebug("Lights OFF");
        mySimpit.deactivateAction(LIGHT_ACTION);
        break;
    }
}
void refreshGear()
{
    switch (Input.getVirtualPin(VPIN_GEAR_SWITCH))
    {
    case NOT_READY:
        break;
    case ON:
        printDebug("Gear UP");
        mySimpit.deactivateAction(GEAR_ACTION);
        break;
    case OFF:
        printDebug("Gear DOWN");
        mySimpit.activateAction(GEAR_ACTION);
        break;
    }
}
void refreshBrake()
{
    ButtonState val = Input.getVirtualPin(VPIN_BRAKE_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        printDebug("Brakes ON");
        mySimpit.activateAction(BRAKES_ACTION);
        break;
    case OFF:
        printDebug("Brakes OFF");
        mySimpit.deactivateAction(BRAKES_ACTION);
        break;
    }
}
void refreshDocking()
{
    const bool DONT_CHANGE = true;
    ButtonState val = Input.getVirtualPin(VPIN_DOCKING_SWITCH, DONT_CHANGE);
    if (val == ON || val == OFF)
    {
        if (val == ON)
            printDebug("Docking mode ON");
        else
            printDebug("Docking mode OFF");
        
        keyboardEmulatorMessage msg(0x2E);  // Delete key (toggle docking mode)
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshCAGs()
{
    if (Input.getVirtualPin(VPIN_CAG1) == ON)
    {
        printDebug("CAG1 toggled");
        mySimpit.toggleCAG(1);
    }
    if (Input.getVirtualPin(VPIN_CAG2) == ON)
    {
        printDebug("CAG2 toggled");
        mySimpit.toggleCAG(2);
    }
    if (Input.getVirtualPin(VPIN_CAG3) == ON)
    {
        printDebug("CAG3 toggled");
        mySimpit.toggleCAG(3);
    }
    if (Input.getVirtualPin(VPIN_CAG4) == ON)
    {
        printDebug("CAG4 toggled");
        mySimpit.toggleCAG(4);
    }
    if (Input.getVirtualPin(VPIN_CAG5) == ON)
    {
        printDebug("CAG5 toggled");
        mySimpit.toggleCAG(5);
    }
    if (Input.getVirtualPin(VPIN_CAG6) == ON)
    {
        printDebug("CAG6 toggled");
        mySimpit.toggleCAG(6);
    }
    if (Input.getVirtualPin(VPIN_CAG7) == ON)
    {
        printDebug("CAG7 toggled");
        mySimpit.toggleCAG(7);
    }    if (Input.getVirtualPin(VPIN_CAG8) == ON)
    {
        printDebug("CAG8 toggled");
        mySimpit.toggleCAG(8);
    }
    if (Input.getVirtualPin(VPIN_CAG9) == ON)
    {
        printDebug("CAG9 toggled");
        mySimpit.toggleCAG(9);
    }
    if (Input.getVirtualPin(VPIN_CAG10) == ON)
    {
        printDebug("CAG10 toggled");
        mySimpit.toggleCAG(10);
    }


    Output.setLED(CAG1_LED, cagStatusMsg.is_action_activated(1));
    Output.setLED(CAG2_LED, cagStatusMsg.is_action_activated(2));
    Output.setLED(CAG3_LED, cagStatusMsg.is_action_activated(3));
    Output.setLED(CAG4_LED, cagStatusMsg.is_action_activated(4));
    Output.setLED(CAG5_LED, cagStatusMsg.is_action_activated(5));
    Output.setLED(CAG6_LED, cagStatusMsg.is_action_activated(6));
    Output.setLED(CAG7_LED, cagStatusMsg.is_action_activated(7));
    Output.setLED(CAG8_LED, cagStatusMsg.is_action_activated(8));
    Output.setLED(CAG9_LED, cagStatusMsg.is_action_activated(9));
    Output.setLED(CAG10_LED, cagStatusMsg.is_action_activated(10));
}

void refreshSAS()
{
    ButtonState sasSwitch = Input.getVirtualPin(VPIN_SAS_SWITCH);
    if (sasSwitch == ON)
    {
        printDebug("SAS ON");
        mySimpit.activateAction(SAS_ACTION);
    }
    else if (sasSwitch == OFF)
    {
        printDebug("SAS OFF");
        mySimpit.deactivateAction(SAS_ACTION);
    }
}
void refreshRCS()
{
    ButtonState rcsSwitch = Input.getVirtualPin(VPIN_RCS_SWITCH);
    if (rcsSwitch == ON)
    {
        printDebug("RCS ON");
        mySimpit.activateAction(RCS_ACTION);
    }
    else if (rcsSwitch == OFF)
    {
        printDebug("RCS OFF");
        mySimpit.deactivateAction(RCS_ACTION);
    }
}
void refreshAllSASModes()
{
    if (Input.getVirtualPin(VPIN_SAS_STABILITY_ASSIST_BUTTON) == ON)
    {
        printDebug("SAS: Stability Assist");
        mySimpit.setSASMode(AP_STABILITYASSIST);
    }
    if (Input.getVirtualPin(VPIN_SAS_MANEUVER_BUTTON) == ON)
    {
        printDebug("SAS: Maneuver");
        mySimpit.setSASMode(AP_MANEUVER);
    }
    if (Input.getVirtualPin(VPIN_SAS_PROGRADE_BUTTON) == ON)
    {
        printDebug("SAS: Prograde");
        mySimpit.setSASMode(AP_PROGRADE);
    }
    if (Input.getVirtualPin(VPIN_SAS_RETROGRADE_BUTTON) == ON)
    {
        printDebug("SAS: Retrograde");
        mySimpit.setSASMode(AP_RETROGRADE);
    }
    if (Input.getVirtualPin(VPIN_SAS_NORMAL_BUTTON) == ON)
    {
        printDebug("SAS: Normal");
        mySimpit.setSASMode(AP_NORMAL);
    }
    if (Input.getVirtualPin(VPIN_SAS_ANTI_NORMAL_BUTTON) == ON)
    {
        printDebug("SAS: Anti-Normal");
        mySimpit.setSASMode(AP_ANTINORMAL);
    }
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_IN_BUTTON) == ON)
    {
        printDebug("SAS: Radial In");
        mySimpit.setSASMode(AP_RADIALIN);
    }
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_OUT_BUTTON) == ON)
    {
        printDebug("SAS: Radial Out");
        mySimpit.setSASMode(AP_RADIALOUT);
    }
    if (Input.getVirtualPin(VPIN_SAS_TARGET_BUTTON) == ON)
    {
        printDebug("SAS: Target");
        mySimpit.setSASMode(AP_TARGET);
    }
    if (Input.getVirtualPin(VPIN_SAS_ANTI_TARGET_BUTTON) == ON)
    {
        printDebug("SAS: Anti-Target");
        mySimpit.setSASMode(AP_ANTITARGET);
    }
}

void refreshCamReset()
{
    if (Input.getVirtualPin(VPIN_CAM_RESET_BUTTON) == ON)
    {
        printDebug("Camera reset - backtick key");
        keyboardEmulatorMessage msg(0xC0);  // Backtick key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshCamMode()
{
    if (Input.getVirtualPin(VPIN_CAM_MODE_BUTTON) == ON)
    {
        printDebug("Camera mode changed - V key");
        keyboardEmulatorMessage msg(0x56); // V key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshFocus()
{
    if (Input.getVirtualPin(VPIN_FOCUS_BUTTON) == ON)
    {
        printDebug("Focus changed");
        keyboardEmulatorMessage msg(0xDD);  // ] key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshView()
{
    if (Input.getVirtualPin(VPIN_VIEW_SWITCH) != NOT_READY) // Switch toggles in both states
    {
        printDebug("Camera view cycled");
        keyboardEmulatorMessage msg(0x43);  // C key (toggle camera view)
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshNav()
{
    if (Input.getVirtualPin(VPIN_NAV_SWITCH) != NOT_READY) // Switch toggles in both states
    {
        printDebug("Map Toggled");
        keyboardEmulatorMessage msg(0x4D);  // M key (toggle map view)
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshUI()
{
    if (Input.getVirtualPin(VPIN_UI_BUTTON) == OFF)
    {
        printDebug("UI Toggled");
        keyboardEmulatorMessage msg(0x71);  // F2
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}

void refreshSoundSwitch()
{
    if (Input.getVirtualPin(VPIN_SOUND_SWITCH) != NOT_READY)
    {
        printDebug("Sound switch toggled");
        keyboardEmulatorMessage msg(0xDE);  // VK_OEM_7 = single/double quote key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}

void refreshAP()
{
    // Repurpose the physical-warp switch as an AUTOPILOT toggle.
    auto ap = Input.getVirtualPin(VPIN_AUTO_PILOT_SWITCH, true);
    if (ap == ON)
    {
        // Engage autopilot only if connected to KSP and in flight
        if (isConnectedToKSP && flightStatusMsg.isInFlight())
        {
            // Capture current telemetry as the hold targets
            // Use prograde (velocity) heading for more stable flight - less sensitive to vessel oscillations
            autopilotHeading = vesselPointingMsg.surfaceVelocityHeading;
            autopilotSpeed = velocityMsg.surface;
            autopilotAltitude = altitudeMsg.sealevel;
            autopilotEngageTime = millis();
            autopilotEnabled = true;

            // Save current SAS state and enable SAS for autopilot
            sasWasOnBeforeAutopilot = ag.isSAS;
            mySimpit.activateAction(SAS_ACTION);

            mySimpit.printToKSP("Autopilot Engaged", PRINT_TO_SCREEN);
        }
        else
        {
            printDebug("Cannot engage autopilot");
        }
    }
    else if (ap == OFF)
    {
        if (autopilotEnabled)
        {
            autopilotEnabled = false;
            
            // Restore SAS to its previous state
            sasWasOnBeforeAutopilot ? mySimpit.activateAction(SAS_ACTION) : mySimpit.deactivateAction(SAS_ACTION);
            
            mySimpit.printToKSP("Autopilot DISENGAGED", PRINT_TO_SCREEN);
            printDebug("Autopilot disengaged");
        }
    }
}

void refreshWarp()
{
    timewarpMessage twMsg;

    // If state just set off, cancel warp
    if (Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH) == OFF)
    {
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
    }
    // Cancel warp button, always allowed
    if (Input.getVirtualPin(VPIN_CANCEL_WARP_BUTTON) == ON)
    {
        printDebug("Warp cancelled");
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }

    // exit, until unlocked
    if (Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH, false) == OFF)
    {
        return;
    }

    if (Input.getVirtualPin(VPIN_INCREASE_WARP_BUTTON) == ON) 
    {
        printDebug("Warp increased");
        twMsg.command = TIMEWARP_UP;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
    } 
    if (Input.getVirtualPin(VPIN_DECREASE_WARP_BUTTON) == ON) 
    {
        printDebug("Warp decreased");
        twMsg.command = TIMEWARP_DOWN;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
    }
}
void refreshPause()
{
    if (Input.getVirtualPin(VPIN_PAUSE_BUTTON) == ON)
    {
        printDebug("Pause toggled");
        keyboardEmulatorMessage msg(0x1B);  // ESC key (toggle pause)
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}

void setSASModeLEDs()
{
    // Turn off all SAS mode LEDs first
    Output.setLED(SAS_STABILITY_ASSIST_LED, false);
    Output.setLED(SAS_MANEUVER_LED, false);
    Output.setLED(SAS_PROGRADE_LED, false);
    Output.setLED(SAS_RETROGRADE_LED, false);
    Output.setLED(SAS_NORMAL_LED, false);
    Output.setLED(SAS_ANTI_NORMAL_LED, false);
    Output.setLED(SAS_RADIAL_IN_LED, false);
    Output.setLED(SAS_RADIAL_OUT_LED, false);
    Output.setLED(SAS_TARGET_LED, false);
    Output.setLED(SAS_ANTI_TARGET_LED, false);

    // Only show active SAS mode if SAS is actually enabled
    if (ag.isSAS)
    {
        switch (sasInfoMsg.currentSASMode)
        {
        case AP_STABILITYASSIST:
            Output.setLED(SAS_STABILITY_ASSIST_LED, true);
            break;
        case AP_PROGRADE:
            Output.setLED(SAS_PROGRADE_LED, true);
            break;
        case AP_RETROGRADE:
            Output.setLED(SAS_RETROGRADE_LED, true);
            break;
        case AP_NORMAL:
            Output.setLED(SAS_NORMAL_LED, true);
            break;
        case AP_ANTINORMAL:
            Output.setLED(SAS_ANTI_NORMAL_LED, true);
            break;
        case AP_RADIALIN:
            Output.setLED(SAS_RADIAL_IN_LED, true);
            break;
        case AP_RADIALOUT:
            Output.setLED(SAS_RADIAL_OUT_LED, true);
            break;
        case AP_TARGET:
            Output.setLED(SAS_TARGET_LED, true);
            break;
    
        case AP_ANTITARGET:
            Output.setLED(SAS_ANTI_TARGET_LED, true);
            break;
        case AP_MANEUVER:
            Output.setLED(SAS_MANEUVER_LED, true);
            break;
        }
    }
}
void refreshRotationButton()
{
    if (Input.getVirtualPin(VPIN_ROTATION_BUTTON) == ON)
    {
        printDebug("Rotation button - HOME key");
        keyboardEmulatorMessage msg(0x24);  // HOME key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshJump()
{
    if (Input.getVirtualPin(VPIN_ROTATION_BUTTON) == ON)
    {
        printDebug("Rotation button - SPACE key");
        
        keyboardEmulatorMessage msg(0x20);  // Space key
        mySimpit.send(KEYBOARD_EMULATOR, msg);
    }
}
void refreshGrab()
{
    // Check if we're in EVAcccmmcc
    bool inEVA = flightStatusMsg.isInEVA();
    bool inFlight = flightStatusMsg.isInFlight();
    
    if (Input.getVirtualPin(VPIN_GRAB_BUTTON) == ON)
    {
        if (inEVA)
        {
            // EVA Grab using F key (VK code 0x46)
            printDebug("EVA Grab");
            
            keyboardEmulatorMessage msg(0x46);  // F key (EVA grab)
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (inFlight && !inEVA)
        {
            // In flight (not EVA): Decrease precision mode
            precisionModifier -= PRECISION_STEP;
            if (precisionModifier < MIN_PRECISION_MODIFIER)
                precisionModifier = MIN_PRECISION_MODIFIER;
            
            mySimpit.printToKSP("Precision: " + String((int)(precisionModifier * 100)) + "%", PRINT_TO_SCREEN);
            printDebug("Precision decreased to " + String((int)(precisionModifier * 100)) + "%");
            
            // Trigger precision display overlay
            precisionDisplayActive = true;
            precisionDisplayStartTime = millis();
        }
    }
    
}
void refreshBoard()
{
    // Check if we're in EVA
    bool inEVA = flightStatusMsg.isInEVA();
    bool inFlight = flightStatusMsg.isInFlight();
    
    // Only trigger on button press (transition from OFF to ON)
    if (Input.getVirtualPin(VPIN_BOARD_BUTTON) == ON)
    {
        if (inEVA)
        {
            // EVA Board using B key (VK code 0x42)
            printDebug("EVA Board");
            
            keyboardEmulatorMessage msg(0x42);  // B key (EVA board)
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (inFlight && !inEVA)
        {
            // In flight (not EVA): Increase precision mode
            precisionModifier += PRECISION_STEP;
            if (precisionModifier > MAX_PRECISION_MODIFIER)
                precisionModifier = MAX_PRECISION_MODIFIER;
            
            mySimpit.printToKSP("Precision: " + String((int)(precisionModifier * 100)) + "%", PRINT_TO_SCREEN);
            printDebug("Precision increased to " + String((int)(precisionModifier * 100)) + "%");
            
            // Trigger precision display overlay
            precisionDisplayActive = true;
            precisionDisplayStartTime = millis();
        }
    }
}

void refreshThrottle()
{
    static int16_t lastThrottle = 0;  // Remember last throttle value
    
    // If autopilot is enabled, attempt to actively control throttle to maintain speed/altitude
    if (autopilotEnabled)
    {
        
        // Throttle control: Only for speed adjustment
        // Altitude is controlled by pitch offset (1° up/down to adjust prograde)
        if (isConnectedToKSP)
        {
            float currentSpeed = velocityMsg.surface;
            float speedError = autopilotSpeed - currentSpeed; // m/s (positive = need more speed)

            // Determine whether altitude or heading need priority. If so, avoid aggressive speed matching.
            float currentAlt = altitudeMsg.sealevel;
            float altErr = autopilotAltitude - currentAlt; // meters
            float currentHeading = vesselPointingMsg.surfaceVelocityHeading;
            float hdgErr = shortestAngleDiff(autopilotHeading, currentHeading);

            // Dynamic base throttle that adapts to find equilibrium
            static float baseThrottle = 0.5; // Start at 50%
            
            // Adapt base throttle continuously when stable
            if (abs(speedError) < 10.0 && abs(altErr) <= AUTOPILOT_ALT_PRIORITY_THRESHOLD && abs(hdgErr) <= AUTOPILOT_HEADING_PRIORITY_THRESHOLD)
            {
                // Continuously adjust base throttle based on speed error
                if (speedError > 0.5) {
                    baseThrottle += AP_THROTTLE_ADAPT_RATE; // Need more throttle
                } else if (speedError < -0.5) {
                    baseThrottle -= AP_THROTTLE_ADAPT_RATE; // Need less throttle
                }
                // Allow full range for base throttle
                if (baseThrottle < 0.0) baseThrottle = 0.0;
                if (baseThrottle > 1.0) baseThrottle = 1.0;
            }

            float throttleFraction;
            if (abs(altErr) > AUTOPILOT_ALT_PRIORITY_THRESHOLD || abs(hdgErr) > AUTOPILOT_HEADING_PRIORITY_THRESHOLD)
            {
                // Prioritize altitude/heading: keep base throttle
                throttleFraction = baseThrottle;
            }
            else
            {
                // Base throttle + aggressive speed correction using full range
                throttleFraction = baseThrottle + (AP_SPEED_K * speedError);
            }

            // Allow full throttle range from 0 to 1
            if (throttleFraction < 0.0f) throttleFraction = 0.0f;
            if (throttleFraction > 1.0f) throttleFraction = 1.0f;

            // Smooth throttle changes to avoid abrupt commands
            static float lastThrottleFraction = 0.3;
            float alpha = THROTTLE_SMOOTH_ALPHA;
            float smoothed = lastThrottleFraction * (1.0f - alpha) + throttleFraction * alpha;
            lastThrottleFraction = smoothed;

            int16_t apThrottle = (int16_t)(smoothed * (float)INT16_MAX);
            throttleMessage throttleMsg;
            throttleMsg.throttle = apThrottle;
            mySimpit.send(THROTTLE_MESSAGE, throttleMsg);
        }
        // Block manual throttle while autopilot holds
        return;
    }

    // Only read and update throttle axis if throttle lock is ON
    if (Input.getVirtualPin(VPIN_THROTTLE_LOCK_SWITCH, false) == ON)
    {
        int axis = Input.getThrottleAxis();
        // overflow protection using calibrated values
        if (axis < throttleCalMin) axis = throttleCalMin;
        if (axis > throttleCalMax) axis = throttleCalMax;
        
        // Apply deadzone at minimum (snap to 0)
        if (axis < (throttleCalMin + THROTTLE_DEADZONE))
        {
            lastThrottle = 0;
        }
        // Apply deadzone at maximum (snap to 100%)
        else if (axis > (throttleCalMax - THROTTLE_DEADZONE))
        {
            lastThrottle = INT16_MAX;
        }
        else
        {
            // Map with adjusted range (excluding deadzones)
            lastThrottle = map(axis, 
                              throttleCalMin + THROTTLE_DEADZONE, 
                              throttleCalMax - THROTTLE_DEADZONE, 
                              0, INT16_MAX);
        }
        
        throttleMessage throttleMsg;
        throttleMsg.throttle = lastThrottle;
        if (isConnectedToKSP) mySimpit.send(THROTTLE_MESSAGE, throttleMsg);
    }
    // If lock is OFF, don't send any throttle updates (holds current position in KSP)
    
    // Debug output
    // Removed frequent throttle debug prints to reduce serial spam.
}
void refreshTranslation()
{
    // Translation button now toggles view mode instead of requiring hold
    // Check for button press to toggle state
    ButtonState btnState = Input.getVirtualPin(VPIN_TRANSLATION_BUTTON);
    if (btnState == ON)
    {
        viewModeEnabled = !viewModeEnabled;
        printDebug(viewModeEnabled ? "View mode ENABLED" : "View mode DISABLED");
    }
    
    // If view mode is enabled, use joystick for camera control
    if (viewModeEnabled)
    {
        // Use keyboard emulation for camera control (arrow keys) but send momentary key presses
        // Arrow key VK codes: Up=0x26, Down=0x28, Left=0x25, Right=0x27
        int x = Input.getTranslationXAxis();
        int y = Input.getTranslationYAxis();
        int z = Input.getTranslationZAxis();

        // X-axis: Left/Right camera - send single keypress like zoom
        if (x < (512 - CAMERA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x25); // Left
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (x > (512 + CAMERA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x27); // Right
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }

        // Y-axis: Up/Down camera (INVERTED - forward = look up, back = look down)
        if (y < (512 - CAMERA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x26); // Up
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (y > (512 + CAMERA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x28); // Down
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }

        // Z-axis: Zoom in/out using mouse wheel emulation
        // Using special VK codes: 0xFF01 = wheel up (zoom in), 0xFF02 = wheel down (zoom out)
        if (z < (512 - CAMERA_DEADZONE))
        {
            // Zoom out (Mouse wheel down) - joystick pulled back/down
            keyboardEmulatorMessage msg(0xFF02);  // Mouse wheel down
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (z > (512 + CAMERA_DEADZONE))
        {
            // Zoom in (Mouse wheel up) - joystick pushed forward/up
            keyboardEmulatorMessage msg(0xFF01);  // Mouse wheel up
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }

        return;
    }

    // Trim button - add current joystick position to existing trim (accumulative)
    // Use false parameter for immediate response without waiting for state change
    // NOTE: Button logic is inverted (hardware reports ON when released)
    static bool lastTransTrimState = false;
    bool currentTransTrimState = (Input.getVirtualPin(VPIN_TRANS_HOLD_BUTTON, false) == OFF); // INVERTED
    
    if (currentTransTrimState && !lastTransTrimState) // Detect rising edge (button just pressed)
    {
        // Capture current joystick values and add to existing trim
        int x = Input.getTranslationXAxis();
        int y = Input.getTranslationYAxis();
        int z = Input.getTranslationZAxis();
        
        int16_t newTrimX = mapTranslationX(x);
        int16_t newTrimY = mapTranslationY(y);
        int16_t newTrimZ = mapTranslationZ(z);
        
        if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
        {
            // Only apply precision scaling to flight translation controls.
            if (Input.getVirtualPin(VPIN_TRANSLATION_BUTTON, false) == OFF)
            {
                if (precisionModifier > 0 && precisionModifier < 1)
                {
                    newTrimX *= precisionModifier;
                    newTrimY *= precisionModifier;
                    newTrimZ *= precisionModifier;
                }
            }
        }
        
        // Add new trim to existing trim (accumulative) with overflow protection
        // Use int32_t for intermediate calculation to prevent overflow
        int32_t tempX = (int32_t)trimTransX + (int32_t)newTrimX;
        int32_t tempY = (int32_t)trimTransY + (int32_t)newTrimY;
        int32_t tempZ = (int32_t)trimTransZ + (int32_t)newTrimZ;
        
        // Clamp to valid int16_t range
        trimTransX = (tempX > INT16_MAX) ? INT16_MAX : ((tempX < INT16_MIN) ? INT16_MIN : (int16_t)tempX);
        trimTransY = (tempY > INT16_MAX) ? INT16_MAX : ((tempY < INT16_MIN) ? INT16_MIN : (int16_t)tempY);
        trimTransZ = (tempZ > INT16_MAX) ? INT16_MAX : ((tempZ < INT16_MIN) ? INT16_MIN : (int16_t)tempZ);
        
        printDebug("Translation TRIM adjusted");
    }
    lastTransTrimState = currentTransTrimState;
    
    // Reset button clears trim
    // NOTE: Button logic is inverted (hardware reports ON when released)
    static bool lastTransResetState = false;
    bool currentTransResetState = (Input.getVirtualPin(VPIN_TRANS_RESET_BUTTON, false) == OFF); // INVERTED
    
    if (currentTransResetState && !lastTransResetState) // Detect rising edge
    {
        trimTransX = 0;
        trimTransY = 0;
        trimTransZ = 0;
        printDebug("Translation TRIM reset");
    }
    lastTransResetState = currentTransResetState;

    // If autopilot is enabled, block translation inputs and allow joystick override to cancel autopilot
    if (autopilotEnabled)
    {
        int x = Input.getTranslationXAxis();
        int y = Input.getTranslationYAxis();
        int z = Input.getTranslationZAxis();
        const int OVERRIDE_THRESHOLD = 256;
        bool overrideDetected = (x < (512 - OVERRIDE_THRESHOLD) || x > (512 + OVERRIDE_THRESHOLD) ||
                                 y < (512 - OVERRIDE_THRESHOLD) || y > (512 + OVERRIDE_THRESHOLD) ||
                                 z < (512 - OVERRIDE_THRESHOLD) || z > (512 + OVERRIDE_THRESHOLD));
        bool delayElapsed = (millis() - autopilotEngageTime) >= HOLD_OVERRIDE_DELAY;
        if (delayElapsed && overrideDetected)
        {
            autopilotEnabled = false;
            
            // Restore SAS to its previous state
            if (sasWasOnBeforeAutopilot)
            {
                mySimpit.activateAction(SAS_ACTION);
            }
            else
            {
                mySimpit.deactivateAction(SAS_ACTION);
            }
            
            mySimpit.printToKSP("Autopilot DISENGAGED (joystick override)", PRINT_TO_SCREEN);
            printDebug("Autopilot cancelled by translation joystick override");
        }
        else
        {
            // Block translation messages while autopilot holds
            return;
        }
    }


    
    // If we got here, we're not in hold mode anymore (or override just cancelled it)
    // Continue with normal operation below
        
    // Normal operation - read current joystick values
    int x = Input.getTranslationXAxis();
    int y = Input.getTranslationYAxis();
    int z = Input.getTranslationZAxis();

    int16_t transX = mapTranslationX(x);
    int16_t transY = mapTranslationY(y);
    int16_t transZ = mapTranslationZ(z);

    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        // Only apply precision scaling to flight translation controls (not when using camera button)
        if (Input.getVirtualPin(VPIN_TRANSLATION_BUTTON, false) == OFF)
        {
            if (precisionModifier > 0 && precisionModifier < 1)
            {
                transX *= precisionModifier;
                transY *= precisionModifier;
                transZ *= precisionModifier;
            }
        }
    }

    // Add trim offsets to joystick input with overflow protection
    int32_t tempX = (int32_t)transX + (int32_t)trimTransX;
    int32_t tempY = (int32_t)transY + (int32_t)trimTransY;
    int32_t tempZ = (int32_t)transZ + (int32_t)trimTransZ;
    
    // Clamp to valid range
    transX = (tempX > INT16_MAX) ? INT16_MAX : ((tempX < INT16_MIN) ? INT16_MIN : (int16_t)tempX);
    transY = (tempY > INT16_MAX) ? INT16_MAX : ((tempY < INT16_MIN) ? INT16_MIN : (int16_t)tempY);
    transZ = (tempZ > INT16_MAX) ? INT16_MAX : ((tempZ < INT16_MIN) ? INT16_MIN : (int16_t)tempZ);

    // In shared control mode, translation joystick is used for rotation - don't send translation message
    // UNLESS view mode is enabled (camera controls take priority over dual player mode)
    if (Input.getVirtualPin(VPIN_DUAL_SWITCH, false) == ON)
        return;

    translationMessage transMsg;
    transMsg.setXYZ(transX, transY, transZ);  // Swapped Y and Z
    if (isConnectedToKSP) mySimpit.send(TRANSLATION_MESSAGE, transMsg);
}

bool handleAutopilotRotation()
{
    if (!autopilotEnabled)
        return false;
    
    int x = Input.getRotationXAxis();
    int y = Input.getRotationYAxis();
    int z = Input.getRotationZAxis();
    const int OVERRIDE_THRESHOLD = 256;
    bool overrideDetected = (x < (512 - OVERRIDE_THRESHOLD) || x > (512 + OVERRIDE_THRESHOLD) ||
                             y < (512 - OVERRIDE_THRESHOLD) || y > (512 + OVERRIDE_THRESHOLD) ||
                             z < (512 - OVERRIDE_THRESHOLD) || z > (512 + OVERRIDE_THRESHOLD));
    bool delayElapsed = (millis() - autopilotEngageTime) >= HOLD_OVERRIDE_DELAY;
    
    if (delayElapsed && overrideDetected)
    {
        autopilotEnabled = false;
        
        // Restore SAS to its previous state
        if (sasWasOnBeforeAutopilot)
        {
            mySimpit.activateAction(SAS_ACTION);
        }
        else
        {
            mySimpit.deactivateAction(SAS_ACTION);
        }
        
        mySimpit.printToKSP("Autopilot DISENGAGED (joystick override)", PRINT_TO_SCREEN);
        printDebug("Autopilot cancelled by rotation joystick override");
        return false;
    }
    
    // Active autopilot corrections
    if (isConnectedToKSP)
    {
        // Heading control (yaw) - use prograde (velocity) heading for stability
        float currentHeading = vesselPointingMsg.surfaceVelocityHeading;
        float hdgErr = shortestAngleDiff(autopilotHeading, currentHeading);
        
        float yawFrac = 0.0f;
        if (abs(hdgErr) > 2.0f)
        {
            yawFrac = hdgErr * AP_HEADING_K;
            if (yawFrac > 1.0f) yawFrac = 1.0f;
            if (yawFrac < -1.0f) yawFrac = -1.0f;
        }
        int16_t yawVal = (int16_t)(yawFrac * (float)INT16_MAX);

        // Pitch control: Monitor prograde pitch and adjust vessel pitch to correct it
        float progradePitch = vesselPointingMsg.surfaceVelocityPitch;
        float currentAlt = altitudeMsg.sealevel;
        float altErr = autopilotAltitude - currentAlt;

        // Determine target prograde pitch based on altitude error
        float targetProgradePitch = 0.0f;
        if (abs(altErr) > 8.0f)
        {
            float pitchOffset = altErr / 15.0f;
            if (pitchOffset > 12.0f) pitchOffset = 12.0f;
            if (pitchOffset < -12.0f) pitchOffset = -12.0f;
            targetProgradePitch = pitchOffset;
        }
        
        float progradeErr = targetProgradePitch - progradePitch;
        
        float pitchFrac = 0.0f;
        if (abs(progradeErr) > 0.3f)
        {
            float gain = (abs(progradeErr) > 2.0f) ? 0.08 : 0.05;
            pitchFrac = progradeErr * gain;
            float maxFrac = (abs(progradeErr) > 2.0f) ? 0.35f : 0.20f;
            if (pitchFrac > maxFrac) pitchFrac = maxFrac;
            if (pitchFrac < -maxFrac) pitchFrac = -maxFrac;
        }
        int16_t pitchVal = (int16_t)(pitchFrac * (float)INT16_MAX);

        // Roll hold: keep roll at zero (level wings)
        float currentRoll = vesselPointingMsg.roll;
        float rollErr = shortestAngleDiff(0.0f, currentRoll);
        
        float rollFrac = 0.0f;
        if (abs(rollErr) > 1.0f)
        {
            rollFrac = rollErr * AP_ROLL_K;
            if (rollFrac > 1.0f) rollFrac = 1.0f;
            if (rollFrac < -1.0f) rollFrac = -1.0f;
        }
        int16_t rollVal = (int16_t)(-rollFrac * (float)INT16_MAX);

        // Compose and send rotation
        rotationMessage rotMsg;
        rotMsg.setPitchRollYaw(pitchVal, rollVal, yawVal);
        mySimpit.send(ROTATION_MESSAGE, rotMsg);
    }
    
    return true;
}

void refreshRotation()
{
    bool inEVA = flightStatusMsg.isInEVA();
    // In EVA mode, use rotation joystick for WASD movement
    if (inEVA)
    {
        const int EVA_DEADZONE = 100;
        int x = Input.getRotationXAxis();
        int y = Input.getRotationYAxis();
        
        if (x < (512 - EVA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x41); // A Key
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (x > (512 + EVA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x44); // D Key
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        
        if (y < (512 - EVA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x57); // W Key
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        else if (y > (512 + EVA_DEADZONE))
        {
            keyboardEmulatorMessage msg(0x53); // S Key
            mySimpit.send(KEYBOARD_EMULATOR, msg);
        }
        
        return; // EVA so exit
    }
    
    // NOTE: Button logic is inverted (hardware reports ON when released)
    static bool lastRotTrimState = false;
    bool currentRotTrimState = (Input.getVirtualPin(VPIN_ROT_HOLD_BUTTON, false) == OFF); // INVERTED
    
    if (currentRotTrimState && !lastRotTrimState) // Detect rising edge (button just pressed)
    {
        // Capture current joystick values and add to existing trim
        int x = Input.getRotationXAxis();
        int y = Input.getRotationYAxis();
        int z = Input.getRotationZAxis();
        
        int16_t newTrimX = mapRotationX(x);
        int16_t newTrimY = mapRotationY(y);
        int16_t newTrimZ = mapRotationZ(z);
        
        if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
        {
            // Only apply precision scaling to flight rotation controls.
            if (Input.getVirtualPin(VPIN_ROTATION_BUTTON, false) == OFF)
            {
                if (precisionModifier > 0 && precisionModifier < 1)
                {
                    newTrimX *= precisionModifier;
                    newTrimY *= precisionModifier;
                    newTrimZ *= precisionModifier;
                }
            }
        }
        
        // Add new trim to existing trim (accumulative) with overflow protection
        // Use int32_t for intermediate calculation to prevent overflow
        int32_t tempX = (int32_t)trimRotX + (int32_t)newTrimX;
        int32_t tempY = (int32_t)trimRotY + (int32_t)newTrimY;
        int32_t tempZ = (int32_t)trimRotZ + (int32_t)newTrimZ;
        
        // Clamp to valid int16_t range
        trimRotX = (tempX > INT16_MAX) ? INT16_MAX : ((tempX < INT16_MIN) ? INT16_MIN : (int16_t)tempX);
        trimRotY = (tempY > INT16_MAX) ? INT16_MAX : ((tempY < INT16_MIN) ? INT16_MIN : (int16_t)tempY);
        trimRotZ = (tempZ > INT16_MAX) ? INT16_MAX : ((tempZ < INT16_MIN) ? INT16_MIN : (int16_t)tempZ);
        
        printDebug("Rotation TRIM adjusted");
    }
    lastRotTrimState = currentRotTrimState;
    
    // Reset button clears trim
    // NOTE: Button logic is inverted (hardware reports ON when released)
    static bool lastRotResetState = false;
    bool currentRotResetState = (Input.getVirtualPin(VPIN_ROT_RESET_BUTTON, false) == OFF); // INVERTED
    
    if (currentRotResetState && !lastRotResetState) // Detect rising edge
    {
        trimRotX = 0;
        trimRotY = 0;
        trimRotZ = 0;
        printDebug("Rotation TRIM reset");
    }
    lastRotResetState = currentRotResetState;
        
    // If autopilot is enabled, handle it and return if still active
    if (handleAutopilotRotation())
        return;
    
    // Continue with normal operation below
    
    // Check if shared control mode is enabled (UI switch)
    bool sharedControlMode = (Input.getVirtualPin(VPIN_DUAL_SWITCH, false) == ON && !viewModeEnabled);
    
    // Read rotation joystick (Player 1)
    int x = Input.getRotationXAxis();
    int y = Input.getRotationYAxis();
    int z = Input.getRotationZAxis();
    

    int16_t x1 = mapRotationX(x);
    int16_t y1 = mapRotationY(y);
    int16_t z1 = mapRotationZ(z);

    int16_t rotX;
    int16_t rotY;
    int16_t rotZ;

    if (sharedControlMode) // dual player rotation control
    {  
        int16_t x2 = smoothAndMapAxis(Input.getTranslationXAxis(), false);
        int16_t y2 = smoothAndMapAxis(Input.getTranslationYAxis(), false);
        int16_t z2 = smoothAndMapAxis(Input.getTranslationZAxis(), false);
        rotX = dualPlayerHelper(x1, x2);
        rotY = dualPlayerHelper(y1, y2);
        rotZ = dualPlayerHelper(z1, z2);
    }
    else // regular user rotation control
    {
        rotX = x1;
        rotY = y1;
        rotZ = z1;
    }


    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        // Only apply precision scaling to flight rotation controls (not when rotation button is held for camera/EVA)
        if (Input.getVirtualPin(VPIN_ROTATION_BUTTON, false) == OFF)
        {
            if (precisionModifier > 0 && precisionModifier < 1)
            {
                rotX *= precisionModifier;
                rotY *= precisionModifier;
                rotZ *= precisionModifier;
            }
        }
    }
    
    // Add trim offsets to joystick input with overflow protection
    int32_t tempX = (int32_t)rotX + (int32_t)trimRotX;
    int32_t tempY = (int32_t)rotY + (int32_t)trimRotY;
    int32_t tempZ = (int32_t)rotZ + (int32_t)trimRotZ;
    
    // Clamp to valid range
    rotX = (tempX > INT16_MAX) ? INT16_MAX : ((tempX < INT16_MIN) ? INT16_MIN : (int16_t)tempX);
    rotY = (tempY > INT16_MAX) ? INT16_MAX : ((tempY < INT16_MIN) ? INT16_MIN : (int16_t)tempY);
    rotZ = (tempZ > INT16_MAX) ? INT16_MAX : ((tempZ < INT16_MIN) ? INT16_MIN : (int16_t)tempZ);

    // Apply roll sensitivity scaling to roll for less sensitive control (on final value)
    // If we're not in atmosphere (space), give full roll authority and do NOT apply sensitivity.
    if (atmoConditionsMsg.isVesselInAtmosphere())
    {
        rotX *= ROLL_SENSITIVITY;
    }
    
    rotationMessage rotMsg;
    wheelMessage wheelMsg;
    
    rotMsg.setPitchRollYaw(rotY, rotX, rotZ);
    // Send wheel steering directly
    wheelMsg.setSteer(smoothAndMapAxis(z, true)); // Negate to match expected direction
    if (isConnectedToKSP) {
        mySimpit.send(ROTATION_MESSAGE, rotMsg);
        mySimpit.send(WHEEL_MESSAGE, wheelMsg);
    }
}

// Display
void setSpeedLCD()
{
    // Speed
    float speed;
    float verticalSpeed;
    String topTxt = "";
    String botTxt = "";
    
    // Check for overspeed and stall warnings
    float surfaceSpeed = velocityMsg.surface;
    float radarAlt = altitudeMsg.surface;
    bool isOverspeed = (surfaceSpeed > 900.0 && radarAlt < 15000.0); // Match mod: 900 m/s, 15km altitude
    
    // Stall warning: only when under 100 mph (44.7 m/s) AND gear is up
    // Subtract vertical velocity to get horizontal component (ignore vertical descent)
    const float STALL_SPEED_MPH = 100.0;
    const float STALL_SPEED_MS = STALL_SPEED_MPH * 0.44704; // 44.7 m/s
    bool gearUp = !ag.isGear;
    float vertSpeed = velocityMsg.vertical;
    float horizontalSpeed = sqrt(max(0.0f, surfaceSpeed * surfaceSpeed - vertSpeed * vertSpeed));
    bool isStall = (gearUp && horizontalSpeed < STALL_SPEED_MS);
    
    // Blink the display if overspeed or stall warning is active
    bool blinkState = (millis() / 500) % 2; // Blink every 500ms

    // Normal display when not blinking or no warnings
    // Top line: Always show reference velocity based on current speed mode
    switch (currentSpeedMode)
    {
    case SPEED_SURFACE_MODE:
        speed = velocityMsg.surface;
        topTxt += "SRF-SPD ";
        break;
    case SPEED_ORBIT_MODE:
        speed = velocityMsg.orbital;
        topTxt += "ORB-SPD ";
        break;
    case SPEED_TARGET_MODE:
        speed = targetMsg.velocity;
        topTxt += "TGT-SPD ";
        break;
    default:
        speed = 0;
        topTxt += "--- ";
        break;
    }
    // Convert speed to imperial if needed - only change units if number won't fit
    String speedUnit = "m/s";
    if (useImperialUnits) {
        // Convert m/s to mph for imperial display
        speed *= 2.23693629; // m/s to mph
        speedUnit = "mph";
        // Only use mi/s if mph won't fit (>99999)
        if (abs(speed) >= 99999.0) {
            speed /= 3600.0; // Convert mph to mi/s
            speedUnit = "mi/s";
            topTxt += formatNumber(speed, 4, true, false);
        } else {
            topTxt += formatNumber(speed, 5, true, false);
        }
    } else {
        // Metric: only use km/s if m/s won't fit (>9999)
        if (abs(speed) >= 9999.0) {
            speed /= 1000.0; // Convert m/s to km/s
            speedUnit = "km/s";
            topTxt += formatNumber(speed, 4, true, false);
        } else {
            topTxt += formatNumber(speed, 5, true, false);
        }
    }
    topTxt += speedUnit;

    // Bottom line: Always show vertical velocity - only change units if won't fit
    verticalSpeed = velocityMsg.vertical;
    String vertUnit = "m/s";
    if (useImperialUnits) {
        // Use mph for vertical/horizontal consistency
        verticalSpeed *= 2.23693629; // m/s to mph
        vertUnit = "mph";
        // Only use mi/s if mph won't fit (>99999)
        if (abs(verticalSpeed) >= 99999.0) {
            verticalSpeed /= 3600.0; // Convert mph to mi/s
            vertUnit = "mi/s";
            botTxt += "VRT-SPD ";
            botTxt += formatNumber(verticalSpeed, 4, true, false);
        } else {
            botTxt += "VRT-SPD ";
            botTxt += formatNumber(verticalSpeed, 5, true, false);
        }
    } else {
        // Metric: only use km/s if m/s won't fit (>9999)
        if (abs(verticalSpeed) >= 9999.0) {
            verticalSpeed /= 1000.0; // Convert m/s to km/s
            vertUnit = "km/s";
            botTxt += "VRT-SPD ";
            botTxt += formatNumber(verticalSpeed, 4, true, false);
        } else {
            botTxt += "VRT-SPD ";
            botTxt += formatNumber(verticalSpeed, 5, true, false);
        }
    }
    botTxt += vertUnit;

    // Replace bottom line with warning if active and blinking
    if ((isOverspeed || isStall) && blinkState)
    {
        if (isOverspeed)
        {
            // Show OVERSPEED on bottom line with proper units
            float displaySpeed = surfaceSpeed;
            String speedUnit = "m/s";
            if (useImperialUnits) {
                displaySpeed *= 2.23693629; // m/s to mph
                speedUnit = "mph";
            }
            botTxt = "OVERSPEED " + formatNumber(displaySpeed, 5, true, false) + speedUnit;
        }
        else if (isStall)
        {
            // Show STALL on bottom line with proper units
            float displaySpeed = surfaceSpeed;
            String speedUnit = "m/s";
            if (useImperialUnits) {
                displaySpeed *= 2.23693629; // m/s to mph
                speedUnit = "mph";
            }
            botTxt = "STALL " + formatNumber(displaySpeed, 5, true, false) + speedUnit;
        }
    }

    Output.setSpeedLCD(topTxt, botTxt);
}
void setAltitudeLCD()
{
    
    String topTxt = "";
    String botTxt = "";

    // Check if precision display overlay is active
    if (precisionDisplayActive)
    {
        topTxt = "PRECISION MODE";
        int precisionPercent = (int)(precisionModifier * 100);
        botTxt = String(precisionPercent) + "%";
        
        Output.setAltitudeLCD(topTxt, botTxt);
        return;
    }

    // Top line: SOI name
    String soiName = (soi.length() > 0) ? soi : "Unknown";
    String atmLabel = atmoConditionsMsg.isVesselInAtmosphere() ? "ATMOS" : "VACUUM";
    topTxt = calculateGap(soiName, 16 - atmLabel.length());
    topTxt += atmLabel;

    // Bottom left: Radar or Sea label; Bottom right: altitude value (right-aligned)
    bool radarMode = (Input.getVirtualPin(VPIN_RADAR_ALTITUDE_SWITCH, false) == ON);
    if (radarMode)
        botTxt = "ALT-RAD ";
    else
        botTxt = "ALT-SEA ";

    // Choose altitude value and unit - only change units if number won't fit
    if (useImperialUnits) {
        float alt = radarMode ? altitudeMsg.surface : altitudeMsg.sealevel;
        float feet = alt * 3.28084;
        // Only use miles if feet won't fit (>999999 ft)
        if (feet >= 999999.0) {
            float miles = alt / 1609.34;
            // Show miles with 1 decimal place
            botTxt += formatNumber((int)miles, 4, true, false);
            botTxt += ".";
            int decimal = (int)((miles - (int)miles) * 10);
            botTxt += String(decimal);
            botTxt += "mi";
        } else {
            botTxt += formatNumber((int)feet, 6, true, false);
            botTxt += "ft";
        }
    } else {
        float alt = radarMode ? altitudeMsg.surface : altitudeMsg.sealevel;
        // Only use Mm if km won't fit (>999999 km = 999999000 m)
        if (alt >= 999999000.0) {
            int altMm = (int)(alt / 1000000.0);
            botTxt += formatNumber(altMm, 5, true, false);
            botTxt += "Mm";
        } else if (alt >= 9999.0) { // Only use km if m won't fit (>9999 m)
            int altKm = (int)(alt / 1000.0);
            botTxt += formatNumber(altKm, 6, true, false);
            botTxt += "km";
        } else {
            botTxt += formatNumber((int)alt, 7, true, false);
            botTxt += "m";
        }
    }

    Output.setAltitudeLCD(topTxt, botTxt);
}
void setInfoLCD()
{
    String topTxt = "";
    String botTxt = "";
    // Helper: prefer fullLabel+value if it fits in 16 chars, otherwise use abbrev+value or value only
    auto composeLabelValue = [](const String &fullLabel, const String &abbrev, const String &value)->String {
        String cand = fullLabel;
        if (value.length() > 0) {
            cand += " ";
            cand += value;
        }
        if (cand.length() <= 16) return cand;

        cand = abbrev;
        if (value.length() > 0) {
            cand += " ";
            cand += value;
        }
        if (cand.length() <= 16) return cand;

        // Fallback: value only (or truncated)
        if (value.length() <= 16) return value;
        return value.substring(0, 16);
    };

    // Small helpers to format distances and speeds according to unit preference
    // Only convert units if the number won't fit in base units
    auto formatDistance = [&](float meters)->String {
        if (useImperialUnits) {
            int feet = (int)(meters * 3.28084);
            // Only use miles if feet won't fit (>99999 ft)
            if (feet > 99999) {
                float miles = meters / 1609.34;
                int whole = (int)miles;
                int frac = (int)(miles * 10) % 10;
                return String(whole) + "." + String(frac) + "mi";
            } else {
                return String(feet) + "ft";
            }
        } else {
            int m = (int)meters;
            // Only use km if meters won't fit (>99999 m)
            if (m > 99999) {
                int km = getKilometers((int)meters);
                return String(km) + "km";
            } else {
                return String(m) + "m";
            }
        }
    };

    auto formatSpeed = [&](float mps)->String {
        if (useImperialUnits) {
            int mph = (int)round(mps * 2.23693629);
            // Only convert if mph won't fit (>99999)
            if (mph > 99999) {
                float mps_imp = mph / 3600.0;
                return String((int)mps_imp) + " mi/s";
            } else {
                return String(mph) + " mph";
            }
        } else {
            int ms = (int)round(mps);
            // Only convert if m/s won't fit (>9999)
            if (ms > 9999) {
                float kms = mps / 1000.0;
                return String((int)kms) + " km/s";
            } else {
                return String(ms) + " m/s";
            }
        }
    };

    // Display data based on current info mode (1-12)
    switch (infoMode)
    {
        case 1:  // Apoapsis Time and Altitude
        {
            topTxt = "Ap ";
            float apAltMeters = (float)apsidesMsg.apoapsis; // meters
            String altStr = formatDistance(apAltMeters);
            // Right-align value area (reserve 13 chars)
            topTxt += calculateGap(altStr, 13);
            topTxt += altStr;

            if (apsidesTimeMsg.apoapsis >= 0)
            {
                int timeToAp = apsidesTimeMsg.apoapsis;
                int hours = timeToAp / 3600;
                int minutes = (timeToAp % 3600) / 60;
                int seconds = timeToAp % 60;
                String timeStr;
                if (hours > 0)
                    timeStr = String(hours) + "h " + String(minutes) + "m";
                else
                    timeStr = String(minutes) + "m " + String(seconds) + "s";
                botTxt = "Time to " + timeStr;
            }
            else
                botTxt = "Time to N/A";
            break;
        }

        case 2:  // Periapsis Time and Altitude
        {
            topTxt = "Pe ";
            float peAltMeters = (float)apsidesMsg.periapsis; // meters (can be negative)
            String altStr = formatDistance(abs(peAltMeters));
            topTxt += calculateGap(altStr, 13);
            topTxt += altStr;

            if (apsidesTimeMsg.periapsis >= 0)
            {
                int timeToPe = apsidesTimeMsg.periapsis;
                int hours = timeToPe / 3600;
                int minutes = (timeToPe % 3600) / 60;
                int seconds = timeToPe % 60;
                String timeStr;
                if (hours > 0)
                    timeStr = String(hours) + "h " + String(minutes) + "m";
                else
                    timeStr = String(minutes) + "m " + String(seconds) + "s";
                botTxt = "Time to " + timeStr;
            }
            else
                botTxt = "Time to N/A";
            break;
        }

        case 3:  // Time to Node and Node DeltaV (combined)
            if (maneuverMsg.timeToNextManeuver >= 0 && maneuverMsg.deltaVNextManeuver > 0)
            {
                // Top line: Time to node
                int timeToNode = (int)maneuverMsg.timeToNextManeuver;
                int hours = timeToNode / 3600;
                int minutes = (timeToNode % 3600) / 60;
                int seconds = timeToNode % 60;
                String timeStr;
                if (hours > 0)
                    timeStr = String(hours) + "h " + String(minutes) + "m";
                else
                    timeStr = String(minutes) + "m " + String(seconds) + "s";
                topTxt = "Node " + timeStr;
                
                // Bottom line: DeltaV
                botTxt = "dV " + formatSpeed(maneuverMsg.deltaVNextManeuver);
            }
            else
            {
                topTxt = "Maneuver Node";
                botTxt = "No Node";
            }
            break;

        case 4:  // Node DeltaV and Burn Time (combined)
            if (maneuverMsg.deltaVNextManeuver > 0)
            {
                // Top line: DeltaV
                topTxt = "dV " + formatSpeed(maneuverMsg.deltaVNextManeuver);
                
                // Bottom line: Burn time
                if (burnTimeMsg.stageBurnTime > 0)
                {
                    int burnTime = (int)burnTimeMsg.stageBurnTime;
                    int minutes = burnTime / 60;
                    int seconds = burnTime % 60;
                    if (minutes > 0)
                        botTxt = "Burn " + String(minutes) + "m " + String(seconds) + "s";
                    else
                        botTxt = "Burn " + String(seconds) + "s";
                }
                else
                {
                    botTxt = "Burn N/A";
                }
            }
            else
            {
                topTxt = "Node DeltaV";
                botTxt = "No Node";
            }
            break;

        case 5:  // Orbit Period + Eccentricity (paired)
        {
            // Build time string
            String timeStr = "N/A";
            if (orbitInfoMsg.period > 0)
            {
                int period = (int)orbitInfoMsg.period;
                int hours = period / 3600;
                int minutes = (period % 3600) / 60;
                int seconds = period % 60;
                String periodStr;
                if (hours > 0)
                    timeStr = String(hours) + "h " + String(minutes) + "m";
                else if (minutes > 0)
                    timeStr = String(minutes) + "m " + String(seconds) + "s";
                else
                    timeStr = String(seconds) + "s";
            }

            String eccStr = String(orbitInfoMsg.eccentricity, 3);

            // Prefer full words if they fit, otherwise use short labels
            topTxt = composeLabelValue("Orbit Period", "PRD      ", timeStr);
            botTxt = composeLabelValue("Eccentricity", "Ecc       ", eccStr);
            break;
        }

        case 6:  // LAN + Arg Periapsis (paired)
        {
            String lanStr = formatNumber((int)orbitInfoMsg.longAscendingNode, 11, false, false);
            lanStr += DEGREE_CHAR_LCD;
            String argStr = formatNumber((int)orbitInfoMsg.argPeriapsis, 6, false, false);
            argStr += DEGREE_CHAR_LCD;

            topTxt = composeLabelValue("Long Asc Node", "LAN", lanStr);
            botTxt = composeLabelValue("Arg Peri", "ARG", argStr);
            break;
        }

        case 7:  // Inclination + Ejection angle (true anomaly) (paired)
        {
            String incStr = formatNumber((int)orbitInfoMsg.inclination, 3, false, false);
            incStr += DEGREE_CHAR_LCD;
            String ejStr = formatNumber((int)orbitInfoMsg.trueAnomaly, 6, false, false);
            ejStr += DEGREE_CHAR_LCD;

            topTxt = composeLabelValue("Inclination", "INC", incStr);
            botTxt = composeLabelValue("Ejection", "EJ", ejStr);
            break;
        }

        case 8: // Remaining Flight Time (fuel-based)
            {
                // Top line: Time remaining at current burn rate
                float burnTime = burnTimeMsg.stageBurnTime; // seconds
                if (burnTime > 0)
                {
                    int hours = (int)burnTime / 3600;
                    int minutes = ((int)burnTime % 3600) / 60;
                    int seconds = (int)burnTime % 60;
                    
                    String timeStr;
                    if (hours > 0)
                        timeStr = String(hours) + "h " + String(minutes) + "m";
                    else if (minutes > 0)
                        timeStr = String(minutes) + "m " + String(seconds) + "s";
                    else
                        timeStr = String(seconds) + "s";
                    topTxt = "Burn Time " + timeStr;
                    
                    // Bottom line: Distance possible at current speed with remaining fuel
                    float currentSpeed = velocityMsg.surface; // m/s
                    if (currentSpeed > 1.0)
                    {
                        float distancePossible = burnTime * currentSpeed; // meters
                        botTxt = "Range " + formatDistance(distancePossible);
                    }
                    else
                    {
                        botTxt = "Not Moving";
                    }
                }
                else
                {
                    topTxt = "Burn Time";
                    botTxt = "No Fuel Flow";
                }
            }
            break;

        case 9: // DeltaV (single mode, toggles between Total/Stage based on stage view switch)
        {
            bool stageView = (Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false) == ON);
            float dv = stageView ? deltaVMsg.stageDeltaV : deltaVMsg.totalDeltaV;
            topTxt = stageView ? "Stage DeltaV" : "Total DeltaV";
            botTxt = formatSpeed(dv);
            break;
        }

        case 10:  // Time-To-Impact and estimated distance
            {
                float verticalSpeed = velocityMsg.vertical;
                float surfaceAlt = altitudeMsg.surface;
                
                // Only show estimate if descending (negative vertical speed) and above ground
                if (verticalSpeed < -1.0 && surfaceAlt > 0)
                {
                    // Time to impact = altitude / abs(vertical speed)
                    int timeToImpact = (int)(surfaceAlt / -verticalSpeed);
                    int minutes = timeToImpact / 60;
                    int seconds = timeToImpact % 60;
                    
                    // Top line: Time-To-Impact
                    topTxt = "Time-To-Impact ";
                    if (minutes > 0)
                        topTxt += String(minutes) + "m" + String(seconds) + "s";
                    else
                        topTxt += String(seconds) + "s";
                    
                    // Bottom line: Estimated distance traveled during descent
                    float horizontalSpeed = velocityMsg.surface;
                    // Remove vertical component to get true horizontal speed
                    float vertSpeed = abs(verticalSpeed);
                    float trueHorizontalSpeed = sqrt(max(0.0f, horizontalSpeed * horizontalSpeed - vertSpeed * vertSpeed));
                    float estimatedDistance = trueHorizontalSpeed * timeToImpact;
                    
                    botTxt = "Est-Dist " + formatDistance(estimatedDistance);
                }
                else if (verticalSpeed >= 0)
                {
                    topTxt = "Time-To-Impact";
                    botTxt = "Ascending";
                }
                else
                {
                    topTxt = "Time-To-Impact";
                    botTxt = "On Ground";
                }
            }
            break;

        case 11:  // Target Distance and Velocity (combined)
            if (targetMsg.distance > 0)
            {
                // Top line: Target distance
                topTxt = "TGT " + formatDistance(targetMsg.distance);
                
                // Bottom line: Target velocity
                botTxt = "Vel " + formatSpeed(targetMsg.velocity);
            }
            else
            {
                topTxt = "Target Info";
                botTxt = "No Target";
            }
            break;

        case 12:  // TWR, ISP, Thrust info
        {
            // Note: KSP doesn't provide direct TWR, ISP, or Thrust via Simpit
            // We can calculate TWR if we had mass and thrust data
            // For now, show DeltaV info and burn time as closest available metrics
            float stageDV = deltaVMsg.stageDeltaV;
            float totalDV = deltaVMsg.totalDeltaV;
            
            // Top line: Stage and Total DeltaV - only convert to k if too large
            topTxt = "DV ";
            String stageStr, totalStr;
            
            // Try to fit stage DV in m/s first
            if (stageDV < 10000.0) {
                stageStr = String((int)stageDV);
            } else {
                // Only use k notation if >= 10000
                stageStr = String((int)(stageDV / 1000.0)) + "." + String(((int)stageDV % 1000) / 100) + "k";
            }
            
            // Try to fit total DV in m/s first
            if (totalDV < 10000.0) {
                totalStr = String((int)totalDV);
            } else {
                // Only use k notation if >= 10000
                totalStr = String((int)(totalDV / 1000.0)) + "." + String(((int)totalDV % 1000) / 100) + "k";
            }
            
            topTxt += stageStr + "/" + totalStr;
            
            // Only add unit if there's room (16 chars total)
            if (topTxt.length() <= 11) {
                topTxt += " m/s";
            }
            
            // Bottom line: Burn time if available
            float burnTime = burnTimeMsg.stageBurnTime;
            if (burnTime > 0) {
                int minutes = (int)burnTime / 60;
                int seconds = (int)burnTime % 60;
                if (minutes > 0)
                    botTxt = "Burn " + String(minutes) + "m " + String(seconds) + "s";
                else
                    botTxt = "Burn " + String(seconds) + "s";
            } else {
                botTxt = "TWR/ISP/Thr N/A";
            }
            break;
        }

        default:
            topTxt = "Info Mode";
            botTxt = "Select Mode";
            break;
    }

    Output.setInfoLCD(topTxt, botTxt);
}
void setHeadingLCD()
{
    String topTxt = "";
    String botTxt = "";

    // Top line: Roll value, Compass direction, and G-force
    // Build left portion (roll) - negated so negative is left
    String left = "RLL ";
    left += formatNumber((int)(-vesselPointingMsg.roll), 3, true, false);
    left += DEGREE_CHAR_LCD;

    // Compass direction (N, NE, E, SE, S, SW, W, NW)
    String compass = getCardinalDirection((int)vesselPointingMsg.heading);
    
    // G-force from airspeed message (display as X.XG)
    float gforce = airspeedMsg.gForces;
    String gStr = String(gforce, 1) + "G";

    // Build top line: "RLL +XXX° CCC X.XG" (compass before G-force)
    String right = compass + " " + gStr;
    int spaceForLeft = 16 - right.length();
    if (spaceForLeft < 0) spaceForLeft = 0;
    if (left.length() > (unsigned int)spaceForLeft) left = left.substring(0, spaceForLeft);
    topTxt = calculateGap(left, spaceForLeft) + right;
    
    // Bottom line: Heading (left) and Pitch (right)
    botTxt = "HDG ";
    botTxt += formatNumber(vesselPointingMsg.heading, 3, false, false);
    botTxt += DEGREE_CHAR_LCD;
    botTxt += " PTH";
    botTxt += formatNumber(vesselPointingMsg.pitch, 3, true, false);
    botTxt += DEGREE_CHAR_LCD;

    Output.setHeadingLCD(topTxt, botTxt);
}
void setDirectionLCD()
{
    String topTxt = "";
    String botTxt = "";
    float heading = 0;
    float pitch = 0;
    
    // Get heading and pitch based on current direction mode (1-12)
    switch (directionMode)
    {
        case 1:  // Maneuver Node
            topTxt = "Maneuver Mode";
            heading = maneuverMsg.headingNextManeuver;
            pitch = maneuverMsg.pitchNextManeuver;
            break;
        case 2:  // Prograde (orbital velocity)
            topTxt = "Prograde";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = vesselPointingMsg.orbitalVelocityPitch;
            break;
        case 3:  // Retrograde (opposite of prograde)
            topTxt = "Retrograde";
            heading = vesselPointingMsg.orbitalVelocityHeading + 180.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = -vesselPointingMsg.orbitalVelocityPitch;
            break;
        case 4:  // Normal (perpendicular to orbital plane, +90 pitch from prograde)
            topTxt = "Normal";
            heading = vesselPointingMsg.orbitalVelocityHeading + 90.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = 0;  // Normal is perpendicular to orbital plane
            break;
        case 5:  // Anti-Normal (opposite of normal)
            topTxt = "Anti-Normal";
            heading = vesselPointingMsg.orbitalVelocityHeading - 90.0;
            if (heading < 0.0) heading += 360.0;
            pitch = 0;
            break;
        case 6:  // Radial In (toward planet center)
            topTxt = "Radial In";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = -90;  // Radial in points down
            break;
        case 7:  // Radial Out (away from planet center)
            topTxt = "Radial Out";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = 90;  // Radial out points up
            break;
        case 8:  // Target
            topTxt = "Target";
            heading = targetMsg.heading;
            pitch = targetMsg.pitch;
            break;
        case 9:  // Combined: Anti-Target and Velocity (based on reference mode)
            topTxt = "Anti-Target";
            heading = targetMsg.heading + 180.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = -targetMsg.pitch;
            break;
        case 10:  // Velocity direction based on current reference mode
            // Show surface or orbital velocity based on speed mode
            if (currentSpeedMode == SPEED_SURFACE_MODE)
            {
                topTxt = "Surface Velocity";
                heading = vesselPointingMsg.surfaceVelocityHeading;
                pitch = vesselPointingMsg.surfaceVelocityPitch;
            }
            else  // SPEED_ORBIT_MODE or SPEED_TARGET_MODE - use orbital
            {
                topTxt = "Orbital Velocity";
                heading = vesselPointingMsg.orbitalVelocityHeading;
                pitch = vesselPointingMsg.orbitalVelocityPitch;
            }
            break;
        case 11:  // Autopilot Status
            if (autopilotEnabled)
            {
                // Show autopilot target values with prograde indicator
                topTxt = "AP PG:" + String((int)autopilotHeading);
                topTxt += " " + String((int)autopilotSpeed) + "m/s";
                
                // Show altitude target and current error
                float currentAlt = altitudeMsg.sealevel;
                float altErr = autopilotAltitude - currentAlt;
                botTxt = "ALT:" + String((int)autopilotAltitude);
                botTxt += " E:" + String((int)altErr) + "m";
            }
            else
            {
                topTxt = "Autopilot";
                botTxt = "Disabled";
            }
            Output.setDirectionLCD(topTxt, botTxt);
            return;
        case 12:  // Easter egg mode
            if (easterEggActive)
            {
                // Animate 8==D~~ scrolling across screen
                if (millis() - lastEasterEggUpdate >= EASTER_EGG_ANIMATION_SPEED)
                {
                    lastEasterEggUpdate = millis();
                    easterEggPosition++;
                    if (easterEggPosition > 21)  // Reset after it scrolls off screen (16 + 5 chars)
                    {
                        easterEggPosition = 0;
                    }
                }
                
                // Create scrolling effect
                String rocket = "8==D  8===D 8=D";
                int rocketLen = rocket.length();
                
                // Top line - rocket scrolls across
                topTxt = "";
                for (int i = 0; i < 16; i++)
                {
                    int charPos = i - easterEggPosition;
                    if (charPos >= 0 && charPos < rocketLen)
                    {
                        topTxt += rocket[charPos];
                    }
                    else
                    {
                        topTxt += " ";
                    }
                }
                
                // Bottom line - stars background
                botTxt = "8=============D";
                
                Output.setDirectionLCD(topTxt, botTxt);
            }
            else
            {
                topTxt = "Nothing here???";
                botTxt = "";
                Output.setDirectionLCD(topTxt, botTxt);
            }
            return;
        default:
            topTxt = "Direction";
            botTxt = "Select Mode";
            Output.setDirectionLCD(topTxt, botTxt);
            return;
    }
    
    // Format top line: Mode name centered/left-aligned (like "Surface", "Orbit", etc.)
    // Pad mode name to match the style of Speed/Altitude displays
    String modeName = topTxt;
    topTxt = modeName;  // Keep mode name simple on top line
    
    // Format bottom line: "HDG" + heading + "PTH" + pitch (all on one line)
    botTxt = "HDG ";
    botTxt += formatNumber((int)heading, 3, false, false);
    botTxt += DEGREE_CHAR_LCD;
    botTxt += " PTH";
    botTxt += formatNumber((int)pitch, 3, true, false);
    botTxt += DEGREE_CHAR_LCD;
    
    Output.setDirectionLCD(topTxt, botTxt);
}




/// <summary>Map axis with calibration data</summary>
/// <returns>Returns a mapped value using calibrated min/center/max.</returns>
int16_t mapAxisWithCalibration(int raw, int calMin, int calCenter, int calMax, bool flip)
{
    // Use value shorter distance as outer range.
    auto outterVal = abs(calCenter - calMin) < abs(calMax - calCenter) ? abs(calCenter - calMin) : abs(calMax - calCenter);
    return smoothAndMapAxis(map(raw, calCenter - outterVal, outterVal + calCenter, 0, 1023), flip);

    /*
    // Check center deadzone first
    int deadzoneLow = calCenter - JOYSTICK_DEADZONE_CENTER;
    int deadzoneHigh = calCenter + JOYSTICK_DEADZONE_CENTER;
    
    if (raw >= deadzoneLow && raw <= deadzoneHigh)
    {
        // Within center deadzone - return zero
        return 0;
    }

    // Apply edge deadzone and map the value
    int minEdge = calMin + JOYSTICK_DEADZONE;
    int maxEdge = calMax - JOYSTICK_DEADZONE;
    
    // Map lower range (min to center) to (INT16_MIN to 0)
    if (raw < calCenter)
    {
        if (raw < minEdge) raw = minEdge;
        if (raw > deadzoneLow) raw = deadzoneLow;
        // Raw decreases from center to min, output should go from 0 to INT16_MIN
        if (!flip) return map(raw, deadzoneLow, minEdge, 0, INT16_MIN);
        else return map(raw, deadzoneLow, minEdge, 0, INT16_MAX);
    }
    // Map upper range (center to max) to (0 to INT16_MAX)
    else
    {
        if (raw < deadzoneHigh) raw = deadzoneHigh;
        if (raw > maxEdge) raw = maxEdge;
        // Raw increases from center to max, output should go from 0 to INT16_MAX
        if (!flip) return map(raw, deadzoneHigh, maxEdge, 0, INT16_MAX);
        else return map(raw, deadzoneHigh, maxEdge, 0, INT16_MIN);
    }
    */
}

int16_t smoothAndMapAxis(int raw, bool flip)
{
    // Check center deadzone first
    if (raw > 512 - JOYSTICK_DEADZONE_CENTER && raw < 512 + JOYSTICK_DEADZONE_CENTER)
    {
        // Within center deadzone - return zero
        return 0;
    }

    // Apply edge deadzone and map the value
    // For values outside center deadzone, map from edge of center deadzone to edges
    int min = JOYSTICK_DEADZONE;
    int max = 1023 - JOYSTICK_DEADZONE;
    int centerMin = 512 - JOYSTICK_DEADZONE_CENTER;
    int centerMax = 512 + JOYSTICK_DEADZONE_CENTER;
    
    // Map lower range (min to centerMin) to (INT16_MIN to 0)
    // When pulling back (raw decreasing from 512), output should go negative
    if (raw < 512)
    {
        if (raw < min) raw = min;
        if (raw > centerMin) raw = centerMin;
        // Raw decreases from centerMin to min, output should go from 0 to INT16_MIN
        if (!flip) return map(raw, centerMin, min, 0, INT16_MIN);
        else return map(raw, centerMin, min, 0, INT16_MAX);
    }
    // Map upper range (centerMax to max) to (0 to INT16_MAX)
    // When pushing forward (raw increasing from 512), output should go positive
    else
    {
        if (raw < centerMax) raw = centerMax;
        if (raw > max) raw = max;
        // Raw increases from centerMax to max, output should go from 0 to INT16_MAX
        if (!flip) return map(raw, centerMax, max, 0, INT16_MAX);
        else return map(raw, centerMax, max, 0, INT16_MIN);
    }
}

int16_t mapRotationX(int raw) { return mapAxisWithCalibration(raw, rotXCalMin, rotXCalCenter, rotXCalMax, true); }
int16_t mapRotationY(int raw) { return mapAxisWithCalibration(raw, rotYCalMin, rotYCalCenter, rotYCalMax, true); }
int16_t mapRotationZ(int raw) { return mapAxisWithCalibration(raw, rotZCalMin, rotZCalCenter, rotZCalMax, false); }
int16_t mapTranslationX(int raw) { return mapAxisWithCalibration(raw, transXCalMin, transXCalCenter, transXCalMax, true); }
int16_t mapTranslationY(int raw) { return mapAxisWithCalibration(raw, transYCalMin, transYCalCenter, transYCalMax, true); }
int16_t mapTranslationZ(int raw) { return mapAxisWithCalibration(raw, transZCalMin, transZCalCenter, transZCalMax, true); }

// Helper: shortest signed angle difference (target - current) in degrees (-180..180]
float shortestAngleDiff(float target, float current)
{
    float d = target - current;
    while (d > 180.0f) d -= 360.0f;
    while (d <= -180.0f) d += 360.0f;
    return d;
}

void calcResource(float total, float avail, bool* newLEDs)
{
    // Handle edge cases
    if (total <= 0) {
        // No resource capacity - turn off all LEDs
        for (int i = 0; i < 20; i++) {
            newLEDs[i] = false;
        }
        return;
    }
    
    double percentFull = getPercent(total, avail);
    double ledsToLight = PercentageToValue(20, percentFull);
    
    // Light LEDs from bottom to top based on percentage
    for (int i = 0; i < 20; i++)
    {
        if (i < ledsToLight)
            newLEDs[i] = true;
        else
            newLEDs[i] = false;
    }
}
/// <summary>Convert heading degrees to cardinal direction (N, NE, E, SE, S, SW, W, NW)</summary>
/// <returns>Returns a cardinal direction string</returns>
String getCardinalDirection(int heading)
{
    // Normalize heading to 0-359
    heading = heading % 360;
    if (heading < 0) heading += 360;
    
    // 8 cardinal directions, each covers 45 degrees
    // N: 337.5-22.5, NE: 22.5-67.5, E: 67.5-112.5, SE: 112.5-157.5 // S: 157.5-202.5, SW: 202.5-247.5, W: 247.5-292.5, NW: 292.5-337.5
    if (heading < 22.5 || heading >= 337.5) return "N";
    else if (heading < 67.5) return "NE";
    else if (heading < 112.5) return "E";
    else if (heading < 157.5) return "SE";
    else if (heading < 202.5) return "S";
    else if (heading < 247.5) return "SW";
    else if (heading < 292.5) return "W";
    else return "NW";
}

/// <summary>Format numbers for lcd. Length max is 16 characters.This will fit a number to a character range,
/// the number will be to the right of the excess characters. 
/// The excess will become zeros (to the left of the number)</summary>
/// <returns>Returns a formated number at a specific length.</returns>
String formatNumber(int number, byte lengthReq, bool canBeNegative, bool flipNegative)
{
    // Makes the number a positive
    int num = abs(number);
    // Check if the number is negative 
    bool isNegative = number < 0 ? true : false;
    // If should flip the polarity, Does not flip if the number is a zero (fix for causing zero to go negative)
    if (flipNegative && number != 0) isNegative = !isNegative;
    // Check length (32-bit int max is ~2.1 billion, so we only check up to 1 billion)
    if (num < 10) lengthReq -= 1; // 1 characters
    else if (num < 100) lengthReq -= 2; // 2 characters
    else if (num < 1000) lengthReq -= 3; // 3 characters
    else if (num < 10000) lengthReq -= 4; // 4 characters
    else if (num < 100000) lengthReq -= 5; // 5 characters
    else if (num < 1000000) lengthReq -= 6; // 6 characters
    else if (num < 10000000) lengthReq -= 7; // 7 characters
    else if (num < 100000000) lengthReq -= 8; // 8 characters
    else if (num < 1000000000) lengthReq -= 9; // 9 characters
    else lengthReq -= 10; // 10 characters (max for 32-bit int: 2147483647)

    String str;
    for (size_t i = 0; i < lengthReq; i++)
    {
        if (canBeNegative)
        {
            if (i == 0 && isNegative) str += "-";
            else if (i == 0 && !isNegative) str += "+";
            else str += " ";
        }
        else str += " ";
    }

    return str + (String)num;
}
/// <summary>Calculate a gap. The txt length cannot be more than the ideal length.</summary>
String calculateGap(String includedTxt, int idealLength)
{
    // Calculate gap
    int gap = idealLength - includedTxt.length();
    if (gap < 0) return "";
    String str;
    for (size_t i = 0; i < (unsigned int)gap; i++)
    {
        str += " ";
    }
    return includedTxt + str;
}
/// <summary>Input meters and receive it converted and rounded into kilos.</summary>
/// <param name="meters"></param>
/// <returns>Meters rounded into kilometers.</returns>
int getKilometers(int meters)
{
    // Convert
    int km = meters / 1000;
    // Round and return
    return round(km);
}
/// <summary>
/// Find a vertain value using the percentage of a number.
/// </summary>
/// <param name="total">Number to find value.</param>
/// <param name="percentage">Percentage of the total.</param>
/// <returns>Returns the value, percentage is of total.</returns>
double PercentageToValue(double total, double percentage)
{
    double newNum = ((total / 100) * percentage);
    if (newNum > total)
        newNum = total;
    if (newNum < 0)
        newNum = 0;
    return (int)newNum;
}
/// <summary>Get the percent val is of total.</summary>
/// <returns>Percentage</returns>
double getPercent(double total, double val)
{
    if (total <= 0) return 0;
    return (val / total) * 100;
}
int16_t dualPlayerHelper(int16_t x1, int16_t x2)
{
    int16_t x;
    if (x1  == 0 && x2 == 0) // Both zero
    {
        x = 0;
    } 
    else if (x1 == 0 && x2 != 0) // Use x2
    {
        x = x2;
    }
    else if (x2 == 0 && x1 != 0) // Use x1
    {
        x = x1;
    }
    else if ((x1 > 0 && x2 > 0) || (x1 < 0 && x2 < 0)) // If both input on same side of axis, average them
    {
        x = (x1/2)+(x2/2);
    }
    else // Opposite add full values
    {
        x = x1 + x2;
    }
    return x;
}

void debugJoysticks()
{
    // Comprehensive joystick debugging - outputs every 500ms
    if (!joystickDebugTimer.check())
        return;
    
    printDebug("\n========== JOYSTICK DEBUG ==========");
    
    // Raw analog values
    int rotX_raw = Input.getRotationXAxis();
    int rotY_raw = Input.getRotationYAxis();
    int rotZ_raw = Input.getRotationZAxis();
    int transX_raw = Input.getTranslationXAxis();
    int transY_raw = Input.getTranslationYAxis();
    int transZ_raw = Input.getTranslationZAxis();
    int throttle_raw = Input.getThrottleAxis();
    
    printDebug("--- RAW ANALOG VALUES (0-1023) ---");
    printDebug("Rotation  X(Roll): " + String(rotX_raw) + "  Y(Pitch): " + String(rotY_raw) + "  Z(Yaw): " + String(rotZ_raw));
    printDebug("Translation X(L/R): " + String(transX_raw) + "  Y(F/B): " + String(transY_raw) + "  Z(U/D): " + String(transZ_raw));
    printDebug("Throttle: " + String(throttle_raw));
    
    // Centered values (offset from 512)
    printDebug("\n--- CENTERED VALUES (offset from 512) ---");
    printDebug("Rotation  X: " + String(rotX_raw - 512) + "  Y: " + String(rotY_raw - 512) + "  Z: " + String(rotZ_raw - 512));
    printDebug("Translation X: " + String(transX_raw - 512) + "  Y: " + String(transY_raw - 512) + "  Z: " + String(transZ_raw - 512));
    
    // Processed values (after smoothing and mapping)
    int16_t rotX_proc = mapRotationX(rotX_raw);
    int16_t rotY_proc = mapRotationY(rotY_raw);
    int16_t rotZ_proc = mapRotationZ(rotZ_raw);
    int16_t transX_proc = mapTranslationX(transX_raw);
    int16_t transY_proc = mapTranslationY(transY_raw);
    int16_t transZ_proc = mapTranslationZ(transZ_raw);
    
    printDebug("\n--- PROCESSED VALUES (INT16: -32768 to 32767) ---");
    printDebug("Rotation  X: " + String(rotX_proc) + "  Y: " + String(rotY_proc) + "  Z: " + String(rotZ_proc));
    printDebug("Translation X: " + String(transX_proc) + "  Y: " + String(transY_proc) + "  Z: " + String(transZ_proc));
    
    // Deadzone status
    printDebug("\n--- DEADZONE STATUS (90 units from center) ---");
    bool rotX_dead = (rotX_raw > (512 - JOYSTICK_DEADZONE) && rotX_raw < (512 + JOYSTICK_DEADZONE));
    bool rotY_dead = (rotY_raw > (512 - JOYSTICK_DEADZONE) && rotY_raw < (512 + JOYSTICK_DEADZONE));
    bool rotZ_dead = (rotZ_raw > (512 - JOYSTICK_DEADZONE) && rotZ_raw < (512 + JOYSTICK_DEADZONE));
    bool transX_dead = (transX_raw > (512 - JOYSTICK_DEADZONE) && transX_raw < (512 + JOYSTICK_DEADZONE));
    bool transY_dead = (transY_raw > (512 - JOYSTICK_DEADZONE) && transY_raw < (512 + JOYSTICK_DEADZONE));
    bool transZ_dead = (transZ_raw > (512 - JOYSTICK_DEADZONE) && transZ_raw < (512 + JOYSTICK_DEADZONE));
    
    printDebug("Rotation  X: " + String(rotX_dead ? "IN DEADZONE" : "ACTIVE") + 
               "  Y: " + String(rotY_dead ? "IN DEADZONE" : "ACTIVE") + 
               "  Z: " + String(rotZ_dead ? "IN DEADZONE" : "ACTIVE"));
    printDebug("Translation X: " + String(transX_dead ? "IN DEADZONE" : "ACTIVE") + 
               "  Y: " + String(transY_dead ? "IN DEADZONE" : "ACTIVE") + 
               "  Z: " + String(transZ_dead ? "IN DEADZONE" : "ACTIVE"));
    
    // Current trim values
    printDebug("\n--- TRIM OFFSETS ---");
    printDebug("Rotation Trim  X: " + String(trimRotX) + "  Y: " + String(trimRotY) + "  Z: " + String(trimRotZ));
    printDebug("Translation Trim X: " + String(trimTransX) + "  Y: " + String(trimTransY) + "  Z: " + String(trimTransZ));
    
    // Final output values (processed + trim)
    int16_t rotX_final = rotX_proc + trimRotX;
    int16_t rotY_final = rotY_proc + trimRotY;
    int16_t rotZ_final = rotZ_proc + trimRotZ;
    int16_t transX_final = transX_proc + trimTransX;
    int16_t transY_final = transY_proc + trimTransY;
    int16_t transZ_final = transZ_proc + trimTransZ;
    
    printDebug("\n--- FINAL OUTPUT (Processed + Trim) ---");
    printDebug("Rotation  X: " + String(rotX_final) + "  Y: " + String(rotY_final) + "  Z: " + String(rotZ_final));
    printDebug("Translation X: " + String(transX_final) + "  Y: " + String(transY_final) + "  Z: " + String(transZ_final));
    
    // Precision mode
    bool precisionMode = (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON);
    printDebug("\n--- CONTROL MODIFIERS ---");
    String precMsg = "Precision Mode: " + String(precisionMode ? "ON" : "OFF");
    if (precisionMode) {
        precMsg += " (Modifier: " + String(precisionModifier * 100) + "%)";
    }
    printDebug(precMsg);
    
    // View mode
    printDebug("View Mode (Translation for camera): " + String(viewModeEnabled ? "ENABLED" : "DISABLED"));
    
    // Throttle info
    printDebug("\n--- THROTTLE INFO ---");
    printDebug("Raw: " + String(throttle_raw) + "  (Cal Range: " + String(throttleCalMin) + "-" + String(throttleCalMax) + ")");
    
    bool throttleLocked = (Input.getVirtualPin(VPIN_THROTTLE_LOCK_SWITCH, false) == ON);
    printDebug("Throttle Lock: " + String(throttleLocked ? "ON" : "OFF"));
    
    if (throttleLocked) {
        // Calculate percentage
        float throttlePercent = 0.0;
        if (throttle_raw < (throttleCalMin + THROTTLE_DEADZONE)) {
            throttlePercent = 0.0;
        } else if (throttle_raw > (throttleCalMax - THROTTLE_DEADZONE)) {
            throttlePercent = 100.0;
        } else {
            throttlePercent = map(throttle_raw, 
                                 throttleCalMin + THROTTLE_DEADZONE,
                                 throttleCalMax - THROTTLE_DEADZONE,
                                 0, 100);
        }
        printDebug("Throttle Position: " + String(throttlePercent) + "%");
    }
    
    // Button states (INVERTED: hardware reports ON when released)
    printDebug("\n--- BUTTON STATES ---");
    printDebug("Rotation Hold/Trim: " + String(Input.getVirtualPin(VPIN_ROT_HOLD_BUTTON, false) == OFF ? "PRESSED" : "Released"));
    printDebug("Rotation Reset: " + String(Input.getVirtualPin(VPIN_ROT_RESET_BUTTON, false) == OFF ? "PRESSED" : "Released"));
    printDebug("Translation Hold/Trim: " + String(Input.getVirtualPin(VPIN_TRANS_HOLD_BUTTON, false) == OFF ? "PRESSED" : "Released"));
    printDebug("Translation Reset: " + String(Input.getVirtualPin(VPIN_TRANS_RESET_BUTTON, false) == OFF ? "PRESSED" : "Released"));
    printDebug("Translation Button (View Mode Toggle): " + String(Input.getVirtualPin(VPIN_TRANSLATION_BUTTON, false) == ON ? "PRESSED" : "Released"));
    printDebug("Rotation Button: " + String(Input.getVirtualPin(VPIN_ROTATION_BUTTON, false) == ON ? "PRESSED" : "Released"));
    
    printDebug("\n===================================\n");
}

void printDebug(String msg) 
{
    if (isConnectedToKSP && Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
    {
        mySimpit.printToKSP(msg, PRINT_TO_SCREEN);
    }
    else if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
    {
        Serial.println(msg);
    }
}


