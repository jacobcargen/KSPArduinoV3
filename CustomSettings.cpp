// 
// 
// 

#include "CustomSettings.h"

typedef std::map<String, int> Dictionary;

#pragma region Private

Dictionary pins;

void readFile()
{
	FILE * file = fopen("CustomSettings.dat", "r");
	
	for (int i = 0; i < TOTAL_ANALOG; i++)
	{

	}

	fscanf(file, "");
	String key;
	String op; 
	int pin;
	fscanf(file, "%s%s%d", &key, &op, &pin);
	if (strcmp(op.c_str(), "=") == 0)
	{
		pins.find(key)   pin;
	}

	// Close the file
	fclose(file);
}

void assignPins()
{

}

#pragma endregion


#pragma region Public

void CustomSettingsClass::init()
{
	// Read the settings file
	readFile();
	// Assign virtual pins to real ones here
	assignPins();

}

int getPin()
{
	
}
uint8_t getPinAnalog();
int getShiftPin();

#pragma endregion

CustomSettingsClass CustomSettings;
