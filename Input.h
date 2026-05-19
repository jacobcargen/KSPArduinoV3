/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Input.h

#include <pins_arduino.h> 

#ifndef _INPUT_h
#define _INPUT_h

// Virtual Pin Definitions
// Test Buttons
#define VPIN_TEST_BUTTON                     0
#define VPIN_TEST_SWITCH                     0

// Miscellaneous
#define VPIN_DEBUG_SWITCH 0
#define VPIN_SOUND_SWITCH 1
#define VPIN_MOD_BUTTON 2 // MOD KEY
#define VPIN_AUTO_PILOT_SWITCH 58

// Warnings
#define VPIN_TEMP_WARNING_BUTTON 5
#define VPIN_GEE_WARNING_BUTTON 6
#define VPIN_WARP_WARNING_BUTTON 10
#define VPIN_BRAKE_WARNING_BUTTON 3
#define VPIN_SAS_WARNING_BUTTON 9
#define VPIN_RCS_WARNING_BUTTON 4
#define VPIN_GEAR_WARNING_BUTTON 12
#define VPIN_COMMS_WARNING_BUTTON 7
#define VPIN_ALT_WARNING_BUTTON 8
#define VPIN_PITCH_WARNING_BUTTON 11

// Display Controls
#define VPIN_INFO_MODE_1 38
#define VPIN_INFO_MODE_2 32
#define VPIN_INFO_MODE_3 18
#define VPIN_INFO_MODE_4 25
#define VPIN_INFO_MODE_5 21
#define VPIN_INFO_MODE_6 27
#define VPIN_INFO_MODE_7 26
#define VPIN_INFO_MODE_8 24
#define VPIN_INFO_MODE_9 19
#define VPIN_INFO_MODE_10 28
#define VPIN_INFO_MODE_11 35
#define VPIN_INFO_MODE_12 39

#define VPIN_DIRECTION_MODE_1 29
#define VPIN_DIRECTION_MODE_2 34
#define VPIN_DIRECTION_MODE_3 37
#define VPIN_DIRECTION_MODE_4 40
#define VPIN_DIRECTION_MODE_5 22
#define VPIN_DIRECTION_MODE_6 20
#define VPIN_DIRECTION_MODE_7 17
#define VPIN_DIRECTION_MODE_8 23
#define VPIN_DIRECTION_MODE_9 33
#define VPIN_DIRECTION_MODE_10 36
#define VPIN_DIRECTION_MODE_11 31
#define VPIN_DIRECTION_MODE_12 30

#define VPIN_STAGE_VIEW_SWITCH 16 
#define VPIN_VERTICAL_VELOCITY_SWITCH 101
#define VPIN_REFERENCE_MODE_BUTTON 13
#define VPIN_RADAR_ALTITUDE_SWITCH 14


// Staging
#define VPIN_STAGE_BUTTON 43
#define VPIN_STAGE_LOCK_SWITCH 44

// Aborting
#define VPIN_ABORT_BUTTON 41
#define VPIN_ABORT_LOCK_SWITCH 42

// Custom Action Groups
#define VPIN_CAG1 48
#define VPIN_CAG2 45
#define VPIN_CAG3 54
#define VPIN_CAG4 51
#define VPIN_CAG5 49
#define VPIN_CAG6 46
#define VPIN_CAG7 47
#define VPIN_CAG8 52
#define VPIN_CAG9 53
#define VPIN_CAG10 50

// Other Action Groups
#define VPIN_DOCKING_SWITCH 57
#define VPIN_PRECISION_SWITCH 55
#define VPIN_LIGHTS_SWITCH 59
#define VPIN_GEAR_SWITCH 56
#define VPIN_BRAKE_SWITCH 63

// View
#define VPIN_UI_BUTTON 95
#define VPIN_DUAL_SWITCH 91
#define VPIN_NAV_SWITCH 89
#define VPIN_VIEW_SWITCH 87
#define VPIN_FOCUS_BUTTON 86
#define VPIN_CAM_MODE_BUTTON 84
#define VPIN_CAM_RESET_BUTTON 93

// Warping & Pause
#define VPIN_WARP_LOCK_SWITCH 68
#define VPIN_CANCEL_WARP_BUTTON 94
#define VPIN_DECREASE_WARP_BUTTON 96
#define VPIN_INCREASE_WARP_BUTTON 92
#define VPIN_PAUSE_BUTTON 98

// SAS & RCS
#define VPIN_SAS_STABILITY_ASSIST_BUTTON 65
#define VPIN_SAS_MANEUVER_BUTTON 75
#define VPIN_SAS_PROGRADE_BUTTON 78
#define VPIN_SAS_RETROGRADE_BUTTON 74
#define VPIN_SAS_NORMAL_BUTTON 66
#define VPIN_SAS_ANTI_NORMAL_BUTTON 64
#define VPIN_SAS_RADIAL_IN_BUTTON 73
#define VPIN_SAS_RADIAL_OUT_BUTTON 67
#define VPIN_SAS_TARGET_BUTTON 72
#define VPIN_SAS_ANTI_TARGET_BUTTON 79
#define VPIN_SAS_SWITCH 61
#define VPIN_RCS_SWITCH 62

// EVA Specific Controls
#define VPIN_BOARD_BUTTON 85
#define VPIN_GRAB_BUTTON 100

// Throttle
#define VPIN_THROTTLE_LOCK_SWITCH 69

// Translation
#define VPIN_TRANS_HOLD_BUTTON 99
#define VPIN_TRANS_RESET_BUTTON 97

// Rotation
#define VPIN_ROT_HOLD_BUTTON 88
#define VPIN_ROT_RESET_BUTTON 90

#define VPIN_TRANSLATION_BUTTON 82
#define VPIN_ROTATION_BUTTON 83

// Rotation Joystick X-Axis(Roll)
const int ROTATION_X_AXIS_PIN = A3;
// Rotation Joystick Y-Axis(Pitch)
const int ROTATION_Y_AXIS_PIN = A0;
// Rotation Joystick Z-Axis(Yaw)
const int ROTATION_Z_AXIS_PIN = A1;
// Rotation Joystick Button
const int ROTATION_BUTTON_PIN = A2;
// Translation Joystick X-Axis(Left/Right)
const int TRANSLATION_X_AXIS_PIN = A7;
// Translation Joystick Y-Axis(Forward/Back)
const int TRANSLATION_Y_AXIS_PIN = A6;
// Translation Joystick Z-Axis(Up/Down)
const int TRANSLATION_Z_AXIS_PIN = A5;
// Translation Joystick Button
const int TRANSLATION_BUTTON_PIN = A4;
// Throttle Axis
const int THROTTLE_AXIS_PIN = A8;

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
    Stream* debugSerial = nullptr;
    
public:
	void init(Stream& serial);
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
