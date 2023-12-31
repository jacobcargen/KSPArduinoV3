/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

#include "Print.h"
#include "Output.h"
#include "Input.h"
#include <PayloadStructs.h>
#include <KerbalSimpitMessageTypes.h>
#include <KerbalSimpit.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

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

bool translationHold = false;
bool rotationHold = false;

// For hz
int previousMillis;

bool isDebugMode = true;

void setup()
{
    // Open up the serial port
    Serial.begin(115200);

    initIO();

    //while (true)
        //preKSPConnectionLoop();

    ///// Initialize Simpit
    initSimpit();

    // Additional things to do at start AFTER initialization

    Output.setPowerLED(true);
    
    //waitForInputEnable();
    mySimpit.update();


    // Initialization complete
    mySimpit.printToKSP("Initialization Complete!", PRINT_TO_SCREEN);




    
}

void preKSPConnectionLoop()
{
    Input.update();
    Output.update();

    // INPUT READ EXAMPLE
    byte val = Input.getTestSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        Output.setTestLED(true);
        break;
    case OFF:
        Output.setTestLED(false);
        break;
    default:
        break;
    }
}
void loop() 
{
    
    // If vessel  change, 
    //if (vesselSwitched)
        //waitForInputEnable();

    // Update input from controller (Refresh inputs)
    Input.update();

    ////// Set things //////
    
    //setCAG();
    setStage();

    // Update output to controller (Refresh controller)
    Output.update();
    
    // Update simpit
    mySimpit.update();


    if (isDebugMode)
        printHz();
}

void initIO()
{
    // Initialize Output
    Output.init();
    // Initialize Input
    Input.init();

    // Test Output
    testOutput();
    // Test Input
    // NOT IMPLEMENTED

    Output.update();
    Input.update();
}
void initSimpit()
{
    // Wait for a connection to ksp
    while (!mySimpit.init())
    {
        //preKSPConnectionLoop();
    }
    // Show that the controller has connected
    mySimpit.printToKSP("KSP Controller Connected!", PRINT_TO_SCREEN);
    // Register a method for receiving simpit message from ksp
    mySimpit.inboundHandler(myCallbackHandler);
    // Register the simpit channels
    registerSimpitChannels();
}
void testOutput()
{
    // Test output leds
    bool x[144];
    for (int i = 0; i < 144; i++)
    {
        x[i] = true;
    }
    Output.overrideSet(x);
    Output.update();
    delay(250);
    for (int i = 0; i < 144; i++)
    {
        x[i] = false;
    }
    Output.overrideSet(x);
    Output.update();
    delay(50);
    for (int i = 0; i < 144; i++)
    {
        x[i] = true;
    }
    Output.overrideSet(x);
    Output.update();
    delay(250);
    for (int i = 0; i < 144; i++)
    {
        x[i] = false;
    }
    Output.overrideSet(x);
    Output.update();
    delay(500);
}

void print(String x) 
{ 
    Serial.println("\t" + x);
}

