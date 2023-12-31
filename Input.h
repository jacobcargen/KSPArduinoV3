/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Input.h

#ifndef _INPUT_h
#define _INPUT_h

enum ButtonState
{
    OFF = 0,
    ON = 1,
    NOT_READY = 255,
};

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class InputClass
{
 protected:


 public:

	 void init();
	 void update();
     void setAllVPinsReady();

     // Test Buttons
     byte getTestButton(bool waitForChange = true);
     byte getTestSwitch(bool waitForChange = true);

     // Miscellaneous
     byte getDebugSwitch(bool waitForChange = true);
     byte getSoundSwitch(bool waitForChange = true);
     byte getInputEnableButton(bool waitForChange = true);

     // Warnings
     byte getTempWarningButton(bool waitForChange = true);
     byte getGeeWarningButton(bool waitForChange = true);
     byte getWarpWarningButton(bool waitForChange = true);
     byte getBrakeWarningButton(bool waitForChange = true);
     byte getSASWarningButton(bool waitForChange = true);
     byte getRCSWarningButton(bool waitForChange = true);
     byte getGearWarningButton(bool waitForChange = true);
     byte getCommsWarningButton(bool waitForChange = true);
     byte getAltWarningButton(bool waitForChange = true);
     byte getPitchWarningButton(bool waitForChange = true);

     // Display Controls
     byte getInfoMode();
     byte getDirectionMode();
     byte getStageViewSwitch(bool waitForChange = true);
     byte getVerticalVelocitySwitch(bool waitForChange = true);
     byte getReferenceModeButton(bool waitForChange = true);
     byte getRadarAltitudeSwitch(bool waitForChange = true);

     // Staging
     byte getStageButton(bool waitForChange = true);
     byte getStageLockSwitch(bool waitForChange = true);

     // Aborting
     byte getAbortButton(bool waitForChange = true);
     byte getAbortLockSwitch(bool waitForChange = true);

     // Custom Action Groups
     byte getCAG1(bool waitForChange = true); 
     byte getCAG2(bool waitForChange = true); 
     byte getCAG3(bool waitForChange = true); 
     byte getCAG4(bool waitForChange = true); 
     byte getCAG5(bool waitForChange = true); 
     byte getCAG6(bool waitForChange = true); 
     byte getCAG7(bool waitForChange = true); 
     byte getCAG8(bool waitForChange = true); 
     byte getCAG9(bool waitForChange = true); 
     byte getCAG10(bool waitForChange = true);

     // Other Action Groups
     byte getDockingSwitch(bool waitForChange = true);
     byte getPercisionSwitch(bool waitForChange = true);
     byte getLightsSwitch(bool waitForChange = true);
     byte getGearSwitch(bool waitForChange = true);
     byte getBrakeSwitch(bool waitForChange = true);

     // View
     byte getScreenshotButton(bool waitForChange = true);
     byte getUISwitch(bool waitForChange = true);
     byte getNavSwitch(bool waitForChange = true);
     byte getViewSwitch(bool waitForChange = true);
     byte getFocusButton(bool waitForChange = true);
     byte getCamModeButton(bool waitForChange = true);
     byte getCamResetButton(bool waitForChange = true);
     byte getEnableLookButton(bool waitForChange = true);

     // Warping & Pause
     byte getWarpLockSwitch(bool waitForChange = true);
     byte getPhysWarpSwitch(bool waitForChange = true);
     byte getCancelWarpButton(bool waitForChange = true);
     byte getDecreaseWarpButton(bool waitForChange = true);
     byte getIncreaseWarpButton(bool waitForChange = true);
     byte getPauseButton(bool waitForChange = true);

     // SAS & RCS
     byte getSASStabilityAssistButton(bool waitForChange = true);
     byte getSASManeuverButton(bool waitForChange = true);
     byte getSASProgradeButton(bool waitForChange = true);
     byte getSASRetrogradeButton(bool waitForChange = true);
     byte getSASNormalButton(bool waitForChange = true);
     byte getSASAntiNormalButton(bool waitForChange = true);
     byte getSASRadialInButton(bool waitForChange = true);
     byte getSASRadialOutButton(bool waitForChange = true);
     byte getSASTargetButton(bool waitForChange = true);
     byte getSASAntiTargetButton(bool waitForChange = true);
     byte getSASSwitch(bool waitForChange = true);
     byte getRCSSwitch(bool waitForChange = true);

     // EVA Specific Controls
     byte getBoardButton(bool waitForChange = true);
     byte getGrabButton(bool waitForChange = true);
     byte getJumpButton(bool waitForChange = true);

     // Throttle
     int getThrottleAxis(); 
     byte getThrottleLockSwitch(bool waitForChange = true);

     // Translation
     int getTranslationXAxis(); 
     int getTranslationYAxis(); 
     int getTranslationZAxis(); 
     byte getTransHoldButton(bool waitForChange = true);
     byte getTransResetButton(bool waitForChange = true);

     // Rotation
     int getRotationXAxis(); 
     int getRotationYAxis(); 
     int getRotationZAxis(); 
     byte getRotHoldButton(bool waitForChange = true);
     byte getRotResetButton(bool waitForChange = true);


	 ~InputClass();  // Destructor declaration
};

extern InputClass Input;

#endif
