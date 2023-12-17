/*
 Name:		Kerbal_Controller_Arduino rev2.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Input.h

#ifndef _INPUT_h
#define _INPUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
	#include <pins_arduino.h> 
	#include <variant.h> 
	#include <variant.cpp> 
#else
	#include "WProgram.h"
#endif

class InputClass
{
 protected:


 public:

	 void init();

	 void update();

	 // Misc

	 bool getDebugSwitch();
	 bool getSoundSwitch();
	 bool getInputEnableButton();

	 // Warnings

	 bool getTempWarningButton(); 
	 bool getGeeWarningButton();
	 bool getWarpWarningButton();
	 bool getBrakeWarningButton();
	 bool getSASWarningButton();
	 bool getRCSWarningButton();
	 bool getGearWarningButton();
	 bool getCommsWarningButton();
	 bool getAltWarningButton();
	 bool getPitchWarningButton();

	 // Display Controls

	 byte getInfoMode();
	 byte getDirectionMode();
	 bool getStageViewSwitch();
	 bool getVerticalVelocitySwitch();
	 bool getReferenceModeButton(); // WIP
	 bool getRadarAltitudeSwitch();

	 // Staging

	 bool getStageButton(); // done
	 bool getStageLockSwitch(); // done

	 // Aborting

	 bool getAbortButton(); // done
	 bool getAbortLockSwitch(); // done

	 // Custom Action Groups

	 bool getCAG1(); // done
	 bool getCAG2(); // done
	 bool getCAG3(); // done
	 bool getCAG4(); // done
	 bool getCAG5(); // done
	 bool getCAG6(); // done
	 bool getCAG7(); // done
	 bool getCAG8(); // done
	 bool getCAG9(); // done
	 bool getCAG10(); // done

	 // Other Action Groups

	 bool getDockingSwitch(); // WIP
	 bool getPercisionSwitch(); // done
	 bool getLightsSwitch(); // done
	 bool getGearSwitch(); // done
	 bool getBrakeSwitch(); //done

	 // View

	 bool getScreenshotButton();
	 bool getUISwitch();
	 bool getNavSwitch();
	 bool getViewSwitch();
	 bool getFocusButton();
	 bool getCamModeButton();
	 bool getCamResetButton();
	 bool getEnableLookButton();

	 // Warping & Pause

	 bool getWarpLockSwitch();
	 bool getPhysWarpSwitch();
	 bool getCancelWarpButton();
	 bool getDecreaseWarpButton();
	 bool getIncreaseWarpButton();
	 bool getPauseButton();

	 // SAS & RCS

	 bool getSASStabilityAssistButton(); // done
	 bool getSASManeuverButton();		 // done
	 bool getSASProgradeButton();		 // done
	 bool getSASRetrogradeButton();		 // done
	 bool getSASNormalButton();			 // done
	 bool getSASAntiNormalButton();		 // done
	 bool getSASRadialInButton();		 // done
	 bool getSASRadialOutButton();		 // done
	 bool getSASTargetButton();			 // done
	 bool getSASAntiTargetButton();		 // done
	 bool getSASSwitch(); // done
	 bool getRCSSwitch(); // done

	 // EVA Specific Controls

	 bool getBoardButton();
	 bool getGrabButton();
	 bool getJumpButton();

	 // Throttle

	 int  getThrottleAxis(); // done
	 bool getThrottleLockSwitch(); // done

	 // Translation

	 int  getTranslationXAxis(); // done
	 int  getTranslationYAxis(); // done
	 int  getTranslationZAxis(); // done
	 bool getTransHoldButton(); // WIP
	 bool getTransResetButton(); // WIP

	 // Rotation

	 int  getRotationXAxis(); // done
	 int  getRotationYAxis(); // done
	 int  getRotationZAxis(); // done
	 bool getRotHoldButton(); // WIP
	 bool getRotResetButton(); // WIP

};

extern InputClass Input;

#endif
