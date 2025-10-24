/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Wire.h"
#include "Print.h"
#include "Output.h"
#include "Input.h"
#include <PayloadStructs.h>
#include <KerbalSimpitMessageTypes.h>
#include <KerbalSimpit.h>
#include <LiquidCrystal_I2C.h>

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
struct inputStates
{

};
enum infoModes
{
    EMPTY = 0,
    A = 1,
    B = 2,
    C = 3,
    D = 4,
    E = 5,
    F = 6,
    G = 7,
    H = 8,
    I = 9,
    J = 10,
    K = 11,
    L = 12,
    
};
enum speedMode
{
    SPEED_SURFACE_MODE,
    SPEED_ORBIT_MODE,
    SPEED_TARGET_MODE,
    SPEED_VERTICAL_MODE
};

#pragma region Ksp Simpit

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

#pragma endregion

byte infoMode = EMPTY;
byte directionMode = 0;  // Track which direction mode (1-12)


/////////////////////////////////////////////////////////////////
/////////////////////// Configing stuff /////////////////////////
/////////////////////////////////////////////////////////////////

// Degree character for the lcd
const char DEGREE_CHAR_LCD = 223;

// Warning thresholds

byte COMMS_WARNING_THRESHOLD = 50; // 50%
int LOW_ALTITUDE_WARNING_THRESHOLD = 10000; // Warning light turns solid if below this altitude in meters
byte HIGH_GEE_WARNING_SOLID_THRESHOLD = 5; // 5G
byte HIGH_GEE_WARNING_BLINKING_THRESHOLD = 7; // 7G
byte HIGH_TEMP_WARNING_SOLID_THRESHOLD = 25; // 50%
byte HIGH_TEMP_WARNING_BLINKING_THRESHOLD = 50; // 75%

// Joystick configs

// Multiplier for percision mode
float PERCISION_MODIFIER = 0.125;
// Define smoothing factor
const float JOYSTICK_SMOOTHING_FACTOR = 0.2;  // Adjust this value for more or less smoothing (For Rot and Trans)
// Center deadzone (raw value range around center that maps to zero)
const int JOYSTICK_DEADZONE = 75;  // Deadzone range (±50 from center = 512)

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

speedMode currentSpeedMode;
bool translationHold = false;
bool rotationHold = false;

// Stored values for hold functionality
int16_t heldTransX = 0;
int16_t heldTransY = 0;
int16_t heldTransZ = 0;
int16_t heldRotX = 0;
int16_t heldRotY = 0;
int16_t heldRotZ = 0;

// For hz
int previousMillis;

bool isDebugMode = true;
bool isConnectedToKSP = false;

Timer timer;
int loopCount = 0;

Timer lcdTimer;  // Timer for LCD updates
Timer twoSecondTimer;

// Debug flags for resource messages
bool lfReceived = false;
bool oxReceived = false;
bool sfReceived = false;
bool mpReceived = false;
bool ecReceived = false;

/////////////////////////////////////////////////////////////////
////////////////////////// Functions ////////////////////////////
/////////////////////////////////////////////////////////////////

void setup()
{
    loopCount = 0;
    timer.start(1000);
    lcdTimer.start(250);  // LCD updates every 250ms
    twoSecondTimer.start(2000);
    
    // Open up the serial port
    Serial.begin(115200);
    // Init I/O
    initIO();
	
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

    // Additional things to do at start AFTER initialization

    
    //waitForInputEnable();
    mySimpit.update();

    // Initialization complete
    printDebug("Initialization Complete!");

} 

void loop() 
{
    loopCount++;
    // If vessel  change, 
    //if (vesselSwitched)
        //waitForInputEnable();

    // Update input from controller (Refresh inputs)
    //int inputStart = millis();
    Input.update();
    //int inputDelay = millis() - inputStart;


    uint32_t simpitStart = millis();
    // Update simpit (receive messages from KSP including CAG status)
    mySimpit.update();
    uint32_t simpitDelay = millis() - simpitStart;

    // Refresh inputs (this sets LED states based on KSP data)
    refresh();

    uint32_t outputStart = millis();
    // Update output to controller (send LED states to hardware)
    Output.update();
    uint32_t outputDelay = millis() - outputStart;
    
} 


