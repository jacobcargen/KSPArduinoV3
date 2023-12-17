/*
 Name:		Kerbal_Controller_Arduino rev2.0
 Created:	4/19/2023 4:14:14 PM
 Author:	Jacob Cargen
 Copyright: Jacob Cargen
*/

// KSPMessenger.h

#ifndef _KSPMESSENGER_h
#define _KSPMESSENGER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
	#include <PayloadStructs.h>
	#include <KerbalSimpitMessageTypes.h>
	#include <KerbalSimpit.h>
#else
	#include "WProgram.h"
#endif

class KSPMessengerClass
{
 protected:


 public:
	
	KSPMessengerClass::KSPMessengerClass();
	void init();
	void update();


	// Misc
	// Warnings
	// Display Controls
	

	// Staging & Aborting & Other Actions
	void sendAction(ActionGroupIndexes action, bool state);
	// Custom Action Groups
	void sendActionCustom(byte action, bool state);
	// Warping & Pause
	void sendSetWarp(Timewarp mode);
	void sendPause();
	// SAS & RCS
	void sendSASMode(AutopilotMode mode);
	byte getSASState();
	// EVA Specific Controls
	// Throttle
	void sendThrottle(int axis, bool state);
	// Translation
	void sendTranslation(int x, int y, int z);
	// Rotation
	void sendRotation(int, int, int);

};

extern KSPMessengerClass KSPMessenger;

#endif