void printHz()
{
    // Measure the current time
    unsigned long currentMillis = millis();

    // Calculate the time elapsed since the previous iteration
    unsigned long elapsedTime = currentMillis - previousMillis;

    // Print the loop rate (inverse of the elapsed time)
    float loopRate = 1000.0 / elapsedTime;  // Convert to loops per second (Hz)
    Serial.print("");
    Serial.print(loopRate);
    Serial.println(" Hz");

    mySimpit.printToKSP("Loop Rate: " + String(loopRate) + " Hz", PRINT_TO_SCREEN);
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

    mySimpit.printToKSP("Please reset controller to correct state., then press the Input Enable Button.", PRINT_TO_SCREEN);
    // Wait for user to reset controller to default and then press Input enable button
    while (true)
    {
        if (checkInput(Input.getGearSwitch(false), ag.isGear, "OFF", "ON", "Gear Switch") &&
            checkInput(Input.getLightsSwitch(false), ag.isLights, "OFF", "ON", "Lights Switch") &&
            checkInput(Input.getBrakeSwitch(false), ag.isBrake, "OFF", "ON", "Brake") //&&
            //checkInput()
            )
            break;
        else
        {
            waitForInputEnable();
            return;
        }
    }
    mySimpit.printToKSP("Input activated!", PRINT_TO_SCREEN);
    // Success!

}
bool checkInput(byte current, bool correct, String disabled, String enabled, String name)
{
    if (current == 255)
    {
        // THIS SHOULD NEVER HAPPEN
    }
    if ((current && correct) || (!current && !correct))
    {
        // Set correctly
        mySimpit.printToKSP("GOOD: " + name + " is set correctly.", PRINT_TO_SCREEN);
        return true;
    }
    else
    {
        // User needs to put gear in the correct posisition
        String pos = current ? disabled : enabled;
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
    //mySimpit.registerChannel(VELOCITY_MESSAGE);
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
    //mySimpit.registerChannel(TARGETINFO_MESSAGE);
    //mySimpit.registerChannel(SOI_MESSAGE);
    //mySimpit.registerChannel(SCENE_CHANGE_MESSAGE);
    mySimpit.registerChannel(FLIGHT_STATUS_MESSAGE);
    //mySimpit.registerChannel(ATMO_CONDITIONS_MESSAGE);
    //mySimpit.registerChannel(VESSEL_NAME_MESSAGE);
    
}

#pragma endregion

void updateStates()
{
    /*
    // Inputs
     // Test Buttons
    isTestButton = getTestButton();
    isTestSwitch = getTestSwitch();

    // Miscellaneous
    isDebugSwitch = getDebugSwitch();
    isSoundSwitch = getSoundSwitch();
    isInputEnableButton = getInputEnableButton();

    // Warnings
    isTempWarningButton = getTempWarningButton();
    isGeeWarningButton = getGeeWarningButton();
    isWarpWarningButton = getWarpWarningButton();
    isBrakeWarningButton = getBrakeWarningButton();
    isSASWarningButton = getSASWarningButton();
    isRCSWarningButton = getRCSWarningButton();
    isGearWarningButton = getGearWarningButton();
    isCommsWarningButton = getCommsWarningButton();
    isAltWarningButton = getAltWarningButton();
    isPitchWarningButton = getPitchWarningButton();

    // Display Controls
    isGetInfoMode = getInfoMode();
    isGetDirectionMode = getDirectionMode();
    isGetStageViewSwitch = getStageViewSwitch();
    isGetVerticalVelocitySwitch = getVerticalVelocitySwitch();
    isGetReferenceModeButton = getReferenceModeButton();
    isGetRadarAltitudeSwitch = getRadarAltitudeSwitch();

    // Staging
    isGetStageButton = getStageButton();
    isGetStageLockSwitch = getStageLockSwitch();

    // Aborting
    isGetAbortButton = getAbortButton();
    isGetAbortLockSwitch = getAbortLockSwitch();

    // Custom Action Groups
    isGetCAG1 = getCAG1();
    isGetCAG2 = getCAG2();
    isGetCAG3 = getCAG3();
    isGetCAG4 = getCAG4();
    isGetCAG5 = getCAG5();
    isGetCAG6 = getCAG6();
    isGetCAG7 = getCAG7();
    isGetCAG8 = getCAG8();
    isGetCAG9 = getCAG9();
    isGetCAG10 = getCAG10();

    // Other Action Groups
    isGetDockingSwitch = getDockingSwitch();
     = getPercisionSwitch();
     = getLightsSwitch();
     = getGearSwitch();
     = getBrakeSwitch();

    // View
     = getScreenshotButton();
     = getUISwitch();
     = getNavSwitch();
     = getViewSwitch();
     = getFocusButton();
     = getCamModeButton();
     = getCamResetButton();
     = getEnableLookButton();

    // Warping & Pause
     = getWarpLockSwitch();
     = getPhysWarpSwitch();
     = getCancelWarpButton();
     = getDecreaseWarpButton();
     = getIncreaseWarpButton();
     = getPauseButton();

    // SAS & RCS
     = getSASStabilityAssistButton();
     = getSASManeuverButton();
     = getSASProgradeButton();
     = getSASRetrogradeButton();
     = getSASNormalButton();
     = getSASAntiNormalButton();
     = getSASRadialInButton();
     = getSASRadialOutButton();
     = getSASTargetButton();
     = getSASAntiTargetButton();
     = getSASSwitch();
     = getRCSSwitch();

    // EVA Specific Controls
     = getBoardButton();
     = getGrabButton();
     = getJumpButton();

    // Throttle
     = getThrottleLockSwitch();

    // Translation
     = getTransHoldButton();
     = getTransResetButton();

    // Rotation
     = getRotHoldButton();
     = getRotResetButton();
     */
}

void updateAllChecks()
{

    // Misc
    if (Input.getDebugSwitch())
    {

    }
    if (Input.getSoundSwitch())
    {
        // WIP
    }
    if (Input.getInputEnableButton())
    {

    }
    // Warnings
    setTempWarningCancel();       // WIP
    setGeeWarningCancel();        // WIP
    setWarpWarningCancel();       // WIP
    setBrakeWarningCancel();      // WIP
    setSASWarningCancel();        // WIP
    setRCSWarningCancel();        // WIP
    setGearWarningCancel();       // WIP
    setCommsWarningCancel();      // WIP
    setAltWarningCancel();        // WIP
    setPitchWarningCancel();      // WIP
    // Display Controls
    setInfoMode();
    setDirectionMode();
    setVerticalVelocity();
    setReferenceMode();
    setRadarAlt();
    // Resources
    setLFLEDs(); // working
    setSFLEDs(); // working
    setOXLEDs(); // working
    setMPLEDs(); // working
    setECLEDs(); // working
    // Display
    setSpeedLCD();
    setAltitufeLCD();
    setDirectionLCD();
    setHeadingLCD();
    setInfoLCD();
    // Action Groups
    setCAG();
    setStage();
    setAbort();
    setLights();
    setGear();
    setBrake();
    // Custom
    setDocking();
    // View
    setScreenshot();
    setUI();
    setNav();
    setView();
    setFocus();
    setCamMode();
    setCamReset();
    // Warping
    setWarp();
    setPause();
    // SAS & RCS
    setSAS();
    setRCS();
    setAllSASModes();
    // EVA controls
    setBoard();
    setGrab();
    setJump();
    // Send throttle to ksp
    setThrottle();
    // Send translation to ksp
    setTranslation();
    setTranslationHold();
    // Send rotation to ksp
    setRotation();
    setRotationHold();
}

void setDirectionMode()
{
    switch (Input.getDirectionMode()) {}
}
void setInfoMode()
{
    infoMode = Input.getInfoMode();
}
void setReferenceMode()
{
    byte val = Input.getReferenceModeButton();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        mySimpit.cycleNavBallMode();
        break;
    case OFF:
        // Ignore
        break;
    default:
        break;
    }
}
void setRadarAlt()
{
    byte val = Input.getRadarAltitudeSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:

        break;
    case OFF:

        break;
    default:
        break;
    }
}
void setVerticalVelocity()
{
    /*
    if (Input.getVerticalVelocitySwitch())
    {
        if (!Input.getVerticalVelocitySwitch())
        {
            if (currentSpeedMode + 1 == SPEED_VERTICAL_MODE)
            {
                if (currentSpeedMode + 1 >= sizeof(SpeedModes))
                {
                    currentSpeedMode = 0;
                }
                else
                {
                    currentSpeedMode += 2;
                }
            }
            else
            {
                // Next speed mode
                if (currentSpeedMode == sizeof(SpeedModes) - 1)
                {
                    currentSpeedMode = 0;
                }
                else
                {
                    currentSpeedMode++;
                }
            }
        }
        else // Vertical Velocity
        {
            currentSpeedMode = SPEED_VERTICAL_MODE;
        }
    }
    */
}

