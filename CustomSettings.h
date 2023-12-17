// CustomSettings.h

#ifndef _CUSTOMSETTINGS_h
#define _CUSTOMSETTINGS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
	#include <map>
#else
	#include "WProgram.h"
#endif

class CustomSettingsClass
{
 protected:

 public:
	void init();

	// Read File
	void loadSettings();

	int getPin();
	uint8_t getPinAnalog();
	int getShiftPin();

};

extern CustomSettingsClass CustomSettings;

#endif

