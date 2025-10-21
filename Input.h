/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Input.h

#ifndef _INPUT_h
#define _INPUT_h

// Virtual Pin Definitions
// Test Buttons
#define VPIN_TEST_BUTTON 1
#define VPIN_TEST_SWITCH 2

// Miscellaneous
#define VPIN_DEBUG_SWITCH 0
#define VPIN_SOUND_SWITCH 3
#define VPIN_INPUT_ENABLE_BUTTON 4

// Warnings
#define VPIN_TEMP_WARNING_BUTTON 5
#define VPIN_GEE_WARNING_BUTTON 6
#define VPIN_WARP_WARNING_BUTTON 7
#define VPIN_BRAKE_WARNING_BUTTON 8
#define VPIN_SAS_WARNING_BUTTON 9
#define VPIN_RCS_WARNING_BUTTON 10
#define VPIN_GEAR_WARNING_BUTTON 11
#define VPIN_COMMS_WARNING_BUTTON 12
#define VPIN_ALT_WARNING_BUTTON 13
#define VPIN_PITCH_WARNING_BUTTON 14

// Display Controls
#define VPIN_INFO_MODE 15
#define VPIN_DIRECTION_MODE 16
#define VPIN_STAGE_VIEW_SWITCH 17
#define VPIN_VERTICAL_VELOCITY_SWITCH 18
#define VPIN_REFERENCE_MODE_BUTTON 19
#define VPIN_RADAR_ALTITUDE_SWITCH 20

// Staging
#define VPIN_STAGE_BUTTON 21
#define VPIN_STAGE_LOCK_SWITCH 22

// Aborting
#define VPIN_ABORT_BUTTON 23
#define VPIN_ABORT_LOCK_SWITCH 24

// Custom Action Groups
#define VPIN_CAG1 25
#define VPIN_CAG2 26
#define VPIN_CAG3 27
#define VPIN_CAG4 28
#define VPIN_CAG5 29
#define VPIN_CAG6 30
#define VPIN_CAG7 31
#define VPIN_CAG8 32
#define VPIN_CAG9 33
#define VPIN_CAG10 34

// Other Action Groups
#define VPIN_DOCKING_SWITCH 35
#define VPIN_PRECISION_SWITCH 36
#define VPIN_LIGHTS_SWITCH 37
#define VPIN_GEAR_SWITCH 38
#define VPIN_BRAKE_SWITCH 39

// View
#define VPIN_SCREENSHOT_BUTTON 40
#define VPIN_UI_SWITCH 41
#define VPIN_NAV_SWITCH 42
#define VPIN_VIEW_SWITCH 43
#define VPIN_FOCUS_BUTTON 44
#define VPIN_CAM_MODE_BUTTON 45
#define VPIN_CAM_RESET_BUTTON 46
#define VPIN_ENABLE_LOOK_BUTTON 47

// Warping & Pause
#define VPIN_WARP_LOCK_SWITCH 48
#define VPIN_PHYS_WARP_SWITCH 49
#define VPIN_CANCEL_WARP_BUTTON 50
#define VPIN_DECREASE_WARP_BUTTON 51
#define VPIN_INCREASE_WARP_BUTTON 52
#define VPIN_PAUSE_BUTTON 53

// SAS & RCS
#define VPIN_SAS_STABILITY_ASSIST_BUTTON 54
#define VPIN_SAS_MANEUVER_BUTTON 55
#define VPIN_SAS_PROGRADE_BUTTON 56
#define VPIN_SAS_RETROGRADE_BUTTON 57
#define VPIN_SAS_NORMAL_BUTTON 58
#define VPIN_SAS_ANTI_NORMAL_BUTTON 59
#define VPIN_SAS_RADIAL_IN_BUTTON 60
#define VPIN_SAS_RADIAL_OUT_BUTTON 61
#define VPIN_SAS_TARGET_BUTTON 62
#define VPIN_SAS_ANTI_TARGET_BUTTON 63
#define VPIN_SAS_SWITCH 64
#define VPIN_RCS_SWITCH 65

// EVA Specific Controls
#define VPIN_BOARD_BUTTON 66
#define VPIN_GRAB_BUTTON 67
#define VPIN_JUMP_BUTTON 68

// Throttle
#define VPIN_THROTTLE_LOCK_SWITCH 69

// Translation
#define VPIN_TRANS_HOLD_BUTTON 70
#define VPIN_TRANS_RESET_BUTTON 71

// Rotation
#define VPIN_ROT_HOLD_BUTTON 72
#define VPIN_ROT_RESET_BUTTON 73

enum ButtonState
{
    OFF = 0,
    ON = 1,
    NOT_READY = 255,
};

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif


class InputClass
{
private:
    Stream* debugSerial = nullptr;  // Add this member variable
    
public:
	 void init(Stream& serial);     // Modified init signature
	 void update();
     void setAllVPinsReady();

     ButtonState getVirtualPin(int virtualPinNumber, bool waitForChange = true);

     // Throttle
     int getThrottleAxis(); 

     // Translation
     int getTranslationXAxis(); 
     int getTranslationYAxis(); 
     int getTranslationZAxis(); 

     // Rotation
     int getRotationXAxis(); 
     int getRotationYAxis(); 
     int getRotationZAxis(); 

     // Debugging
     void debugInputState(int virtualPinNumber);  
     void debugSASWarningButton();

	 ~InputClass();  // Destructor declaration
};

extern InputClass Input;

#endif
