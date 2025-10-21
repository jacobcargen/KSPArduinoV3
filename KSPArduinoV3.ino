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
float PERCISION_MODIFIER = 0.5;
// Define smoothing factor
const float JOYSTICK_SMOOTHING_FACTOR = 0.2;  // Adjust this value for more or less smoothing (For Rot and Trans)

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

speedMode currentSpeedMode;
bool translationHold = false;
bool rotationHold = false;

// For hz
int previousMillis;

bool isDebugMode = true;

Timer timer;
int loopCount = 0;


Timer twoSecondTimer;


/////////////////////////////////////////////////////////////////
////////////////////////// Functions ////////////////////////////
/////////////////////////////////////////////////////////////////

void setup()
{
    loopCount = 0;
    timer.start(1000);
    twoSecondTimer.start(2000);
    
    // Open up the serial port
    Serial.begin(115200);
    // Init I/O
    initIO();

    // DEBUG MODE ENABLED
    while (Input.getVirtualPin(VPIN_DEBUG_SWITCH, false) == ON)
    {
        preKSPConnectionLoop();
    }
	Serial.println("Starting Simpit");

    ///// Initialize Simpit
    initSimpit();

    // Additional things to do at start AFTER initialization

    Output.setLED(VLED_POWER, true);
    
    //waitForInputEnable();
    mySimpit.update();

    // Initialization complete
    mySimpit.printToKSP("Initialization Complete!", PRINT_TO_SCREEN);

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
    ////// Set things //////
    //setSpeedLCD();
        //printHz("Loop Time");



    //ButtonState inPin7State = (ButtonState)Input.getSASWarningButton(false);
    


    uint32_t outputStart = millis();
    // Update output to controller (Refresh controller)
    Output.update();
    uint32_t outputDelay = millis() - outputStart;


    uint32_t simpitStart = millis();
    // Update simpit
    mySimpit.update();
    uint32_t simpitDelay = millis() - simpitStart;


    /*
    if (timer.check())
    {
        //mySimpit.printToKSP("Loop: " + String(loopCount) + " | OUTPUT: " + String(outputDelay) + "ms", PRINT_TO_SCREEN);
        //mySimpit.printToKSP("Loop: " + String(loopCount) + " | SIMPIT: " + String(simpitDelay) + "ms", PRINT_TO_SCREEN);


        Input.debugInputState(7);
        
        if (inPin7State == ON)
        {
            mySimpit.printToKSP("Pin 7 is ON", PRINT_TO_SCREEN);
        }
        else if (inPin7State == OFF)
        {
            mySimpit.printToKSP("Pin 7 is OFF", PRINT_TO_SCREEN);
        }
        else
        {
            mySimpit.printToKSP("Pin 7 is NOT READY", PRINT_TO_SCREEN);
        }
        
    }

    if (loopCount % 100 == 0) { // Check every 100 loops
        Input.debugSASWarningButton();
    }

    //refreshInputs();
    */
} 


void initIO()
{
    // Initialize Output
    Output.init();
    // Initialize Input
    Input.init(Serial);
    Input.setAllVPinsReady();

    // NOT IMPLEMENTED
	
	Serial.println("Testing I/O");
	for (int i = 0; i < 154; i++)
	{
		VPin tempPin = {'A', i};
		if (i < 64)
			tempPin.reg = 'A';
		else if (i < 128)
			tempPin.reg = 'B';
		else if (i < 144)
			tempPin.reg = 'C';
        else
            tempPin.reg = 'D';

		Output.setLED(tempPin, true);
	}

    Output.update();
    Input.update();
	delay(1500);
	
	for (int i = 0; i < 154; i++)
	{
		VPin tempPin = {'A', i};
		if (i < 64)
			tempPin.reg = 'A';
		else if (i < 128)
			tempPin.reg = 'B';
		else if (i < 144)
			tempPin.reg = 'C';
        else
            tempPin.reg = 'D';
	
		Output.setLED(tempPin, false);
	}
	Output.update();

    for (int i = 0; i < 154; i++)
	{
		VPin tempPin = {'A', i};
		if (i < 64)
			tempPin.reg = 'A';
		else if (i < 128)
			tempPin.reg = 'B';
		else if (i < 144)
			tempPin.reg = 'C';
        else
            tempPin.reg = 'D';

		Output.setLED(tempPin, true);
	}

    Output.update();
    Serial.println("I/O Initialized");
}

