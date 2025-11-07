/*
 Name:		Kerbal_Controller_Arduino rev3.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Output.h
#include <pins_arduino.h> 
#include <variant.h> 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>


#ifndef _OUTPUT_h
#define _OUTPUT_h

#define TOTAL_LEDS 145

// LED Virtual Pin Definitions
#define POWER_LED 0

// Warnings
#define TEMP_WARNING_LED  103
#define GEE_WARNING_LED   107
#define WARP_WARNING_LED  105
#define BRAKE_WARNING_LED 110
#define SAS_WARNING_LED   109
#define RCS_WARNING_LED   104
#define GEAR_WARNING_LED  102
#define COMMS_WARNING_LED 108
#define ALT_WARNING_LED   106
#define PITCH_WARNING_LED 101
// Staging
#define STAGE_LED 111
// Aborting
#define ABORT_LED 112
// Custom Action Groups
#define CAG1_LED   121
#define CAG2_LED   124
#define CAG3_LED   122
#define CAG4_LED   116
#define CAG5_LED   120
#define CAG6_LED   117
#define CAG7_LED   118
#define CAG8_LED   119
#define CAG9_LED   123
#define CAG10_LED  115
// SAS Modes
#define SAS_STABILITY_ASSIST_LED   129
#define SAS_MANEUVER_LED           114
#define SAS_PROGRADE_LED           131
#define SAS_RETROGRADE_LED         126
#define SAS_NORMAL_LED             132
#define SAS_ANTI_NORMAL_LED        127
#define SAS_RADIAL_IN_LED          134
#define SAS_RADIAL_OUT_LED         135
#define SAS_TARGET_LED             133
#define SAS_ANTI_TARGET_LED        96

#define SOLID_FUEL_LED_1   1
#define SOLID_FUEL_LED_2   2
#define SOLID_FUEL_LED_3   3
#define SOLID_FUEL_LED_4   4
#define SOLID_FUEL_LED_5   16
#define SOLID_FUEL_LED_6   15
#define SOLID_FUEL_LED_7   20
#define SOLID_FUEL_LED_8   19
#define SOLID_FUEL_LED_9   17
#define SOLID_FUEL_LED_10  18
#define SOLID_FUEL_LED_11  13
#define SOLID_FUEL_LED_12  14
#define SOLID_FUEL_LED_13  10
#define SOLID_FUEL_LED_14  9
#define SOLID_FUEL_LED_15  7
#define SOLID_FUEL_LED_16  5
#define SOLID_FUEL_LED_17  12
#define SOLID_FUEL_LED_18  11
#define SOLID_FUEL_LED_19  6
#define SOLID_FUEL_LED_20  8

#define LIQUID_FUEL_LED_1   125
#define LIQUID_FUEL_LED_2   22
#define LIQUID_FUEL_LED_3   24
#define LIQUID_FUEL_LED_4   23
#define LIQUID_FUEL_LED_5   28
#define LIQUID_FUEL_LED_6   29
#define LIQUID_FUEL_LED_7   26
#define LIQUID_FUEL_LED_8   27
#define LIQUID_FUEL_LED_9   25
#define LIQUID_FUEL_LED_10  30
#define LIQUID_FUEL_LED_11  32
#define LIQUID_FUEL_LED_12  31
#define LIQUID_FUEL_LED_13  36
#define LIQUID_FUEL_LED_14  35
#define LIQUID_FUEL_LED_15  37
#define LIQUID_FUEL_LED_16  38
#define LIQUID_FUEL_LED_17  39
#define LIQUID_FUEL_LED_18  40
#define LIQUID_FUEL_LED_19  33
#define LIQUID_FUEL_LED_20  34

#define OXIDIZER_LED_1   145
#define OXIDIZER_LED_2   143
#define OXIDIZER_LED_3   144
#define OXIDIZER_LED_4   142
#define OXIDIZER_LED_5   45
#define OXIDIZER_LED_6   46
#define OXIDIZER_LED_7   41
#define OXIDIZER_LED_8   42
#define OXIDIZER_LED_9   48
#define OXIDIZER_LED_10  47
#define OXIDIZER_LED_11  43
#define OXIDIZER_LED_12  44
#define OXIDIZER_LED_13  54
#define OXIDIZER_LED_14  53
#define OXIDIZER_LED_15  51
#define OXIDIZER_LED_16  52
#define OXIDIZER_LED_17  49
#define OXIDIZER_LED_18  50
#define OXIDIZER_LED_19  136
#define OXIDIZER_LED_20  55

#define MONOPROPELLANT_LED_1   94
#define MONOPROPELLANT_LED_2   95
#define MONOPROPELLANT_LED_3   93
#define MONOPROPELLANT_LED_4   92
#define MONOPROPELLANT_LED_5   91
#define MONOPROPELLANT_LED_6   90
#define MONOPROPELLANT_LED_7   89
#define MONOPROPELLANT_LED_8   88
#define MONOPROPELLANT_LED_9   86
#define MONOPROPELLANT_LED_10  87
#define MONOPROPELLANT_LED_11  85
#define MONOPROPELLANT_LED_12  84
#define MONOPROPELLANT_LED_13  80
#define MONOPROPELLANT_LED_14  81
#define MONOPROPELLANT_LED_15  82
#define MONOPROPELLANT_LED_16  83
#define MONOPROPELLANT_LED_17  100
#define MONOPROPELLANT_LED_18  97
#define MONOPROPELLANT_LED_19  99
#define MONOPROPELLANT_LED_20  98

#define ELECTRICITY_LED_1   140
#define ELECTRICITY_LED_2   141
#define ELECTRICITY_LED_3   138
#define ELECTRICITY_LED_4   139
#define ELECTRICITY_LED_5   78
#define ELECTRICITY_LED_6   79
#define ELECTRICITY_LED_7   77
#define ELECTRICITY_LED_8   73
#define ELECTRICITY_LED_9   72
#define ELECTRICITY_LED_10  76
#define ELECTRICITY_LED_11  74
#define ELECTRICITY_LED_12  75
#define ELECTRICITY_LED_13  68
#define ELECTRICITY_LED_14  67
#define ELECTRICITY_LED_15  64
#define ELECTRICITY_LED_16  71
#define ELECTRICITY_LED_17  69
#define ELECTRICITY_LED_18  70
#define ELECTRICITY_LED_19  66
#define ELECTRICITY_LED_20  65


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

	void setLED(int pin, bool state);
	// Displays
	void setSpeedLCD(String top, String bot);
	void setAltitudeLCD(String top, String bot);
	void setHeadingLCD(String top, String bot);
	void setDirectionLCD(String top, String bot);
	void setInfoLCD(String top, String bot);
	// Sound
	void setSound(int frequency, bool enabled);
};

extern OutputClass Output;

#endif
