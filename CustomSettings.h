// CustomSettings.h

#ifndef _CUSTOMSETTINGS_h
#define _CUSTOMSETTINGS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"

#else
	#include "WProgram.h"
#endif

class CustomSettingsClass
{
 protected:

 public:
	void init();

	int getPin();
	uint8_t getPinAnalog();
	int getShiftPin();

};

extern CustomSettingsClass CustomSettings;

#endif