// Warning Cancel
void setPitchWarningCancel()
{
    if (Input.getPitchWarningButton()) {}
}
void setAltWarningCancel()
{
    if (Input.getAltWarningButton()) {}
}
void setCommsWarningCancel()
{
    if (Input.getCommsWarningButton()) {}
}
void setGearWarningCancel()
{
    if (Input.getGearWarningButton()) {}
}
void setRCSWarningCancel()
{
    if (Input.getRCSWarningButton()) {}
}
void setSASWarningCancel()
{
    if (Input.getSASWarningButton()) {}
}
void setBrakeWarningCancel()
{
    if (Input.getBrakeWarningButton()) {}
}
void setWarpWarningCancel()
{
    if (Input.getWarpWarningButton()) {}
}
void setGeeWarningCancel()
{
    if (Input.getGeeWarningButton()) {}
}
void setTempWarningCancel()
{
    if (Input.getTempWarningButton()) {}
}
// Warnings
void setTempWarning()
{
    /*
    if (tempLimitMsg.tempLimitPercentage > HIGH_TEMP_WARNING_BLINKING_THRESHOLD)
    {
        bool state = blinker1.getState();
        Output.setTempWarningLED(state);
        setSpeaker(true, TEMP_WARNING);
    }
    else if (tempLimitMsg.tempLimitPercentage > HIGH_TEMP_WARNING_SOLID_THRESHOLD)
    {
        // Toggle on
        Output.setTempWarningLED(true);
        setSpeaker(false, TEMP_WARNING);
    }
    else
    {
        // Toggle off
        Output.setTempWarningLED(false);
        setSpeaker(false, TEMP_WARNING);
    }
    */
}
void setGeeWarning()
{
    /*
    if (airspeedMsg.gForces > HIGH_GEE_WARNING_BLINKING_THRESHOLD || airspeedMsg.gForces < -HIGH_GEE_WARNING_BLINKING_THRESHOLD)
    {
        bool state = blinker1.getState();
        Output.setGeeWarningLED(state);
        setSpeaker(true, GEE_WARNING);
    }
    else if (airspeedMsg.gForces > HIGH_GEE_WARNING_SOLID_THRESHOLD || airspeedMsg.gForces < -HIGH_GEE_WARNING_SOLID_THRESHOLD)
    {
        Output.setGeeWarningLED(true);
        setSpeaker(false, GEE_WARNING);
    }
    else
    {
        Output.setGeeWarningLED(false);
        setSpeaker(false, GEE_WARNING);
    }
    */
}
void setWarpWarning()
{
    if (flightStatusMsg.currentTWIndex > 1)
    {
        Output.setWarpWarningLED(true);
    }
    else
    {
        Output.setWarpWarningLED(false);
    }
}
void setCommsWarning()
{
    if (flightStatusMsg.commNetSignalStrenghPercentage < COMMS_WARNING_THRESHOLD)
    {
        Output.setCommsWarningLED(true);
    }
    else
    {
        Output.setCommsWarningLED(false);
    }
}
void setAltWarning()
{
    if (altitudeMsg.surface < LOW_ALTITUDE_WARNING_THRESHOLD && true/*if speed is more than thres && other conditions*/)
    {
        Output.setAltWarningLED(true);
    }
    else
    {
        Output.setAltWarningLED(false);
    }
}
void setPitchWarning()
{

}
// Display
void setSpeedLCD()
{
    /*
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
    */
}
void setAltitufeLCD()
{
    /*
    // Alt
    int altitude;
    // Clear the strings
    String topTxt = "";
    String botTxt = "";
    // Calculate gap for soi name
    // No SOI names are more than 7 char, which is good because that is the exact amount of room at max on the lcd.
    topTxt += calculateGap("", 7);//soi, 7);
    // Check altitude mode
    if (!Input.getRadarAltitudeSwitch()) // SEA
    {
        topTxt += "      Sea";
        altitude = altitudeMsg.sealevel;
    }
    else // LAND
    {
        topTxt += "     Land";
        altitude = altitudeMsg.surface;
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
    */
}
void setInfoLCD()
{
    // Just do this last..
}
void setHeadingLCD()
{
    /*
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
    */
}
void setDirectionLCD()
{
    /*
    // Clear the strings
    String topTxt = "";
    String botTxt = "";
    // Gap after the mode name
    String gap = "    "; // Default gap
    // Declare the values
    int hdg, pth; // No need for roll
    // Check for which mode the use and set the values for that mode

    // REMOVE ME WHEN DONE
    topTxt += "MVR";
    hdg = maneuverMsg.headingNextManeuver;
    pth = maneuverMsg.pitchNextManeuver;

    
    switch (currentDirectionMode)
    {
    case DIRECTION_MANEUVER_MODE:
        topTxt += "MVR";
        hdg = maneuverMsg.headingNextManeuver;
        pth = maneuverMsg.pitchNextManeuver;
        break;
    case DIRECTION_PROGRADE_MODE:
        topTxt += "PGD";
        hdg = ;
        pth = ;
        break;
    case DIRECTION_RETROGRADE_MODE:
        topTxt += "RGD";
        hdg = ;
        pth = ;
        break;
    case DIRECTION_NORMAL_MODE:
        topTxt += "NOR";
        hdg = ;
        pth = ;
        break;
    case DIRECTION_ANTI_NORMAL_MODE:
        topTxt += "A-NOR";
        hdg = ;
        pth = ;
        gap = "  ";
        break;
    case DIRECTION_RADIAL_IN_MODE:
        topTxt += "RAD";
        hdg = ;
        pth = ;
        break;
    case DIRECTION_RADIAL_OUT_MODE:
        topTxt += "A-RAD";
        hdg = ;
        pth = ;
        gap = "  ";
        break;
    case DIRECTION_TARGET_MODE:
        topTxt += "TAR";
        hdg = ;
        pth = ;
        break;
    case DIRECTION_ANTI_TARGET_MODE:
        topTxt += "A-TAR";
        hdg = ;
        pth = ;
        gap = "  ";
        break;
    default:
        break;
    }
    
    // Gap
    topTxt += gap;
    // Heading txt
    topTxt += " HDG+";
    topTxt += formatNumber(hdg, 3, false, false);
    topTxt += DEGREE_CHAR_LCD;
    // Pitch txt
    botTxt += "PTH";
    botTxt += formatNumber(pth, 3, true, false);
    botTxt += DEGREE_CHAR_LCD;

    Output.setDirectionLCD(topTxt, botTxt);
    */
}
// Resouces
void setSFLEDs()
{
    bool newLEDs[20];
    byte val = Input.getStageViewSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(solidFuelStageMsg.total, solidFuelStageMsg.available, newLEDs);
        Output.setSolidFuelLEDs(newLEDs);
        break;
    case OFF:
        calcResource(solidFuelMsg.total, solidFuelMsg.available, newLEDs);
        Output.setSolidFuelLEDs(newLEDs);
        break;
    default:
        break;
    }
}
void setLFLEDs()
{
    bool newLEDs[20]; 
    byte val = Input.getStageViewSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(liquidFuelStageMsg.total, liquidFuelStageMsg.available, newLEDs);
        Output.setLiquidFuelLEDs(newLEDs);
        break;
    case OFF:
        calcResource(liquidFuelMsg.total, liquidFuelMsg.available, newLEDs);
        Output.setLiquidFuelLEDs(newLEDs);
        break;
    default:
        break;
    }
}
void setOXLEDs()
{
    bool newLEDs[20];
    byte val = Input.getStageViewSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        calcResource(oxidizerStageMsg.total, oxidizerStageMsg.available, newLEDs);
        Output.setOxidizerLEDs(newLEDs);
        break;
    case OFF:
        calcResource(oxidizerMsg.total, oxidizerMsg.available, newLEDs);
        Output.setOxidizerLEDs(newLEDs);
        break;
    default:
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
    Output.setMonopropellantLEDs(newLEDs);
}
void setECLEDs()
{
    bool newLEDs[20];
    calcResource(electricityMsg.total, electricityMsg.available, newLEDs);
    Output.setElectricityLEDs(newLEDs);
}
// Other action groups
void setStage()
{
    byte lock = Input.getStageLockSwitch(false);
    mySimpit.printToKSP("LOCK: " + String(lock), PRINT_TO_SCREEN);
    switch (lock)
    {
    case NOT_READY:
        break;
    case ON:
        Output.setStageLED(true);
        if (Input.getStageButton() == ON)
            mySimpit.activateAction(STAGE_ACTION);
        break;
    case OFF:
        Output.setStageLED(false);
        break;
    default:
        break;
    }
}
void setAbort()
{
    byte lock = Input.getAbortLockSwitch();
    switch (lock)
    {
    case NOT_READY:
        break;
    case ON:
        Output.setAbortLED(true);
        if (Input.getAbortButton() == ON)
            mySimpit.activateAction(ABORT_ACTION);
        break;
    case OFF:
        Output.setAbortLED(false);
        break;
    default:
        break;
    }
}
void setLights()
{
    byte val = Input.getLightsSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:
        mySimpit.activateAction(LIGHT_ACTION);
        break;
    case OFF:
        mySimpit.deactivateAction(LIGHT_ACTION);
        break;
    default:
        break;
    }
}
void setGear()
{
    byte val = Input.getGearSwitch();
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
    default:
        break;
    }
}
void setBrake()
{
    byte val = Input.getBrakeSwitch();
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
    default:
        break;
    }

    if (ag.isBrake)
        Output.setBrakeWarningLED(true);
    else
        Output.setBrakeWarningLED(false);
}
// Custom
void setDocking()
{
    byte val = Input.getDockingSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON:

        break;
    case OFF:

        break;
    default:
        break;
    }
}
// Custom action groups
void setCAG()
{
    // Custom Action Groups

    if (Input.getCAG1() == ON)
        mySimpit.toggleCAG(1);

    if (Input.getCAG2() == ON)
        mySimpit.toggleCAG(2);

    if (Input.getCAG3() == ON)
        mySimpit.toggleCAG(3);

    if (Input.getCAG4() == ON)
        mySimpit.toggleCAG(4);

    if (Input.getCAG5() == ON)
        mySimpit.toggleCAG(5);

    if (Input.getCAG6() == ON)
        mySimpit.toggleCAG(6);

    if (Input.getCAG7() == ON)
        mySimpit.toggleCAG(7);

    if (Input.getCAG8() == ON)
        mySimpit.toggleCAG(8);

    if (Input.getCAG9() == ON)
        mySimpit.toggleCAG(9);

    if (Input.getCAG10() == ON)
        mySimpit.toggleCAG(10);


    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[0]))
        Output.setCAG1LED(true);
    else
        Output.setCAG1LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[1]))
        Output.setCAG2LED(true);
    else
        Output.setCAG2LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[2]))
        Output.setCAG3LED(true);
    else
        Output.setCAG3LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[3]))
        Output.setCAG4LED(true);
    else
        Output.setCAG4LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[4]))
        Output.setCAG5LED(true);
    else
        Output.setCAG5LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[5]))
        Output.setCAG6LED(true);
    else
        Output.setCAG6LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[6]))
        Output.setCAG7LED(true);
    else
        Output.setCAG7LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[7]))
        Output.setCAG8LED(true);
    else
        Output.setCAG8LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[8]))
        Output.setCAG9LED(true);
    else
        Output.setCAG9LED(false);

    if (cagStatusMsg.is_action_activated(cagStatusMsg.status[9]))
        Output.setCAG10LED(true);
    else
        Output.setCAG10LED(false);
}
// View
void setCamReset()
{
    if (Input.getCamResetButton() == ON)
        mySimpit.setCameraMode(FLIGHT_CAMERA_AUTO);
}
void setCamMode()
{
    if (Input.getCamModeButton() == ON)
        mySimpit.setCameraMode(CAMERA_NEXT_MODE);
}
void setFocus()
{
    if (Input.getFocusButton() == ON) {}
}
void setView()
{
    byte val = Input.getViewSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON: // External

        break;
    case OFF: // Internal (IVA)

        break;
    default:
        break;
    }
}
void setNav()
{
    byte val = Input.getNavSwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON: // Map

        break;
    case OFF: // Flight View

        break;
    default:
        break;
    }
}
void setUI()
{
    byte val = Input.getUISwitch();
    switch (val)
    {
    case NOT_READY:
        break;
    case ON: // Show UI

        break;
    case OFF: // Disable UI

        break;
    default:
        break;
    }
}
void setScreenshot()
{
    if (Input.getScreenshotButton() == ON) {}
}
// Warping & Pause
void setWarp()
{
    timewarpMessage twMsg;
    byte lock = Input.getWarpLockSwitch();

    // Cancel Warp
    if (lock == OFF || Input.getCancelWarpButton() == ON)
    {
        twMsg.command = TIMEWARP_X1;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    // This is different than before, this will not return NOT_READY
    if (Input.getWarpLockSwitch(false) == OFF) 
        return; // Ignore rest

    if (Input.getPhysWarpSwitch())
    {
        // NOT IMPLEMENTED IN KspSimPit!!!
    }

    if (Input.getIncreaseWarpButton()) 
    {
        twMsg.command = TIMEWARP_UP;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
    if (Input.getDecreaseWarpButton()) 
    {
        twMsg.command = TIMEWARP_DOWN;
        mySimpit.send(TIMEWARP_MESSAGE, twMsg);
        return;
    }
}
void setPause()
{
    if (Input.getPauseButton()) 
    {
        
    }
}
// SAS & RCS
void setSAS()
{
    if (Input.getSASSwitch())
        mySimpit.activateAction(SAS_ACTION);
    else
        mySimpit.deactivateAction(SAS_ACTION);

    if (ag.isSAS)
        Output.setSASWarningLED(true);
    else
        Output.setSASWarningLED(false);
}
void setRCS()
{
    if (Input.getRCSSwitch())
        mySimpit.activateAction(RCS_ACTION);
    else
        mySimpit.deactivateAction(RCS_ACTION);

    if (ag.isRCS)
        Output.setRCSWarningLED(true);
    else
        Output.setRCSWarningLED(false);
}
void setAllSASModes()
{
    // SAS Modes
    if (Input.getSASStabilityAssistButton())
        setSASMode(AP_STABILITYASSIST);
    if (Input.getSASManeuverButton())
        setSASMode(AP_MANEUVER);
    if (Input.getSASProgradeButton())
        setSASMode(AP_PROGRADE);
    if (Input.getSASRetrogradeButton())
        setSASMode(AP_RETROGRADE);
    if (Input.getSASNormalButton())
        setSASMode(AP_NORMAL);
    if (Input.getSASAntiNormalButton())
        setSASMode(AP_ANTINORMAL);
    if (Input.getSASRadialInButton())
        setSASMode(AP_RADIALIN);
    if (Input.getSASRadialOutButton())
        setSASMode(AP_RADIALOUT);
    if (Input.getSASTargetButton())
        setSASMode(AP_TARGET);
    if (Input.getSASAntiTargetButton())
        setSASMode(AP_ANTITARGET);

    switch (sasInfoMsg.currentSASMode)
    {
        Output.setSASStabilityAssistLED(false);
        Output.setSASManeuverLED(false);
        Output.setSASProgradeLED(false);
        Output.setSASRetrogradeLED(false);
        Output.setSASNormalLED(false);
        Output.setSASAntiNormalLED(false);
        Output.setSASRadialInLED(false);
        Output.setSASRadialOutLED(false);
        Output.setSASTargetLED(false);
        Output.setSASAntiTargetLED(false);

    case AP_STABILITYASSIST:
        Output.setSASStabilityAssistLED(true);
        break;
    case AP_PROGRADE:
        Output.setSASProgradeLED(true);
        break;
    case AP_RETROGRADE:
        Output.setSASRetrogradeLED(true);
        break;
    case AP_NORMAL:
        Output.setSASNormalLED(true);
        break;
    case AP_ANTINORMAL:
        Output.setSASAntiNormalLED(true);
        break;
    case AP_RADIALIN:
        Output.setSASRadialInLED(true);
        break;
    case AP_RADIALOUT:
        Output.setSASRadialOutLED(true);
        break;
    case AP_TARGET:
        Output.setSASTargetLED(true);
        break;
    case AP_ANTITARGET:
        Output.setSASAntiTargetLED(true);
        break;
    case AP_MANEUVER:
        Output.setSASManeuverLED(true);
        break;
    default:
        break;
    }
}
void setSASMode(AutopilotMode mode)
{
    // Make sure this mode is available
    if (sasInfoMsg.SASModeAvailability != mode)
        return;
    // Set to this mode
    mySimpit.setSASMode(mode);
}
// EVA controls
void setJump()
{
    if (Input.getJumpButton()) {}
}
void setGrab()
{
    if (Input.getGrabButton()) {}
}
void setBoard()
{
    if (Input.getBoardButton()) {}
}
// Throttle
void setThrottle()
{
    int axis = 0;
    axis = Input.getThrottleAxis();
    // Smooth and map the raw input
    int16_t throttle = 0;
    // If toggled on
    if (!Input.getThrottleLockSwitch())
        throttle = smoothAndMapAxis(axis);

    // Create new throttle msg
    throttleMessage throttleMsg;
    // Set values in msg
    throttleMsg.throttle = throttle;
    // Send msg
    mySimpit.send(THROTTLE_MESSAGE, throttleMsg);
}
// Translation
void setTranslation()
{
    if (Input.getTransResetButton())
        translationHold = false;
    if (translationHold)
        return;
    int x, y, z;
    x = Input.getTranslationXAxis();
    y = Input.getTranslationYAxis();
    z = Input.getTranslationZAxis();

    if (Input.getPercisionSwitch())
    {
        x *= PERCISION_MODIFIER;
        y *= PERCISION_MODIFIER;
        z *= PERCISION_MODIFIER;
    }

    // Smoothing and mapping
    int16_t transX = smoothAndMapAxis(x);
    int16_t transY = smoothAndMapAxis(y);
    int16_t transZ = smoothAndMapAxis(z);
    // Creating and setting values for msg
    translationMessage transMsg;
    transMsg.setXYZ(transX, transZ, transY);
    // Send msg to ksp
    mySimpit.send(TRANSLATION_MESSAGE, transMsg);
}
void setTranslationHold()
{
    if (Input.getTransHoldButton())
        translationHold = true;
    else
        translationHold = false;
}
// Rotation
void setRotation()
{

    int x, y, z;
    x = Input.getRotationXAxis();
    y = Input.getRotationYAxis();
    z = Input.getRotationZAxis();

    if (Input.getPercisionSwitch())
    {
        x *= PERCISION_MODIFIER;
        y *= PERCISION_MODIFIER;
        z *= PERCISION_MODIFIER;
    }

    if (Input.getEnableLookButton())
    {
        // Look around instead

        return;
    }

    if (Input.getRotResetButton())
        translationHold = false;
    if (translationHold)
        return;


    // Smoothing and mapping
    int16_t rotX = smoothAndMapAxis(x);
    int16_t rotY = smoothAndMapAxis(y);
    int16_t rotZ = smoothAndMapAxis(z);
    // Flip some values the right way
    rotX *= -1;
    // Creating and setting values for msg
    rotationMessage rotMsg;
    rotMsg.setPitchRollYaw(rotY, rotX, rotZ);
    // Send msg to ksp
    mySimpit.send(ROTATION_MESSAGE, rotMsg);
}
void setRotationHold()
{
    if (Input.getRotHoldButton())
        rotationHold = true;
    else
        rotationHold = false;
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
            else str += "0";
        }
        else str += "0";
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
