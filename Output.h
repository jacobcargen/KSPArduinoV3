/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Output.h
#include <pins_arduino.h> 
#include <variant.h> 
#include <LiquidCrystal_I2C.h>

#ifndef _OUTPUT_h
#define _OUTPUT_h

// VPin structure definition
struct VPin
{
    char reg;  // 'A', 'B', or 'C'
    int pin;  // 0-63 for A/B, 0-15 for C
};

// LED Virtual Pin Definitions
const VPin VLED_POWER = {'A', 0};

// Warnings
const VPin VLED_TEMP_WARNING = {'A', 1};
const VPin VLED_GEE_WARNING = {'A', 2};
const VPin VLED_WARP_WARNING = {'A', 3};
const VPin VLED_BRAKE_WARNING = {'A', 4};
const VPin VLED_SAS_WARNING = {'A', 5};
const VPin VLED_RCS_WARNING = {'A', 6};
const VPin VLED_GEAR_WARNING = {'A', 7};
const VPin VLED_COMMS_WARNING = {'A', 8};
const VPin VLED_ALT_WARNING = {'A', 9};
const VPin VLED_PITCH_WARNING = {'A', 10};

// Staging
const VPin VLED_STAGE = {'A', 11};

// Aborting
const VPin VLED_ABORT = {'A', 12};

// Custom Action Groups
const VPin VLED_CAG1 = {'A', 13};
const VPin VLED_CAG2 = {'A', 14};
const VPin VLED_CAG3 = {'A', 15};
const VPin VLED_CAG4 = {'A', 16};
const VPin VLED_CAG5 = {'A', 17};
const VPin VLED_CAG6 = {'A', 18};
const VPin VLED_CAG7 = {'A', 19};
const VPin VLED_CAG8 = {'A', 20};
const VPin VLED_CAG9 = {'A', 21};
const VPin VLED_CAG10 = {'A', 22};

// SAS Modes
const VPin VLED_SAS_STABILITY_ASSIST = {'A', 23};
const VPin VLED_SAS_MANEUVER = {'A', 24};
const VPin VLED_SAS_PROGRADE = {'A', 25};
const VPin VLED_SAS_RETROGRADE = {'A', 26};
const VPin VLED_SAS_NORMAL = {'A', 27};
const VPin VLED_SAS_ANTI_NORMAL = {'A', 28};
const VPin VLED_SAS_RADIAL_IN = {'A', 29};
const VPin VLED_SAS_RADIAL_OUT = {'A', 30};
const VPin VLED_SAS_TARGET = {'A', 31};
const VPin VLED_SAS_ANTI_TARGET = {'A', 32};

// Bar Graph Base Pins (each bar has 20 consecutive LEDs)
const VPin VLED_SOLID_FUEL_BASE = {'A', 33};        // A33-A52
const VPin VLED_LIQUID_FUEL_BASE = {'A', 53};       // A53-B8 (wraps to next register)
const VPin VLED_OXIDIZER_BASE = {'B', 9};           // B9-B28
const VPin VLED_MONOPROPELLANT_BASE = {'B', 29};    // B29-B48
const VPin VLED_ELECTRICITY_BASE = {'B', 49};       // B49-C4 (wraps to next register)


#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h" 
#else
    #include "WProgram.h"
#endif

class OutputClass
{
protected:


public:

	void init();
	void update();

	void setLED(VPin vPin, bool state);
	// Displays
	void setSpeedLCD(String top, String bot);
	void setAltitudeLCD(String top, String bot);
	void setHeadingLCD(String top, String bot);
	void setDirectionLCD(String top, String bot);
	void setInfoLCD(String top, String bot);
};

extern OutputClass Output;

#endif