// Minimal loop rate tracker (prints every 3 seconds)
static unsigned long __hz_lastPrintMs = 0;
static unsigned long __hz_loopCount = 0;


void preKSPConnectionLoop()
{
    uint32_t timeStart = millis();
    __hz_loopCount++;
    Input.update();
    /////////////////////////////////////////////////////

    
	//inputs
	for (int i = 0; i < 102; i++)
	{
		auto state = Input.getVirtualPin(i);
		if (state != NOT_READY)
		{
			Serial.print("\nPin " + String(i));
			Serial.println(" --> " + String(state ? "On" : "Off"));
		}
	}
	// outputs
    if (timer.check())
    {
        for (int i = 0; i < 144; i++)
        {
			VPin tempPin = {'A', i};
			if (i < 64)
				tempPin.reg = 'A';
			else if (i < 128)
				tempPin.reg = 'B';
			else
				tempPin.reg = 'C';

            Output.setLED(tempPin, (i + __hz_loopCount) % 2 == 0);
        }
    }
    
    //Output.setSpeedLCD("Waiting for KSP", "Pins Active");


    /////////////////////////////////////////////////////
    Output.update();
    uint32_t loopDelay = millis() - timeStart;
    if (twoSecondTimer.check())
    {
        Serial.print("\nLoop Rate: " + String(loopDelay));
        Serial.println(" ms\n------------END OF LOOP--------------");
    }
}
void initSimpit()
{
    Output.setSpeedLCD("Waiting for KSP", "");
    Output.update();
    // Wait for a connection to ksp
    while (!mySimpit.init()) delay(100);
    // Show that the controller has connected
    mySimpit.printToKSP("KSP Controller Connected!", PRINT_TO_SCREEN);
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
    mySimpit.printToKSP(String(customTxt) + ": " + String(loopRate) + " Hz", PRINT_TO_SCREEN);
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
    mySimpit.printToKSP("Input deactivated!", PRINT_TO_SCREEN);
    Input.setAllVPinsReady();

    mySimpit.printToKSP("Please reset controller to correct state, then press the Input Enable Button.", PRINT_TO_SCREEN);
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
    mySimpit.printToKSP("Input activated!", PRINT_TO_SCREEN);
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
        mySimpit.printToKSP("GOOD: " + name + " is set correctly.", PRINT_TO_SCREEN);
        return true;
    }
    else
    {
        String pos = currentBool ? disabled : enabled;
        mySimpit.printToKSP("BAD: Please update the " + name + " to the " + pos + " position.", PRINT_TO_SCREEN);
        return false;
    }
}

#pragma region Ksp Simpit