void initIO()
{
    // Initialize Output
    Output.init();
    // Initialize Input
    Input.init(Serial);
    Input.setAllVPinsReady();
	
	// Test I/O
	printDebug("Testing I/O");
	// Output
	setAllOutputs(false);
    Output.update();
	delay(1500);
	setAllOutputs(true);
    Output.update();
	delay(1500);
	setAllOutputs(false);
    Output.update();

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

// Minimal loop rate tracker (prints every 3 seconds)
static unsigned long __hz_lastPrintMs = 0;
static unsigned long __hz_loopCount = 0;


void debugAllOutputs()
{
    printDebug("\n=== OUTPUT DEBUG MODE ===");
    printDebug("This will test each LED one at a time.");
    printDebug("Press STAGE BUTTON to advance to next LED.");
    printDebug("Starting in 3 seconds...\n");
    delay(3000);
    
    // Test each LED from 0 to TOTAL_LEDS
    for (int ledNum = 0; ledNum <= TOTAL_LEDS; ledNum++)
    {
        // Turn off all LEDs
        setAllOutputs(false);
        
        // Turn on current LED
        Output.setLED(ledNum, true);
        Output.update();
        
        // Print info
        printDebug("LED " + String(ledNum) + " is ON");
        printDebug("Press STAGE BUTTON to continue...");
        
        // Wait for stage button press
        bool buttonPressed = false;
        while (!buttonPressed)
        {
            Input.update();
            if (Input.getVirtualPin(VPIN_STAGE_BUTTON, false) == ON)
            {
                buttonPressed = true;
                // Wait for button release
                delay(200);
            }
        }
    }
    
    // All done
    setAllOutputs(false);
    Output.update();
    printDebug("\n=== OUTPUT DEBUG COMPLETE ===");
    printDebug("All LEDs tested. Returning to normal mode.\n");
    delay(2000);
}

void preKSPConnectionLoop()
{
    uint32_t timeStart = millis();
    __hz_loopCount++;
    Input.update();
    /////////////////////////////////////////////////////
	// outputs
    while (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {

        //Test analog
        refreshRotation();
        refreshTranslation();
        refreshThrottle();
    }
    
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
    // Test all inputs - print when any button is pressed
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
        printDebug(" ms\n------------END OF LOOP--------------");
    }
}
void initSimpit()
{
    //Output.setSpeedLCD("Waiting for KSP", "");
    Output.update();
    // Wait for a connection to ksp
    while (!mySimpit.init()) delay(100);
    
    // Set connection flag
    isConnectedToKSP = true;
    
    // Show that the controller has connected
    printDebug("KSP Controller Connected!");
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
void waitForInputEnable()
{

    //Things to check
    //All input state that are toggled
    /*


    SAS
    RCS
    Phys warp
    Warp lock
    Nav
    View
    Throttle Lock
    
    Optional:
    Throttle Position


    */


    
    




    

    // Deavtivate controller
    printDebug("Input deactivated!");
    Input.setAllVPinsReady();

    printDebug("Please reset controller to correct state, then press the Input Enable Button.");
    // Wait for user to reset controller to default and then press Input enable button
    while (true)
    {
        if (checkInput(Input.getVirtualPin(VPIN_GEAR_SWITCH, false), ag.isGear, "OFF", "ON", "Gear Switch") &&
        		checkInput(Input.getVirtualPin(VPIN_LIGHTS_SWITCH, false), ag.isLights, "OFF", "ON", "Lights Switch") &&
            	checkInput(Input.getVirtualPin(VPIN_BRAKE_SWITCH, false), ag.isBrake, "OFF", "ON", "Brake"))
            break;
        else
        {
            waitForInputEnable();
            return;
        }
    }
    printDebug("Input activated!");
}
bool checkInput(ButtonState current, bool correct, String disabled, String enabled, String name)
{
    if (current == NOT_READY)
    {
        return false;
    }
    
    bool currentBool = (current == ON);
    
    if ((currentBool && correct) || (!currentBool && !correct))
    {
        printDebug("GOOD: " + name + " is set correctly.");
        return true;
    }
    else
    {
        String pos = currentBool ? disabled : enabled;
        printDebug("BAD: Please update the " + name + " to the " + pos + " position.");
        return false;
    }
}
void refresh()
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
    

    updateReferenceMode();

    // Update resource LEDs
    setSFLEDs();
    setLFLEDs();
    setOXLEDs();
    setMPLEDs();
    setECLEDs();

    // Update warning LEDs
    setTempWarning();
    setGeeWarning();
    setWarpWarning();
    setCommsWarning();
    setAltWarning();
    setPitchWarning();

    refreshCamReset();
    refreshCamMode();
    refreshFocus();
    refreshView();
    refreshNav();
    refreshUI();
    refreshScreenshot();

    refreshWarp();
    refreshPause();

    refreshThrottle();
    refreshTranslation();
    refreshRotation();

    updateDirectionMode();
    updateInfoMode();

    // Update LCDs on timer (every 250ms) to avoid freezing
    if (lcdTimer.check())
    {
        setSpeedLCD();
        setAltitudeLCD();
        setHeadingLCD();
        setInfoLCD();
        setDirectionLCD();
    }
}


#pragma region Ksp Simpit

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
    case SOI_MESSAGE: // WIP
        soi = (char*)msg;
        //soi[msgSize] = '\0'; 
        soi[soi.length()] = '\0';
        printDebug("SOI:'" + soi + "'");
        break;
    case SCENE_CHANGE_MESSAGE:

        break;
    case FLIGHT_STATUS_MESSAGE:
        if (msgSize == sizeof(flightStatusMessage))
        {
            flightStatusMsg = parseFlightStatusMessage(msg);
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

#pragma endregion


void updateDirectionMode()
{
    // Array of direction mode VPINs (not sequential)
    const byte directionPins[12] = {
        VPIN_DIRECTION_MODE_1, VPIN_DIRECTION_MODE_2, VPIN_DIRECTION_MODE_3, VPIN_DIRECTION_MODE_4,
        VPIN_DIRECTION_MODE_5, VPIN_DIRECTION_MODE_6, VPIN_DIRECTION_MODE_7, VPIN_DIRECTION_MODE_8,
        VPIN_DIRECTION_MODE_9, VPIN_DIRECTION_MODE_10, VPIN_DIRECTION_MODE_11, VPIN_DIRECTION_MODE_12
    };
    
    // Check which of the 12 direction mode positions is active
    for (int i = 0; i < 12; i++) 
    {
        ButtonState state = Input.getVirtualPin(directionPins[i], false);
        if (state == ON) 
        {
            directionMode = i + 1;  // Mode is 1-12, array index is 0-11
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
        mySimpit.cycleNavBallMode();
}

void refeshPitchWarningCancel()
{
    if (Input.getVirtualPin(VPIN_PITCH_WARNING_BUTTON) == ON) {}
}

void refreshAltWarningCancel()
{
    if (Input.getVirtualPin(VPIN_ALT_WARNING_BUTTON) == ON) {}
}

void refreshCommsWarningCancel()
{
    if (Input.getVirtualPin(VPIN_COMMS_WARNING_BUTTON) == ON) {}
}

void refreshGearWarningCancel()
{
    if (Input.getVirtualPin(VPIN_GEAR_WARNING_BUTTON) == ON) {}
}

void refreshRCSWarningCancel()
{
    if (Input.getVirtualPin(VPIN_RCS_WARNING_BUTTON) == ON) {}
}

void refreshSASWarningCancel()
{
    if (Input.getVirtualPin(VPIN_SAS_WARNING_BUTTON) == ON) {}
}

void refreshBrakeWarningCancel()
{
    if (Input.getVirtualPin(VPIN_BRAKE_WARNING_BUTTON) == ON) {}
}

void refreshWarpWarningCancel()
{
    if (Input.getVirtualPin(VPIN_WARP_WARNING_BUTTON) == ON) {}
}

void refreshGeeWarningCancel()
{
    if (Input.getVirtualPin(VPIN_GEE_WARNING_BUTTON) == ON) {}
}

void refreshTempWarningCancel()
{
    if (Input.getVirtualPin(VPIN_TEMP_WARNING_BUTTON) == ON) {}
}

void setTempWarning()
{
    if (tempLimitMsg.tempLimitPercentage > HIGH_TEMP_WARNING_SOLID_THRESHOLD)
    {
        Output.setLED(TEMP_WARNING_LED, true);
    }
    else
    {
        Output.setLED(TEMP_WARNING_LED, false);
    }
}

void setGeeWarning()
{
    // Get G-force from airspeed message (in Gs)
    float gforce = airspeedMsg.gForces;
    
    if (gforce >= HIGH_GEE_WARNING_BLINKING_THRESHOLD)
    {
        // Blinking for extreme G-force
        Output.setLED(GEE_WARNING_LED, (millis() / 250) % 2);  // Blink every 250ms
    }
    else if (gforce >= HIGH_GEE_WARNING_SOLID_THRESHOLD)
    {
        // Solid ON for high G-force
        Output.setLED(GEE_WARNING_LED, true);
    }
    else
    {
        Output.setLED(GEE_WARNING_LED, false);
    }
}

void setWarpWarning()
{
    if (flightStatusMsg.currentTWIndex > 1)
    {
        Output.setLED(WARP_WARNING_LED, true);
    }
    else
    {
        Output.setLED(WARP_WARNING_LED, false);
    }
}

void setCommsWarning()
{
    if (flightStatusMsg.commNetSignalStrenghPercentage < COMMS_WARNING_THRESHOLD)
    {
        Output.setLED(COMMS_WARNING_LED, true);
    }
    else
    {
        Output.setLED(COMMS_WARNING_LED, false);
    }
}

void setAltWarning()
{
    if (altitudeMsg.surface < LOW_ALTITUDE_WARNING_THRESHOLD)
    {
        Output.setLED(ALT_WARNING_LED, true);
    }
    else
    {
        Output.setLED(ALT_WARNING_LED, false);
    }
}

void setPitchWarning()
{
    // Get pitch angle in degrees
    float pitch = vesselPointingMsg.pitch;

    if (pitch < -25)
    {
        // Blinking for steep pitch
        Output.setLED(PITCH_WARNING_LED, (millis() / 250) % 2);
    }
    else
    {
        Output.setLED(PITCH_WARNING_LED, false);
    }
}

void setSFLEDs()
{
    static int debugCounter = 0;
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    bool newLEDs[20];
    const int sfPins[20] = {SOLID_FUEL_LED_1, SOLID_FUEL_LED_2, SOLID_FUEL_LED_3, SOLID_FUEL_LED_4, 
                            SOLID_FUEL_LED_5, SOLID_FUEL_LED_6, SOLID_FUEL_LED_7, SOLID_FUEL_LED_8,
                            SOLID_FUEL_LED_9, SOLID_FUEL_LED_10, SOLID_FUEL_LED_11, SOLID_FUEL_LED_12,
                            SOLID_FUEL_LED_13, SOLID_FUEL_LED_14, SOLID_FUEL_LED_15, SOLID_FUEL_LED_16,
                            SOLID_FUEL_LED_17, SOLID_FUEL_LED_18, SOLID_FUEL_LED_19, SOLID_FUEL_LED_20};
    
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    
    // Default to total if switch not ready
    if (val == ON) {
        calcResource(solidFuelStageMsg.total, solidFuelStageMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("SF Stage: " + String(solidFuelStageMsg.available) + "/" + String(solidFuelStageMsg.total));
    } else {
        calcResource(solidFuelMsg.total, solidFuelMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("SF Total: " + String(solidFuelMsg.available) + "/" + String(solidFuelMsg.total));
    }
    
    for (int i = 0; i < 20; i++) {
        Output.setLED(sfPins[i], newLEDs[i]);
    }
}

void setLFLEDs()
{
    static int debugCounter = 0;
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    bool newLEDs[20];
    const int lfPins[20] = {LIQUID_FUEL_LED_1, LIQUID_FUEL_LED_2, LIQUID_FUEL_LED_3, LIQUID_FUEL_LED_4,
                            LIQUID_FUEL_LED_5, LIQUID_FUEL_LED_6, LIQUID_FUEL_LED_7, LIQUID_FUEL_LED_8,
                            LIQUID_FUEL_LED_9, LIQUID_FUEL_LED_10, LIQUID_FUEL_LED_11, LIQUID_FUEL_LED_12,
                            LIQUID_FUEL_LED_13, LIQUID_FUEL_LED_14, LIQUID_FUEL_LED_15, LIQUID_FUEL_LED_16,
                            LIQUID_FUEL_LED_17, LIQUID_FUEL_LED_18, LIQUID_FUEL_LED_19, LIQUID_FUEL_LED_20};
    
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    
    // Default to total if switch not ready
    if (val == ON) {
        calcResource(liquidFuelStageMsg.total, liquidFuelStageMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("LF Stage: " + String(liquidFuelStageMsg.available) + "/" + String(liquidFuelStageMsg.total));
    } else {
        calcResource(liquidFuelMsg.total, liquidFuelMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("LF Total: " + String(liquidFuelMsg.available) + "/" + String(liquidFuelMsg.total));
    }
    
    for (int i = 0; i < 20; i++) {
        Output.setLED(lfPins[i], newLEDs[i]);
    }
}

void setOXLEDs()
{
    static int debugCounter = 0;
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    bool newLEDs[20];
    const int oxPins[20] = {OXIDIZER_LED_1, OXIDIZER_LED_2, OXIDIZER_LED_3, OXIDIZER_LED_4,
                            OXIDIZER_LED_5, OXIDIZER_LED_6, OXIDIZER_LED_7, OXIDIZER_LED_8,
                            OXIDIZER_LED_9, OXIDIZER_LED_10, OXIDIZER_LED_11, OXIDIZER_LED_12,
                            OXIDIZER_LED_13, OXIDIZER_LED_14, OXIDIZER_LED_15, OXIDIZER_LED_16,
                            OXIDIZER_LED_17, OXIDIZER_LED_18, OXIDIZER_LED_19, OXIDIZER_LED_20};
    
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    
    // Default to total if switch not ready
    if (val == ON) {
        calcResource(oxidizerStageMsg.total, oxidizerStageMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("OX Stage: " + String(oxidizerStageMsg.available) + "/" + String(oxidizerStageMsg.total));
    } else {
        calcResource(oxidizerMsg.total, oxidizerMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("OX Total: " + String(oxidizerMsg.available) + "/" + String(oxidizerMsg.total));
    }
    
    for (int i = 0; i < 20; i++) {
        Output.setLED(oxPins[i], newLEDs[i]);
    }
}

void setMPLEDs()
{
    static int debugCounter = 0;
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    bool newLEDs[20];
    const int mpPins[20] = {MONOPROPELLANT_LED_1, MONOPROPELLANT_LED_2, MONOPROPELLANT_LED_3, MONOPROPELLANT_LED_4,
                            MONOPROPELLANT_LED_5, MONOPROPELLANT_LED_6, MONOPROPELLANT_LED_7, MONOPROPELLANT_LED_8,
                            MONOPROPELLANT_LED_9, MONOPROPELLANT_LED_10, MONOPROPELLANT_LED_11, MONOPROPELLANT_LED_12,
                            MONOPROPELLANT_LED_13, MONOPROPELLANT_LED_14, MONOPROPELLANT_LED_15, MONOPROPELLANT_LED_16,
                            MONOPROPELLANT_LED_17, MONOPROPELLANT_LED_18, MONOPROPELLANT_LED_19, MONOPROPELLANT_LED_20};
    
    if (flightStatusMsg.isInEVA()) {
        calcResource(evaMonopropellantMsg.total, evaMonopropellantMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("MP EVA: " + String(evaMonopropellantMsg.available) + "/" + String(evaMonopropellantMsg.total));
    }
    else {
        calcResource(monopropellantMsg.total, monopropellantMsg.available, newLEDs);
        if (debugOn && debugCounter++ % 100 == 0) printDebug("MP: " + String(monopropellantMsg.available) + "/" + String(monopropellantMsg.total));
    }
    
    for (int i = 0; i < 20; i++) {
        Output.setLED(mpPins[i], newLEDs[i]);
    }
}

void setECLEDs()
{
    static int debugCounter = 0;
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    bool newLEDs[20];
    const int ecPins[20] = {ELECTRICITY_LED_1, ELECTRICITY_LED_2, ELECTRICITY_LED_3, ELECTRICITY_LED_4,
                            ELECTRICITY_LED_5, ELECTRICITY_LED_6, ELECTRICITY_LED_7, ELECTRICITY_LED_8,
                            ELECTRICITY_LED_9, ELECTRICITY_LED_10, ELECTRICITY_LED_11, ELECTRICITY_LED_12,
                            ELECTRICITY_LED_13, ELECTRICITY_LED_14, ELECTRICITY_LED_15, ELECTRICITY_LED_16,
                            ELECTRICITY_LED_17, ELECTRICITY_LED_18, ELECTRICITY_LED_19, ELECTRICITY_LED_20};
    
    calcResource(electricityMsg.total, electricityMsg.available, newLEDs);
    if (debugOn && debugCounter++ % 100 == 0) printDebug("EC: " + String(electricityMsg.available) + "/" + String(electricityMsg.total));
    
    for (int i = 0; i < 20; i++) {
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
            if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
                printDebug("Stage button pressed");
            mySimpit.activateAction(STAGE_ACTION);
        }
    }
    else if (stageLock == OFF)
        Output.setLED(STAGE_LED, false);
}

void refreshAbort()
{
    ButtonState abortLock = Input.getVirtualPin(VPIN_ABORT_LOCK_SWITCH, false);
    if (abortLock == ON)
    {
        Output.setLED(ABORT_LED, true);
        if (Input.getVirtualPin(VPIN_ABORT_BUTTON) == ON)
        {
            if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
                printDebug("Abort button pressed");
            mySimpit.activateAction(ABORT_ACTION);
        }
    }
    else if (abortLock == OFF)
        Output.setLED(ABORT_LED, false);
}

void refreshLights()
{
    ButtonState lightSwitch = Input.getVirtualPin(VPIN_LIGHTS_SWITCH);
    switch (lightSwitch)
    {
    case NOT_READY:
        break;
    case ON:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Lights ON");
        mySimpit.activateAction(LIGHT_ACTION);
        break;
    case OFF:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Lights OFF");
        mySimpit.deactivateAction(LIGHT_ACTION);
        break;
    }
}

void refreshGear()
{
    ButtonState val = Input.getVirtualPin(VPIN_GEAR_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Gear DOWN");
        mySimpit.activateAction(GEAR_ACTION);
        break;
    case OFF:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Gear UP");
        mySimpit.deactivateAction(GEAR_ACTION);
        break;
    }

    Output.setLED(GEAR_WARNING_LED, ag.isGear);
}

void refreshBrake()
{
    ButtonState val = Input.getVirtualPin(VPIN_BRAKE_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Brakes ON");
        mySimpit.activateAction(BRAKES_ACTION);
        break;
    case OFF:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Brakes OFF");
        mySimpit.deactivateAction(BRAKES_ACTION);
        break;
    }

    Output.setLED(BRAKE_WARNING_LED, ag.isBrake);
}

void refreshDocking()
{
    ButtonState val = Input.getVirtualPin(VPIN_DOCKING_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Docking mode ON");
        break;
    case OFF:
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Docking mode OFF");
        break;
    }
}

void refreshCAGs()
{
    if (Input.getVirtualPin(VPIN_CAG1) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG1 toggled");
        mySimpit.toggleCAG(1);
    }
    if (Input.getVirtualPin(VPIN_CAG2) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG2 toggled");
        mySimpit.toggleCAG(2);
    }
    if (Input.getVirtualPin(VPIN_CAG3) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG3 toggled");
        mySimpit.toggleCAG(3);
    }
    if (Input.getVirtualPin(VPIN_CAG4) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG4 toggled");
        mySimpit.toggleCAG(4);
    }
    if (Input.getVirtualPin(VPIN_CAG5) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG5 toggled");
        mySimpit.toggleCAG(5);
    }
    if (Input.getVirtualPin(VPIN_CAG6) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG6 toggled");
        mySimpit.toggleCAG(6);
    }
    if (Input.getVirtualPin(VPIN_CAG7) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG7 toggled");
        mySimpit.toggleCAG(7);
    }    if (Input.getVirtualPin(VPIN_CAG8) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG8 toggled");
        mySimpit.toggleCAG(8);
    }
    if (Input.getVirtualPin(VPIN_CAG9) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("CAG9 toggled");
        mySimpit.toggleCAG(9);
    }
    if (Input.getVirtualPin(VPIN_CAG10) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
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

void refreshCamReset()
{
    if (Input.getVirtualPin(VPIN_CAM_RESET_BUTTON) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Camera reset");
        mySimpit.setCameraMode(FLIGHT_CAMERA_AUTO);
    }
}

void refreshCamMode()
{
    if (Input.getVirtualPin(VPIN_CAM_MODE_BUTTON) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Camera mode changed");
        mySimpit.setCameraMode(CAMERA_NEXT_MODE);
    }
}

void refreshFocus()
{
    if (Input.getVirtualPin(VPIN_FOCUS_BUTTON) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Focus changed");
        mySimpit.setCameraMode(CAMERA_NEXT);  // Cycle to next focus target
    }
}

void refreshView()
{
    ButtonState val = Input.getVirtualPin(VPIN_VIEW_SWITCH);
    if (val == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Internal View");
        
    }
    else if (val == OFF)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("External View");
        
    }
}

// Changes from flight view to map view and vice versa, needs to check current state because this is a switch
void refreshNav()
{
    // NOTE: Map toggle requires keyboard emulation which doesn't work on Linux with Arduino Due
    // This function is disabled. For Linux, use Leonardo/Micro board with Keyboard.h library
    // Or test on Windows where KEYBOARD_EMULATOR works
    /*
    const bool DONT_CHANGE = true;
    ButtonState val = Input.getVirtualPin(VPIN_NAV_SWITCH, DONT_CHANGE);
    if (val == ON || val == OFF)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Map Toggled");
        byte key = 0x4D;  // 'M' key - Windows only!
        mySimpit.send(KEYBOARD_EMULATOR, key);
    }
    */
}

void refreshUI()
{
    // NOTE: UI toggle requires keyboard emulation which doesn't work on Linux with Arduino Due
    // This function is disabled. For Linux, use Leonardo/Micro board with Keyboard.h library
    // Or test on Windows where KEYBOARD_EMULATOR works
    /*
    const bool DONT_CHANGE = true;
    ButtonState val = Input.getVirtualPin(VPIN_UI_SWITCH, DONT_CHANGE);
    if (val == ON || val == OFF)
    {
        printDebug("UI Toggled");
        byte key = 0x74;  // 'F2' key - Windows only!
        mySimpit.send(KEYBOARD_EMULATOR, key);
    }
    */
}

void refreshScreenshot()
{
    // Screenshot functionality requires keyboard emulation (F1 key press)
    // which doesn't work on Linux with Arduino Due.
    // KerbalSimpit library has no native screenshot API.
    // User must press F1 manually to take screenshots.
    
    /* Original code - disabled for Linux/Due compatibility
    if (Input.getVirtualPin(VPIN_SCREENSHOT_BUTTON) == ON) 
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Screenshot taken");
        byte key = 0x70;  // F1 key - requires keyboard emulation (Windows only or Leonardo/Micro board)
        mySimpit.send(KEYBOARD_EMULATOR, key);
    }
    */
}

void refreshWarp()
{
    timewarpMessage twMsg;
    ButtonState lock = Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH);
    ButtonState physWarp = Input.getVirtualPin(VPIN_PHYS_WARP_SWITCH);

    // Handle physics warp switch (only if warp lock is ON)
    if (physWarp == ON && lock == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Physics warp 2x enabled");
        twMsg.command = TIMEWARP_X2_PHYSICAL;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    else if (physWarp == OFF && lock == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Physics warp disabled - canceling all warp");
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }

    // Cancel warp button
    if (Input.getVirtualPin(VPIN_CANCEL_WARP_BUTTON) == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Warp cancelled");
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }

    // If warp lock is OFF, cancel warp
    if (lock == OFF)
    {
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    
    if (Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH, false) == OFF) 
        return;

    if (Input.getVirtualPin(VPIN_INCREASE_WARP_BUTTON) == ON) 
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Warp increased");
        twMsg.command = TIMEWARP_UP;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    if (Input.getVirtualPin(VPIN_DECREASE_WARP_BUTTON) == ON) 
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Warp decreased");
        twMsg.command = TIMEWARP_DOWN;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
}

void refreshPause()
{
    if (Input.getVirtualPin(VPIN_PAUSE_BUTTON) == ON) 
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Pause toggled");
    }
}

void refreshSAS()
{
    ButtonState sasSwitch = Input.getVirtualPin(VPIN_SAS_SWITCH);
    if (sasSwitch == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("SAS ON");
        mySimpit.activateAction(SAS_ACTION);
    }
    else if (sasSwitch == OFF)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("SAS OFF");
        mySimpit.deactivateAction(SAS_ACTION);
    }

    Output.setLED(SAS_WARNING_LED, ag.isSAS);
}

void refreshRCS()
{
    ButtonState rcsSwitch = Input.getVirtualPin(VPIN_RCS_SWITCH);
    if (rcsSwitch == ON)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("RCS ON");
        mySimpit.activateAction(RCS_ACTION);
    }
    else if (rcsSwitch == OFF)
    {
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("RCS OFF");
        mySimpit.deactivateAction(RCS_ACTION);
    }

    Output.setLED(RCS_WARNING_LED, ag.isRCS);
}

void refreshAllSASModes()
{
    bool debugOn = Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON;
    
    if (Input.getVirtualPin(VPIN_SAS_STABILITY_ASSIST_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Stability Assist");
        mySimpit.setSASMode(AP_STABILITYASSIST);
    }
    if (Input.getVirtualPin(VPIN_SAS_MANEUVER_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Maneuver");
        mySimpit.setSASMode(AP_MANEUVER);
    }
    if (Input.getVirtualPin(VPIN_SAS_PROGRADE_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Prograde");
        mySimpit.setSASMode(AP_PROGRADE);
    }
    if (Input.getVirtualPin(VPIN_SAS_RETROGRADE_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Retrograde");
        mySimpit.setSASMode(AP_RETROGRADE);
    }
    if (Input.getVirtualPin(VPIN_SAS_NORMAL_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Normal");
        mySimpit.setSASMode(AP_NORMAL);
    }
    if (Input.getVirtualPin(VPIN_SAS_ANTI_NORMAL_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Anti-Normal");
        mySimpit.setSASMode(AP_ANTINORMAL);
    }
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_IN_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Radial In");
        mySimpit.setSASMode(AP_RADIALIN);
    }
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_OUT_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Radial Out");
        mySimpit.setSASMode(AP_RADIALOUT);
    }
    if (Input.getVirtualPin(VPIN_SAS_TARGET_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Target");
        mySimpit.setSASMode(AP_TARGET);
    }
    if (Input.getVirtualPin(VPIN_SAS_ANTI_TARGET_BUTTON) == ON)
    {
        if (debugOn) printDebug("SAS: Anti-Target");
        mySimpit.setSASMode(AP_ANTITARGET);
    }

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

void refreshJump()
{
    if (Input.getVirtualPin(VPIN_JUMP_BUTTON) == ON) {}
}

void refreshGrab()
{
    if (Input.getVirtualPin(VPIN_GRAB_BUTTON) == ON) {}
}

void refreshBoard()
{
    if (Input.getVirtualPin(VPIN_BOARD_BUTTON) == ON) {}
}

void refreshThrottle()
{
    static int16_t lastThrottle = 0;  // Remember last throttle value
    
    // Only read and update throttle axis if throttle lock is ON
    if (Input.getVirtualPin(VPIN_THROTTLE_LOCK_SWITCH, false) == ON)
    {
        int axis = Input.getThrottleAxis() * 1.33;
        lastThrottle = smoothAndMapAxis(axis, false);
        
        throttleMessage throttleMsg;
        throttleMsg.throttle = lastThrottle;
        if (isConnectedToKSP) mySimpit.send(THROTTLE_MESSAGE, throttleMsg);
    }
    // If lock is OFF, don't send any throttle updates (holds current position in KSP)
}

void refreshTranslation()
{
    // Check for hold button press (capture values)
    if (Input.getVirtualPin(VPIN_TRANS_HOLD_BUTTON) == ON && !translationHold)
    {
        // Capture current values
        int x = Input.getTranslationXAxis();
        int y = Input.getTranslationYAxis();
        int z = Input.getTranslationZAxis();
        
        heldTransX = smoothAndMapAxis(x, true);
        heldTransY = smoothAndMapAxis(y, false);
        heldTransZ = smoothAndMapAxis(z, true);
        
        if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
        {
            if (PERCISION_MODIFIER > 0 && PERCISION_MODIFIER < 1)
            {
                heldTransX *= PERCISION_MODIFIER;
                heldTransY *= PERCISION_MODIFIER;
                heldTransZ *= PERCISION_MODIFIER;
            }
        }
        
        translationHold = true;
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Translation HOLD activated");
    }
    
    // Reset button clears hold
    if (Input.getVirtualPin(VPIN_TRANS_RESET_BUTTON) == ON)
    {
        translationHold = false;
        heldTransX = 0;
        heldTransY = 0;
        heldTransZ = 0;
    }
    
    // If in hold mode, send the held values
    if (translationHold)
    {
        translationMessage transMsg;
        transMsg.setXYZ(heldTransX, heldTransZ, heldTransY);
        if (isConnectedToKSP) mySimpit.send(TRANSLATION_MESSAGE, transMsg);
        return;
    }
        
    // Normal operation - read current joystick values
    int x = Input.getTranslationXAxis();
    int y = Input.getTranslationYAxis();
    int z = Input.getTranslationZAxis();

    int16_t transX = smoothAndMapAxis(x, true);
    int16_t transY = smoothAndMapAxis(y, false);
    int16_t transZ = smoothAndMapAxis(z, true);

    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        if (PERCISION_MODIFIER > 0 && PERCISION_MODIFIER < 1)
        {
            transX *= PERCISION_MODIFIER;
            transY *= PERCISION_MODIFIER;
            transZ *= PERCISION_MODIFIER;
        }
    }

    translationMessage transMsg;
    transMsg.setXYZ(transX, transZ, transY);
    if (isConnectedToKSP) mySimpit.send(TRANSLATION_MESSAGE, transMsg);
}

void refreshRotation()
{
    // Check for hold button press (capture values)
    if (Input.getVirtualPin(VPIN_ROT_HOLD_BUTTON) == ON && !rotationHold)
    {
        // Capture current values
        int x = Input.getRotationXAxis();
        int y = Input.getRotationYAxis();
        int z = Input.getRotationZAxis();
        
        heldRotX = smoothAndMapAxis(x, true);
        heldRotY = smoothAndMapAxis(y, true);
        heldRotZ = smoothAndMapAxis(z, false);
        
        if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
        {
            if (PERCISION_MODIFIER > 0 && PERCISION_MODIFIER < 1)
            {
                heldRotX *= PERCISION_MODIFIER;
                heldRotY *= PERCISION_MODIFIER;
                heldRotZ *= PERCISION_MODIFIER;
            }
        }
        
        rotationHold = true;
        if (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
            printDebug("Rotation HOLD activated");
    }
    
    // Reset button clears hold
    if (Input.getVirtualPin(VPIN_ROT_RESET_BUTTON) == ON)
    {
        rotationHold = false;
        heldRotX = 0;
        heldRotY = 0;
        heldRotZ = 0;
    }
    
    // If in hold mode, send the held values
    if (rotationHold)
    {
        rotationMessage rotMsg;
        rotMsg.setPitchRollYaw(heldRotY, heldRotX, heldRotZ);
        if (isConnectedToKSP) mySimpit.send(ROTATION_MESSAGE, rotMsg);
        return;
    }
    
    // Normal operation - read current joystick values
    int x = Input.getRotationXAxis();
    int y = Input.getRotationYAxis();
    int z = Input.getRotationZAxis();
    
    if (Input.getVirtualPin(VPIN_ENABLE_LOOK_BUTTON, false) == ON)
    {
        //return;
    }

    bool isReadyPrint = timer.check();
    if (isReadyPrint) printDebug("Rot X: " + String(x) + " Rot Y: " + String(y) + " Rot Z: " + String(z));
    int16_t rotX = smoothAndMapAxis(x, true);
    int16_t rotY = smoothAndMapAxis(y, true);
    int16_t rotZ = smoothAndMapAxis(z, false);
    if (isReadyPrint) printDebug("Smoothed Rot X: " + String(rotX) + " Smoothed Rot Y: " + String(rotY) + " Smoothed Rot Z: " + String(rotZ));

    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        if (PERCISION_MODIFIER > 0 && PERCISION_MODIFIER < 1)
        {
            rotX *= PERCISION_MODIFIER;
            rotY *= PERCISION_MODIFIER;
            rotZ *= PERCISION_MODIFIER;
        }
    }
    
    rotationMessage rotMsg;
    rotMsg.setPitchRollYaw(rotY, rotX, rotZ);
    if (isConnectedToKSP) mySimpit.send(ROTATION_MESSAGE, rotMsg);
}

// Display
void setSpeedLCD()
{
    // Speed
    int speed;
    // Clear the strings
    String topTxt = "";
    String botTxt = "";
    // Check the current speed mode to use and set the values for that mode
    switch (currentSpeedMode)
    {
    case SPEED_SURFACE_MODE:
        speed = velocityMsg.surface;
        topTxt += "Surface";
        break;
    case SPEED_ORBIT_MODE:
        speed = velocityMsg.orbital;
        topTxt += "Orbit";
        break;
    case SPEED_TARGET_MODE:
        speed = targetMsg.velocity;
        topTxt += "Target";
        break;
    case SPEED_VERTICAL_MODE:
        speed = velocityMsg.vertical;
        topTxt += "Vertical";
        break;
    default:
        break;
    }
    // Speed txt
    botTxt += "SPD ";
    // Speed
    botTxt += formatNumber(speed, 9, false, false);
    // Add unit measurement
    botTxt += "m/s";

    Output.setSpeedLCD(topTxt, botTxt);
}
void setAltitudeLCD()
{
    // Alt
    int altitude;
    // Clear the strings
    String topTxt = "";
    String botTxt = "";
    // Calculate gap for soi name
    // No SOI names are more than 7 char, which is good because that is the exact amount of room at max on the lcd.
    topTxt += calculateGap("", 7);//soi, 7);
    // Check altitude mode (ON = radar/land, OFF = sea level)
    if (Input.getVirtualPin(VPIN_RADAR_ALTITUDE_SWITCH, false) == ON) // LAND/RADAR
    {
        topTxt += "     Land";
        altitude = altitudeMsg.surface;
    }
    else // SEA LEVEL
    {
        topTxt += "      Sea";
        altitude = altitudeMsg.sealevel;
    }
    // Alt txt
    botTxt += "ALT";
    if (altitude >= 1000000)
    {
        altitude = getKilometers(altitude);
        botTxt += formatNumber(altitude, 12, true, false);
        botTxt += "k";
    }
    else
    {
        botTxt += formatNumber(altitude, 12, true, false);
        botTxt += "m";
    }
    Output.setAltitudeLCD(topTxt, botTxt);
}
void setInfoLCD()
{
    String topTxt = "";
    String botTxt = "";
    
    // Display data based on current info mode (1-12)
    switch (infoMode)
    {
        case 1:  // Apoapsis Time
            topTxt = "Time to Ap";
            if (apsidesTimeMsg.apoapsis >= 0)
            {
                int timeToAp = apsidesTimeMsg.apoapsis;
                int hours = timeToAp / 3600;
                int minutes = (timeToAp % 3600) / 60;
                int seconds = timeToAp % 60;
                if (hours > 0)
                    botTxt = String(hours) + "h " + String(minutes) + "m";
                else
                    botTxt = String(minutes) + "m " + String(seconds) + "s";
            }
            else
                botTxt = "N/A";
            break;
            
        case 2:  // Periapsis Time
            topTxt = "Time to Pe";
            if (apsidesTimeMsg.periapsis >= 0)
            {
                int timeToPe = apsidesTimeMsg.periapsis;
                int hours = timeToPe / 3600;
                int minutes = (timeToPe % 3600) / 60;
                int seconds = timeToPe % 60;
                if (hours > 0)
                    botTxt = String(hours) + "h " + String(minutes) + "m";
                else
                    botTxt = String(minutes) + "m " + String(seconds) + "s";
            }
            else
                botTxt = "N/A";
            break;
            
        case 3:  // Time to Maneuver
            topTxt = "Time to Node";
            if (maneuverMsg.timeToNextManeuver >= 0)
            {
                int timeToNode = (int)maneuverMsg.timeToNextManeuver;
                int hours = timeToNode / 3600;
                int minutes = (timeToNode % 3600) / 60;
                int seconds = timeToNode % 60;
                if (hours > 0)
                    botTxt = String(hours) + "h " + String(minutes) + "m";
                else
                    botTxt = String(minutes) + "m " + String(seconds) + "s";
            }
            else
                botTxt = "No Node";
            break;
            
        case 4:  // Maneuver DeltaV
            topTxt = "Node DeltaV";
            if (maneuverMsg.deltaVNextManeuver > 0)
                botTxt = String((int)maneuverMsg.deltaVNextManeuver) + " m/s";
            else
                botTxt = "No Node";
            break;
            
        case 5:  // Orbit Period
            topTxt = "Orbit Period";
            if (orbitInfoMsg.period > 0)
            {
                int period = (int)orbitInfoMsg.period;
                int hours = period / 3600;
                int minutes = (period % 3600) / 60;
                if (hours > 0)
                    botTxt = String(hours) + "h " + String(minutes) + "m";
                else
                    botTxt = String(minutes) + "m " + String(period % 60) + "s";
            }
            else
                botTxt = "No Orbit";
            break;
            
        case 6:  // Eccentricity
            topTxt = "Eccentricity";
            botTxt = String(orbitInfoMsg.eccentricity, 3);
            break;
            
        case 7:  // Inclination
            topTxt = "Inclination";
            botTxt = String((int)orbitInfoMsg.inclination) + DEGREE_CHAR_LCD;
            break;
            
        case 8:  // Total DeltaV
            topTxt = "Total DeltaV";
            if (deltaVMsg.totalDeltaV >= 1000)
                botTxt = String((int)(deltaVMsg.totalDeltaV / 1000)) + "." + String((int)(deltaVMsg.totalDeltaV) % 1000 / 100) + "k m/s";
            else
                botTxt = String((int)deltaVMsg.totalDeltaV) + " m/s";
            break;
            
        case 9:  // Stage DeltaV
            topTxt = "Stage DeltaV";
            if (deltaVMsg.stageDeltaV >= 1000)
                botTxt = String((int)(deltaVMsg.stageDeltaV / 1000)) + "." + String((int)(deltaVMsg.stageDeltaV) % 1000 / 100) + "k m/s";
            else
                botTxt = String((int)deltaVMsg.stageDeltaV) + " m/s";
            break;
            
        case 10:  // Burn Time
            topTxt = "Burn Time";
            if (burnTimeMsg.stageBurnTime > 0)
            {
                int burnTime = (int)burnTimeMsg.stageBurnTime;
                int minutes = burnTime / 60;
                int seconds = burnTime % 60;
                if (minutes > 0)
                    botTxt = String(minutes) + "m " + String(seconds) + "s";
                else
                    botTxt = String(seconds) + "s";
            }
            else
                botTxt = "N/A";
            break;
            
        case 11:  // Target Distance
            topTxt = "Target Dist";
            if (targetMsg.distance > 0)
            {
                if (targetMsg.distance >= 1000000)
                    botTxt = String((int)(targetMsg.distance / 1000)) + " km";
                else if (targetMsg.distance >= 1000)
                    botTxt = String((int)(targetMsg.distance / 1000)) + "." + String((int)(targetMsg.distance) % 1000 / 100) + " km";
                else
                    botTxt = String((int)targetMsg.distance) + " m";
            }
            else
                botTxt = "No Target";
            break;
            
        case 12:  // Target Velocity
            topTxt = "Target Vel";
            if (targetMsg.velocity != 0)
                botTxt = String((int)targetMsg.velocity) + " m/s";
            else
                botTxt = "No Target";
            break;
            
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

    topTxt += "Heading "; // DO like a north,east,wesst,south here instead of "Heading "
    // Heading txt
    topTxt += " HDG+";
    topTxt += formatNumber(vesselPointingMsg.heading, 3, false, false);
    topTxt += DEGREE_CHAR_LCD;
    // Pitch txt
    botTxt += "PTH";
    botTxt += formatNumber(vesselPointingMsg.pitch, 3, true, false);
    botTxt += DEGREE_CHAR_LCD;
    // Roll txt
    botTxt += " RLL";
    botTxt += formatNumber(vesselPointingMsg.roll, 4, true, true);
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
            topTxt = "MVR";
            heading = maneuverMsg.headingNextManeuver;
            pitch = maneuverMsg.pitchNextManeuver;
            break;
        case 2:  // Prograde (orbital velocity)
            topTxt = "PGD";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = vesselPointingMsg.orbitalVelocityPitch;
            break;
        case 3:  // Retrograde (opposite of prograde)
            topTxt = "RGD";
            heading = vesselPointingMsg.orbitalVelocityHeading + 180.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = -vesselPointingMsg.orbitalVelocityPitch;
            break;
        case 4:  // Normal (perpendicular to orbital plane, +90 pitch from prograde)
            topTxt = "NOR";
            heading = vesselPointingMsg.orbitalVelocityHeading + 90.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = 0;  // Normal is perpendicular to orbital plane
            break;
        case 5:  // Anti-Normal (opposite of normal)
            topTxt = "A-NOR";
            heading = vesselPointingMsg.orbitalVelocityHeading - 90.0;
            if (heading < 0.0) heading += 360.0;
            pitch = 0;
            break;
        case 6:  // Radial In (toward planet center)
            topTxt = "RAD-I";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = -90;  // Radial in points down
            break;
        case 7:  // Radial Out (away from planet center)
            topTxt = "RAD-O";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = 90;  // Radial out points up
            break;
        case 8:  // Target
            topTxt = "TAR";
            heading = targetMsg.heading;
            pitch = targetMsg.pitch;
            break;
        case 9:  // Anti-Target (opposite of target)
            topTxt = "A-TAR";
            heading = targetMsg.heading + 180.0;
            if (heading >= 360.0) heading -= 360.0;
            pitch = -targetMsg.pitch;
            break;
        case 10:  // Surface Velocity
            topTxt = "SRF-V";
            heading = vesselPointingMsg.surfaceVelocityHeading;
            pitch = vesselPointingMsg.surfaceVelocityPitch;
            break;
        case 11:  // Orbital Velocity (same as prograde but labeled differently)
            topTxt = "ORB-V";
            heading = vesselPointingMsg.orbitalVelocityHeading;
            pitch = vesselPointingMsg.orbitalVelocityPitch;
            break;
        case 12:  // North Heading (0 degrees, level)
            topTxt = "NORTH";
            heading = 0;
            pitch = 0;
            break;
        default:
            topTxt = "Direction";
            botTxt = "Select Mode";
            Output.setDirectionLCD(topTxt, botTxt);
            return;
    }
    
    // Format top line: Mode name + Heading
    topTxt += " HDG";
    topTxt += formatNumber((int)heading, 3, false, false);
    topTxt += DEGREE_CHAR_LCD;
    
    // Format bottom line: Pitch
    botTxt = "PTH";
    botTxt += formatNumber((int)pitch, 3, true, false);
    botTxt += DEGREE_CHAR_LCD;
    
    Output.setDirectionLCD(topTxt, botTxt);
}

#pragma region Tools

/// <summary>Give the raw analog some smoothing.</summary>
/// <returns>Returns a smoothed and mapped value.</returns>
int16_t smoothAndMapAxis(int raw, bool flip)//, bool isSmooth = true)
{
    // Map the raw data for simpit (0 - 1023 to INT16_MIN to INT16_MAX)
    if (raw < 0) raw = 0;
    else if (raw > 1023) raw = 1023;

    if (raw < 512 + JOYSTICK_DEADZONE && raw > 512 - JOYSTICK_DEADZONE)
    {
        // Within deadzone - return zero (center position)
        return 0;
    }

    if (!flip) return map(raw, 0, 1023, INT16_MIN, INT16_MAX);
    else return map(raw, 0, 1023, INT16_MAX, INT16_MIN);
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
    // Check length
    if (num < 10) lengthReq -= 1; // 1 characters
    else if (num < 100) lengthReq -= 2; // 2 characters
    else if (num < 1000) lengthReq -= 3; // 3 characters
    else if (num < 10000) lengthReq -= 4; // 4 characters
    else if (num < 100000) lengthReq -= 5; // 5 characters
    else if (num < 1000000) lengthReq -= 6; // 6 characters
    else if (num < 10000000) lengthReq -= 7; // 7 characters
    else if (num < 100000000) lengthReq -= 8; // 8 characters
    else if (num < 1000000000) lengthReq -= 9; // 9 characters
    else if (num < 10000000000) lengthReq -= 10; // 10 characters
    else if (num < 100000000000) lengthReq -= 11; // 11 characters
    else if (num < 1000000000000) lengthReq -= 12; // 12 characters
    else if (num < 10000000000000) lengthReq -= 13; // 13 characters
    else if (num < 100000000000000) lengthReq -= 14; // 14 characters
    else if (num < 1000000000000000) lengthReq -= 15; // 15 characters
    else if (num < 10000000000000000) lengthReq -= 16; // 16 characters

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
    for (size_t i = 0; i < gap; i++)
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

void printDebug(String msg) 
{
    if (isConnectedToKSP && Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
    {
        mySimpit.printToKSP(msg, PRINT_TO_SCREEN);   
    }
    else
    {
        Serial.println(msg);
    }
}

#pragma endregion
