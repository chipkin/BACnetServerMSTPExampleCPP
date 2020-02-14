#include "CASBACnetStackExampleDatabase.h"

#include <time.h> // time()
#ifdef _WIN32 
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#endif // _WIN32 

ExampleDatabase::ExampleDatabase() {
	this->Setup();
}

ExampleDatabase::~ExampleDatabase() {
	this->Setup();
}

void ExampleDatabase::Setup() {

	this->device.instance = 389999;
	this->device.objectName = "Device Rainbow";
	this->device.UTCOffset = 0;
	this->device.currentTimeOffset = 0;
	this->device.description = "Chipkin test BACnet IP Server device";
	// BACnetDeviceStatus ::= ENUMERATED { operational (0), operational-read-only (1), download-required (2), 
	// download-in-progress (3), non-operational (4), backup-in-progress (5) } 
	this->device.systemStatus = 0; // operational (0), non-operational (4)

	// MSTP 
	this->device.serialPort = 0; 
	this->device.baudRate = 9600; 
	this->device.macAddress = 0;
	this->device.maxInfoFrames = 1; // Default=1;
	this->device.maxMaster = 20; // Default=127;

	this->LoadNetworkPortProperties() ; 
}

void ExampleDatabase::LoadNetworkPortProperties() {

	// This function loads the Network port property values needed.
	// It uses system functions to get values like the IP Address and stores them
	// in the example database

	// ToDo: 
}


void ExampleDatabase::Loop() {
}