/// <summary>Info from ksp.</summary>
void myCallbackHandler(byte messageType, byte msg[], byte msgSize)
{
    switch (messageType)
    {
    case LF_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            liquidFuelMsg = parseMessage<resourceMessage>(msg);
        break;
    case LF_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            liquidFuelStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case OX_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            oxidizerMsg = parseMessage<resourceMessage>(msg);
        break;
    case OX_STAGE_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            oxidizerStageMsg = parseMessage<resourceMessage>(msg);
        break;
    case SF_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            solidFuelMsg = parseMessage<resourceMessage>(msg);
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
        if (msgSize == sizeof(resourceMessage))
            monopropellantMsg = parseMessage<resourceMessage>(msg);
        break;
    case EVA_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            evaMonopropellantMsg = parseMessage<resourceMessage>(msg);
        break;
        // MONO_STAGE_MESSAGE ???
    case ELECTRIC_MESSAGE:
        if (msgSize == sizeof(resourceMessage))
            electricityMsg = parseMessage<resourceMessage>(msg);
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
        mySimpit.printToKSP("SOI:'" + soi + "'", PRINT_TO_SCREEN);
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
    //mySimpit.registerChannel(ORE_MESSAGE);
    //mySimpit.registerChannel(AB_MESSAGE);
    //mySimpit.registerChannel(AB_STAGE_MESSAGE);
    //mySimpit.registerChannel(CUSTOM_RESOURCE_1_MESSAGE);
    //// Flight Data
    //mySimpit.registerChannel(ALTITUDE_MESSAGE);
    mySimpit.registerChannel(VELOCITY_MESSAGE);
    //mySimpit.registerChannel(AIRSPEED_MESSAGE);
    //mySimpit.registerChannel(APSIDES_MESSAGE);
    //mySimpit.registerChannel(APSIDESTIME_MESSAGE);
    //mySimpit.registerChannel(MANEUVER_MESSAGE);
    //mySimpit.registerChannel(SAS_MODE_INFO_MESSAGE);
    //mySimpit.registerChannel(ORBIT_MESSAGE);
    //mySimpit.registerChannel(ROTATION_DATA_MESSAGE);
    //mySimpit.registerChannel(ACTIONSTATUS_MESSAGE);
    //mySimpit.registerChannel(DELTAV_MESSAGE);
    //mySimpit.registerChannel(DELTAVENV_MESSAGE);
    //mySimpit.registerChannel(BURNTIME_MESSAGE);
    mySimpit.registerChannel(CAGSTATUS_MESSAGE);
    //mySimpit.registerChannel(TEMP_LIMIT_MESSAGE);
    mySimpit.registerChannel(TARGETINFO_MESSAGE);
    //mySimpit.registerChannel(SOI_MESSAGE);
    //mySimpit.registerChannel(SCENE_CHANGE_MESSAGE);
    mySimpit.registerChannel(FLIGHT_STATUS_MESSAGE);
    //mySimpit.registerChannel(ATMO_CONDITIONS_MESSAGE);
    //mySimpit.registerChannel(VESSEL_NAME_MESSAGE);
    
}

#pragma endregion


void updateDirectionMode()
{
    ButtonState state = Input.getVirtualPin(VPIN_DIRECTION_MODE);
    if (state != NOT_READY) {
    }
}

void updateInfoMode()
{
    ButtonState state = Input.getVirtualPin(VPIN_INFO_MODE);
    if (state != NOT_READY) {
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
        Output.setLED(VLED_TEMP_WARNING, true);
    }
    else
    {
        Output.setLED(VLED_TEMP_WARNING, false);
    }
}

void setGeeWarning()
{
    Output.setLED(VLED_GEE_WARNING, false);
}

void setWarpWarning()
{
    if (flightStatusMsg.currentTWIndex > 1)
    {
        Output.setLED(VLED_WARP_WARNING, true);
    }
    else
    {
        Output.setLED(VLED_WARP_WARNING, false);
    }
}

void setCommsWarning()
{
    if (flightStatusMsg.commNetSignalStrenghPercentage < COMMS_WARNING_THRESHOLD)
    {
        Output.setLED(VLED_COMMS_WARNING, true);
    }
    else
    {
        Output.setLED(VLED_COMMS_WARNING, false);
    }
}

void setAltWarning()
{
    if (altitudeMsg.surface < LOW_ALTITUDE_WARNING_THRESHOLD)
    {
        Output.setLED(VLED_ALT_WARNING, true);
    }
    else
    {
        Output.setLED(VLED_ALT_WARNING, false);
    }
}

void setPitchWarning()
{
    Output.setLED(VLED_PITCH_WARNING, false);
}

void setSFLEDs()
{
    bool newLEDs[20];
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(solidFuelStageMsg.total, solidFuelStageMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            Output.setLED({VLED_SOLID_FUEL_BASE.reg, (byte)(VLED_SOLID_FUEL_BASE.pin + i)}, newLEDs[i]);
        }
        break;
    case OFF:
        calcResource(solidFuelMsg.total, solidFuelMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            Output.setLED({VLED_SOLID_FUEL_BASE.reg, (byte)(VLED_SOLID_FUEL_BASE.pin + i)}, newLEDs[i]);
        }
        break;
    }
}

