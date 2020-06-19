/*
 * BACnet Server MSTP Example C++
 * ----------------------------------------------------------------------------
 * CASBACnetStackExampleDatabase.h
 * 
 * The CASBACnetStackExampleDatabase is a data storage that contains 
 * some example data used in the server example. This data is represented by BACnet 
 * objects for this server example. There will be one object of each type currently 
 * supported by the CASBACnetStack.
 * 
 * The database will include the following:
 *	- present value
 *	- name
 *	- for outputs priority array (bool and value)
 * 
 * Created by: Steven Smethurst
*/


#ifndef __CASBACnetStackExampleDatabase_h__
#define __CASBACnetStackExampleDatabase_h__

#include <string>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <map>

// Base class for all object types. 
class ExampleDatabaseBaseObject
{
	public:
		// Const
		static const uint8_t PRIORITY_ARRAY_LENGTH = 16 ; 

		// All objects will have the following properties 
		std::string objectName ; 
		uint32_t instance ; 
};

class ExampleDatabaseAnalogValue : public ExampleDatabaseBaseObject 
{
	public:
		float presentValue;		
};
class ExampleDatabaseDevice : public ExampleDatabaseBaseObject 
{
	public:
		int UTCOffset;
		int64_t currentTimeOffset;
		std::string description;
		uint32_t systemStatus;

		// MSTP 
		uint8_t serialPort;
		uint32_t baudRate;
		uint8_t macAddress;
		uint8_t maxInfoFrames;
		uint8_t maxMaster;
};

class ExampleDatabase {

	public:
		ExampleDatabaseAnalogValue analogValue;
		ExampleDatabaseDevice device;

	// Constructor / Deconstructor
	ExampleDatabase();
	~ExampleDatabase();

	// Set all the objects to have a default value. 
	void Setup();

	// Update the values as needed 
	void Loop(); 

	// Helper Functions	
	void LoadNetworkPortProperties();

};


#endif // __CASBACnetStackExampleDatabase_h__
