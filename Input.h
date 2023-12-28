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

     // Test Buttons
     byte getTestButton();
     byte getTestSwitch(byte state);

     // Miscellaneous
     byte getDebugSwitch();
     byte getSoundSwitch();
     byte getInputEnableButton();

     // Warnings
     byte getTempWarningButton();
     byte getGeeWarningButton();
     byte getWarpWarningButton();
     byte getBrakeWarningButton();
     byte getSASWarningButton();
     byte getRCSWarningButton();
     byte getGearWarningButton();
     byte getCommsWarningButton();
     byte getAltWarningButton();
     byte getPitchWarningButton();

     // Display Controls
     byte getInfoMode();
     byte getDirectionMode();
     byte getStageViewSwitch();
     byte getVerticalVelocitySwitch();
     byte getReferenceModeButton(); // WIP
     byte getRadarAltitudeSwitch();

     // Staging
     byte getStageButton(); // done
     byte getStageLockSwitch(); // done

     // Aborting
     byte getAbortButton(); // done
     byte getAbortLockSwitch(); // done

     // Custom Action Groups
     byte getCAG1(); // done
     byte getCAG2(); // done
     byte getCAG3(); // done
     byte getCAG4(); // done
     byte getCAG5(); // done
     byte getCAG6(); // done
     byte getCAG7(); // done
     byte getCAG8(); // done
     byte getCAG9(); // done
     byte getCAG10(); // done

     // Other Action Groups
     byte getDockingSwitch(); // WIP
     byte getPercisionSwitch(); // done
     byte getLightsSwitch(); // done
     byte getGearSwitch(); // done
     byte getBrakeSwitch(); // done

     // View
     byte getScreenshotButton();
     byte getUISwitch();
     byte getNavSwitch();
     byte getViewSwitch();
     byte getFocusButton();
     byte getCamModeButton();
     byte getCamResetButton();
     byte getEnableLookButton(); // WIP

     // Warping & Pause
     byte getWarpLockSwitch();
     byte getPhysWarpSwitch();
     byte getCancelWarpButton();
     byte getDecreaseWarpButton();
     byte getIncreaseWarpButton();
     byte getPauseButton();

     // SAS & RCS
     byte getSASStabilityAssistButton(); // done
     byte getSASManeuverButton();         // done
     byte getSASProgradeButton();         // done
     byte getSASRetrogradeButton();       // done
     byte getSASNormalButton();           // done
     byte getSASAntiNormalButton();       // done
     byte getSASRadialInButton();         // done
     byte getSASRadialOutButton();        // done
     byte getSASTargetButton();           // done
     byte getSASAntiTargetButton();       // done
     byte getSASSwitch();                 // done
     byte getRCSSwitch();                 // done

     // EVA Specific Controls
     byte getBoardButton();
     byte getGrabButton();
     byte getJumpButton();

     // Throttle
     int getThrottleAxis(); // done
     byte getThrottleLockSwitch(); // done

     // Translation
     int getTranslationXAxis(); // done
     int getTranslationYAxis(); // done
     int getTranslationZAxis(); // done
     byte getTransHoldButton(); // WIP
     byte getTransResetButton(); // WIP

     // Rotation
     int getRotationXAxis(); // done
     int getRotationYAxis(); // done
     int getRotationZAxis(); // done
     byte getRotHoldButton(); // WIP
     byte getRotResetButton(); // WIP


	 ~InputClass();  // Destructor declaration
};

extern InputClass Input;

#endif