void setLFLEDs()
{
    bool newLEDs[20]; 
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(liquidFuelStageMsg.total, liquidFuelStageMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            VPin vpin = {VLED_LIQUID_FUEL_BASE.reg, (byte)(VLED_LIQUID_FUEL_BASE.pin + i)};
            if (vpin.pin >= 64) {
                vpin.reg = 'B';
                vpin.pin -= 64;
            }
            Output.setLED(vpin, newLEDs[i]);
        }
        break;
    case OFF:
        calcResource(liquidFuelMsg.total, liquidFuelMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            VPin vpin = {VLED_LIQUID_FUEL_BASE.reg, (byte)(VLED_LIQUID_FUEL_BASE.pin + i)};
            if (vpin.pin >= 64) {
                vpin.reg = 'B';
                vpin.pin -= 64;
            }
            Output.setLED(vpin, newLEDs[i]);
        }
        break;
    }
}

void setOXLEDs()
{
    bool newLEDs[20];
    ButtonState val = Input.getVirtualPin(VPIN_STAGE_VIEW_SWITCH, false);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(oxidizerStageMsg.total, oxidizerStageMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            Output.setLED({VLED_OXIDIZER_BASE.reg, (byte)(VLED_OXIDIZER_BASE.pin + i)}, newLEDs[i]);
        }
        break;
    case OFF:
        calcResource(oxidizerMsg.total, oxidizerMsg.available, newLEDs);
        for (int i = 0; i < 20; i++) {
            Output.setLED({VLED_OXIDIZER_BASE.reg, (byte)(VLED_OXIDIZER_BASE.pin + i)}, newLEDs[i]);
        }
        break;
    }
}

void setMPLEDs()
{
    bool newLEDs[20];
    if (flightStatusMsg.isInEVA())
        calcResource(evaMonopropellantMsg.total, evaMonopropellantMsg.available, newLEDs);
    else
        calcResource(monopropellantMsg.total, monopropellantMsg.available, newLEDs);
    
    for (int i = 0; i < 20; i++) {
        Output.setLED({VLED_MONOPROPELLANT_BASE.reg, (byte)(VLED_MONOPROPELLANT_BASE.pin + i)}, newLEDs[i]);
    }
}

void setECLEDs()
{
    bool newLEDs[20];
    calcResource(electricityMsg.total, electricityMsg.available, newLEDs);
    
    for (int i = 0; i < 20; i++) {
        VPin vpin = {VLED_ELECTRICITY_BASE.reg, (byte)(VLED_ELECTRICITY_BASE.pin + i)};
        if (vpin.pin >= 64) {
            vpin.reg = 'C';
            vpin.pin -= 64;
        }
        Output.setLED(vpin, newLEDs[i]);
    }
}

void refreshStage()
{
    ButtonState stageLock = Input.getVirtualPin(VPIN_STAGE_LOCK_SWITCH, false);
    if (stageLock == ON)
    {
        Output.setLED(VLED_STAGE, true);
        if (Input.getVirtualPin(VPIN_STAGE_BUTTON) == ON)
            mySimpit.activateAction(STAGE_ACTION);
    }
    else if (stageLock == OFF)
        Output.setLED(VLED_STAGE, false);
}

void refreshAbort()
{
    ButtonState abortLock = Input.getVirtualPin(VPIN_ABORT_LOCK_SWITCH, false);
    if (abortLock == ON)
    {
        Output.setLED(VLED_ABORT, true);
        if (Input.getVirtualPin(VPIN_ABORT_BUTTON) == ON)
            mySimpit.activateAction(ABORT_ACTION);
    }
    else if (abortLock == OFF)
        Output.setLED(VLED_ABORT, false);
}

void refreshLights()
{
    ButtonState lightSwitch = Input.getVirtualPin(VPIN_LIGHTS_SWITCH);
    switch (lightSwitch)
    {
    case NOT_READY:
        break;
    case ON:
        mySimpit.activateAction(LIGHT_ACTION);
        break;
    case OFF:
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
        mySimpit.activateAction(GEAR_ACTION);
        break;
    case OFF:
        mySimpit.deactivateAction(GEAR_ACTION);
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
        mySimpit.activateAction(BRAKES_ACTION);
        break;
    case OFF:
        mySimpit.deactivateAction(BRAKES_ACTION);
        break;
    }

    if (ag.isBrake)
        Output.setLED(VLED_BRAKE_WARNING, true);
    else
        Output.setLED(VLED_BRAKE_WARNING, false);
}

