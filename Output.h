/*
 Name:		Kerbal_Controller_Arduino rev2.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// Output.h

#ifndef _OUTPUT_h
#define _OUTPUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
	#include <pins_arduino.h> 
	#include <variant.h> 
	#include <variant.cpp> 
	#include <LiquidCrystal_I2C.h>
#else
	#include "WProgram.h"
#endif

class OutputClass
{
protected:


public:

	OutputClass();
	void init();
	void update();


	// Misc

	void setPowerLED(bool state);
	void setSpeaker(bool state);

	// Warnings

	void setTempWarningLED(bool state);
	void setGeeWarningLED(bool state);
	void setWarpWarningLED(bool state);
	void setBrakeWarningLED(bool state);
	void setSASWarningLED(bool state);
	void setRCSWarningLED(bool state);
	void setGearWarningLED(bool state);
	void setCommsWarningLED(bool state);
	void setAltWarningLED(bool state);
	void setPitchWarningLED(bool state);

	// Displays

	void setSpeedLCD(char top[16], char bot[16]);
	void setAltitudeLCD(char top[16], char bot[16]);
	void setHeadingLCD(char top[16], char bot[16]);
	void setDirectionLCD(char top[16], char bot[16]);
	void setInfoLCD(char top[16], char bot[16]);

	// Resources

	void setSolidFuelLEDs(bool states[20]);
	void setLiquidFuelLEDs(bool states[20]);
	void setOxidizerLEDs(bool states[20]);
	void setMonopropellantLEDs(bool states[20]);
	void setElectricityLEDs(bool states[20]);

	// Staging

	void setStageLED(bool state);

	// Aborting

	void setAbortLED(bool state);

	// Custom Action Groups

	void setCAG1LED(bool state);
	void setCAG2LED(bool state);
	void setCAG3LED(bool state);
	void setCAG4LED(bool state);
	void setCAG5LED(bool state);
	void setCAG6LED(bool state);
	void setCAG7LED(bool state);
	void setCAG8LED(bool state);
	void setCAG9LED(bool state);
	void setCAG10LED(bool state);

	// SAS

	void setSASStabilityAssistLED(bool state);
	void setSASManeuverLED(bool state);
	void setSASProgradeLED(bool state);
	void setSASRetrogradeLED(bool state);
	void setSASNormalLED(bool state);
	void setSASAntiNormalLED(bool state);
	void setSASRadialInLED(bool state);
	void setSASRadialOutLED(bool state);
	void setSASTargetLED(bool state);
	void setSASAntiTargetLED(bool state);

};

extern OutputClass Output;

#endif