void refreshDocking()
{
    ButtonState val = Input.getVirtualPin(VPIN_DOCKING_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        break;
    case OFF:
        break;
    }
}

void refreshCAGs()
{
    if (Input.getVirtualPin(VPIN_CAG1) == ON)
        mySimpit.toggleCAG(1);
    if (Input.getVirtualPin(VPIN_CAG2) == ON)
        mySimpit.toggleCAG(2);
    if (Input.getVirtualPin(VPIN_CAG3) == ON)
        mySimpit.toggleCAG(3);
    if (Input.getVirtualPin(VPIN_CAG4) == ON)
        mySimpit.toggleCAG(4);
    if (Input.getVirtualPin(VPIN_CAG5) == ON)
        mySimpit.toggleCAG(5);
    if (Input.getVirtualPin(VPIN_CAG6) == ON)
        mySimpit.toggleCAG(6);
    if (Input.getVirtualPin(VPIN_CAG7) == ON)
        mySimpit.toggleCAG(7);
    if (Input.getVirtualPin(VPIN_CAG8) == ON)
        mySimpit.toggleCAG(8);
    if (Input.getVirtualPin(VPIN_CAG9) == ON)
        mySimpit.toggleCAG(9);
    if (Input.getVirtualPin(VPIN_CAG10) == ON)
        mySimpit.toggleCAG(10);

    Output.setLED(VLED_CAG1, cagStatusMsg.is_action_activated(cagStatusMsg.status[0]));
    Output.setLED(VLED_CAG2, cagStatusMsg.is_action_activated(cagStatusMsg.status[1]));
    Output.setLED(VLED_CAG3, cagStatusMsg.is_action_activated(cagStatusMsg.status[2]));
    Output.setLED(VLED_CAG4, cagStatusMsg.is_action_activated(cagStatusMsg.status[3]));
    Output.setLED(VLED_CAG5, cagStatusMsg.is_action_activated(cagStatusMsg.status[4]));
    Output.setLED(VLED_CAG6, cagStatusMsg.is_action_activated(cagStatusMsg.status[5]));
    Output.setLED(VLED_CAG7, cagStatusMsg.is_action_activated(cagStatusMsg.status[6]));
    Output.setLED(VLED_CAG8, cagStatusMsg.is_action_activated(cagStatusMsg.status[7]));
    Output.setLED(VLED_CAG9, cagStatusMsg.is_action_activated(cagStatusMsg.status[8]));
    Output.setLED(VLED_CAG10, cagStatusMsg.is_action_activated(cagStatusMsg.status[9]));
}

void refreshCamReset()
{
    if (Input.getVirtualPin(VPIN_CAM_RESET_BUTTON) == ON)
        mySimpit.setCameraMode(FLIGHT_CAMERA_AUTO);
}

void refreshCamMode()
{
    if (Input.getVirtualPin(VPIN_CAM_MODE_BUTTON) == ON)
        mySimpit.setCameraMode(CAMERA_NEXT_MODE);
}

void refreshFocus()
{
    if (Input.getVirtualPin(VPIN_FOCUS_BUTTON) == ON) {}
}

void refreshView()
{
    ButtonState val = Input.getVirtualPin(VPIN_VIEW_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        break;
    case OFF:
        break;
    }
}

void refreshNav()
{
    ButtonState val = Input.getVirtualPin(VPIN_NAV_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        break;
    case OFF:
        break;
    }
}

void refreshUI()
{
    ButtonState val = Input.getVirtualPin(VPIN_UI_SWITCH);
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        break;
    case OFF:
        break;
    }
}

void refreshScreenshot()
{
    if (Input.getVirtualPin(VPIN_SCREENSHOT_BUTTON) == ON) {}
}

void refreshWarp()
{
    timewarpMessage twMsg;
    ButtonState lock = Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH);

    if (lock == OFF || Input.getVirtualPin(VPIN_CANCEL_WARP_BUTTON) == ON)
    {
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    
    if (Input.getVirtualPin(VPIN_WARP_LOCK_SWITCH, false) == OFF) 
        return;

    if (Input.getVirtualPin(VPIN_PHYS_WARP_SWITCH, false) == ON)
    {
    }

    if (Input.getVirtualPin(VPIN_INCREASE_WARP_BUTTON) == ON) 
    {
        twMsg.command = TIMEWARP_UP;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    if (Input.getVirtualPin(VPIN_DECREASE_WARP_BUTTON) == ON) 
    {
        twMsg.command = TIMEWARP_DOWN;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
}

void refreshPause()
{
    if (Input.getVirtualPin(VPIN_PAUSE_BUTTON) == ON) 
    {
    }
}

void refreshSAS()
{
    ButtonState sasSwitch = Input.getVirtualPin(VPIN_SAS_SWITCH);
    if (sasSwitch == ON)
        mySimpit.activateAction(SAS_ACTION);
    else if (sasSwitch == OFF)
        mySimpit.deactivateAction(SAS_ACTION);

    Output.setLED(VLED_SAS_WARNING, ag.isSAS);
}

void refreshRCS()
{
    ButtonState rcsSwitch = Input.getVirtualPin(VPIN_RCS_SWITCH);
    if (rcsSwitch == ON)
        mySimpit.activateAction(RCS_ACTION);
    else if (rcsSwitch == OFF)
        mySimpit.deactivateAction(RCS_ACTION);

    Output.setLED(VLED_RCS_WARNING, ag.isRCS);
}

void refreshAllSASModes()
{
    if (Input.getVirtualPin(VPIN_SAS_STABILITY_ASSIST_BUTTON) == ON)
        mySimpit.setSASMode(AP_STABILITYASSIST);
    if (Input.getVirtualPin(VPIN_SAS_MANEUVER_BUTTON) == ON)
        mySimpit.setSASMode(AP_MANEUVER);
    if (Input.getVirtualPin(VPIN_SAS_PROGRADE_BUTTON) == ON)
        mySimpit.setSASMode(AP_PROGRADE);
    if (Input.getVirtualPin(VPIN_SAS_RETROGRADE_BUTTON) == ON)
        mySimpit.setSASMode(AP_RETROGRADE);
    if (Input.getVirtualPin(VPIN_SAS_NORMAL_BUTTON) == ON)
        mySimpit.setSASMode(AP_NORMAL);
    if (Input.getVirtualPin(VPIN_SAS_ANTI_NORMAL_BUTTON) == ON)
        mySimpit.setSASMode(AP_ANTINORMAL);
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_IN_BUTTON) == ON)
        mySimpit.setSASMode(AP_RADIALIN);
    if (Input.getVirtualPin(VPIN_SAS_RADIAL_OUT_BUTTON) == ON)
        mySimpit.setSASMode(AP_RADIALOUT);
    if (Input.getVirtualPin(VPIN_SAS_TARGET_BUTTON) == ON)
        mySimpit.setSASMode(AP_TARGET);
    if (Input.getVirtualPin(VPIN_SAS_ANTI_TARGET_BUTTON) == ON)
        mySimpit.setSASMode(AP_ANTITARGET);

    Output.setLED(VLED_SAS_STABILITY_ASSIST, false);
    Output.setLED(VLED_SAS_MANEUVER, false);
    Output.setLED(VLED_SAS_PROGRADE, false);
    Output.setLED(VLED_SAS_RETROGRADE, false);
    Output.setLED(VLED_SAS_NORMAL, false);
    Output.setLED(VLED_SAS_ANTI_NORMAL, false);
    Output.setLED(VLED_SAS_RADIAL_IN, false);
    Output.setLED(VLED_SAS_RADIAL_OUT, false);
    Output.setLED(VLED_SAS_TARGET, false);
    Output.setLED(VLED_SAS_ANTI_TARGET, false);

    switch (sasInfoMsg.currentSASMode)
    {
    case AP_STABILITYASSIST:
        Output.setLED(VLED_SAS_STABILITY_ASSIST, true);
        break;
    case AP_PROGRADE:
        Output.setLED(VLED_SAS_PROGRADE, true);
        break;
    case AP_RETROGRADE:
        Output.setLED(VLED_SAS_RETROGRADE, true);
        break;
    case AP_NORMAL:
        Output.setLED(VLED_SAS_NORMAL, true);
        break;
    case AP_ANTINORMAL:
        Output.setLED(VLED_SAS_ANTI_NORMAL, true);
        break;
    case AP_RADIALIN:
        Output.setLED(VLED_SAS_RADIAL_IN, true);
        break;
    case AP_RADIALOUT:
        Output.setLED(VLED_SAS_RADIAL_OUT, true);
        break;
    case AP_TARGET:
        Output.setLED(VLED_SAS_TARGET, true);
        break;
    case AP_ANTITARGET:
        Output.setLED(VLED_SAS_ANTI_TARGET, true);
        break;
    case AP_MANEUVER:
        Output.setLED(VLED_SAS_MANEUVER, true);
        break;
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
    int axis = Input.getThrottleAxis();
    int16_t throttle = 0;
    
    if (Input.getVirtualPin(VPIN_THROTTLE_LOCK_SWITCH, false) == OFF)
        throttle = smoothAndMapAxis(axis);

    throttleMessage throttleMsg;
    throttleMsg.throttle = throttle;
    mySimpit.send(THROTTLE_MESSAGE, throttleMsg);
}

void refreshTranslation()
{
    if (Input.getVirtualPin(VPIN_TRANS_RESET_BUTTON) == ON)
        translationHold = false;
    if (translationHold)
        return;
        
    int x = Input.getTranslationXAxis();
    int y = Input.getTranslationYAxis();
    int z = Input.getTranslationZAxis();

    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        x *= PERCISION_MODIFIER;
        y *= PERCISION_MODIFIER;
        z *= PERCISION_MODIFIER;
    }

    int16_t transX = smoothAndMapAxis(x);
    int16_t transY = smoothAndMapAxis(y);
    int16_t transZ = smoothAndMapAxis(z);
    
    translationMessage transMsg;
    transMsg.setXYZ(transX, transZ, transY);
    mySimpit.send(TRANSLATION_MESSAGE, transMsg);
}

void refreshTranslationHold()
{
    translationHold = (Input.getVirtualPin(VPIN_TRANS_HOLD_BUTTON) == ON);
}

void refreshRotation()
{
    int x = Input.getRotationXAxis();
    int y = Input.getRotationYAxis();
    int z = Input.getRotationZAxis();

    if (Input.getVirtualPin(VPIN_PRECISION_SWITCH, false) == ON)
    {
        x *= PERCISION_MODIFIER;
        y *= PERCISION_MODIFIER;
        z *= PERCISION_MODIFIER;
    }

    if (Input.getVirtualPin(VPIN_ENABLE_LOOK_BUTTON, false) == ON)
    {
        return;
    }

    if (Input.getVirtualPin(VPIN_ROT_RESET_BUTTON) == ON)
        rotationHold = false;
    if (rotationHold)
        return;

    int16_t rotX = smoothAndMapAxis(x);
    int16_t rotY = smoothAndMapAxis(y);
    int16_t rotZ = smoothAndMapAxis(z);
    
    rotX *= -1;
    
    rotationMessage rotMsg;
    rotMsg.setPitchRollYaw(rotY, rotX, rotZ);
    mySimpit.send(ROTATION_MESSAGE, rotMsg);
}

void refreshRotationHold()
{
    rotationHold = (Input.getVirtualPin(VPIN_ROT_HOLD_BUTTON) == ON);
}

#pragma region Tools

/// <summary>Give the raw analog some smoothing.</summary>
/// <returns>Returns a smoothed and mapped value.</returns>
int16_t smoothAndMapAxis(int raw)//, bool isSmooth = true)
{
    // Smooth the raw input using exponential moving average (EMA)
    int smoothed = (int)(JOYSTICK_SMOOTHING_FACTOR * raw + (1.0 - JOYSTICK_SMOOTHING_FACTOR) * raw);

    // Map the smoothed data for simpit
    int16_t mappedAndSmoothed = map(smoothed, 0, 1023, INT16_MIN, INT16_MAX);
    // Return the smoothed mapped data
    return mappedAndSmoothed;
}

void calcResource(float total, float avail, bool* newLEDs)
{
    double percentFull = 0.0;
    double amt = 0.0;

    percentFull = getPercent(total, avail);
    amt = 20 - PercentageToValue(20, percentFull);

    for (int i = 20; i > 0; i--)
    {
        if (i > amt)
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
    return (val / total) * 100;
}

#pragma endregion
