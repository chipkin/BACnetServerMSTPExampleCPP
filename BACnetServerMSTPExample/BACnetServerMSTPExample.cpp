// BACnetServerMSTPExample.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "CASBACnetStackAdapter.h"
#include "CASBACnetStackExampleConstants.h"
#include "CASBACnetStackExampleDatabase.h"
#include "CIBuildVersion.h"

// MSTP
#include "MSTP.h"
#include "CHiTimer.h"
#include "Serial.h"

// Helpers 
#include "ChipkinEndianness.h"
#include "ChipkinConvert.h"
#include "ChipkinUtilities.h"


#include <iomanip> // std::setw 
#include <list>
#include <iostream> // std::cout
#ifndef __GNUC__ // Windows
	#include <windows.h> // Sleep 
#endif 

// Globals
// =======================================
ExampleDatabase g_database; // The example database that stores current values.
CHiTimer g_timer; 
CSerial g_serial;

// Constants
// =======================================
const std::string APPLICATION_VERSION = "0.0.1";  // See CHANGELOG.md for a full list of changes.
const uint32_t MAX_XML_RENDER_BUFFER_LENGTH = 1024 * 20;

// MSTP callbacks 
bool RecvByte(unsigned char* byte);
bool SendByte(unsigned char* byte, UINT length);
void ThreadSleep(uint32_t length);
void TimerReset();
uint32_t TimerDifference();
void APDUCallBack(unsigned char* buffer, UINT length, unsigned char source);

struct MSTPAPDUFrame : public MSTPFrame
{
	MSTPAPDUFrame(uint8_t destination, uint8_t* buffer, uint32_t length) {
		if (length > MSTP_MAX_PACKET_OCTETS) {
			this->length = 0;
			return;
		}
		this->length = length;
		this->destination = destination;
		memcpy(this->buffer, buffer, length);
	}
};

std::list<MSTPAPDUFrame> g_incomingFrame;


// Callback Functions to Register to the DLL
// Message Functions
uint16_t CallbackReceiveMessage(uint8_t* message, const uint16_t maxMessageLength, uint8_t* receivedConnectionString, const uint8_t maxConnectionStringLength, uint8_t* receivedConnectionStringLength, uint8_t* networkType);
uint16_t CallbackSendMessage(const uint8_t* message, const uint16_t messageLength, const uint8_t* connectionString, const uint8_t connectionStringLength, const uint8_t networkType, bool broadcast);

// System Functions
time_t CallbackGetSystemTime();

// Get Property Functions
bool CallbackGetPropertyCharString(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, char* value, uint32_t* valueElementCount, const uint32_t maxElementCount, uint8_t* encodingType, const bool useArrayIndex, const uint32_t propertyArrayIndex);
bool CallbackGetPropertyUInt(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, uint32_t* value, const bool useArrayIndex, const uint32_t propertyArrayIndex);

int main()
{
	// Print the application version information 
	std::cout << "CAS BACnet Stack Server MSTP Example v" << APPLICATION_VERSION << "." << CIBUILDNUMBER << std::endl; 
	std::cout << "https://github.com/chipkin/BACnetServerMSTPExampleCPP" << std::endl << std::endl;

	// 1. Load the CAS BACnet stack functions
	// ---------------------------------------------------------------------------
	std::cout << "FYI: Loading CAS BACnet Stack functions... "; 
	if (!LoadBACnetFunctions()) {
		std::cerr << "Failed to load the functions from the DLL" << std::endl;
		return 0;
	}
	std::cout << "OK" << std::endl;
	std::cout << "FYI: CAS BACnet Stack version: " << fpGetAPIMajorVersion() << "." << fpGetAPIMinorVersion() << "." << fpGetAPIPatchVersion() << "." << fpGetAPIBuildVersion() << std::endl;

	// 2. Connect the serial resource
	// ---------------------------------------------------------------------------
	std::cout << "FYI: Connecting serial resource to comport=[" << g_database.device.serialPort << "], baudrate=[" << g_database.device.baudRate << "], ... ";
	if (!g_serial.Open(g_database.device.serialPort, g_database.device.baudRate, 'n', 8, 1)) {
		std::cerr << "Error: Failed to open serial port" << std::endl;
		return 0;
	}
	std::cout << "OK" << std::endl;

	// Configure the highspeed timer. 
	g_timer.Reset(); 

	// Configure the MSTP thread 
	if (!MSTP_Init(RecvByte, SendByte, ThreadSleep, APDUCallBack, TimerReset, TimerDifference, g_database.device.macAddress, NULL)) {
		std::cerr << "Error: Failed to start MSTP stack" << std::endl;
		return 0;
	}
	MSTP_SetBaudRate(g_database.device.baudRate);
	std::cout << "OK, Connected to port" << std::endl;

	// Set the MSTP stack's max master 
	MSTP_SetMaxMaster(g_database.device.maxMaster);

	// 3. Setup the callbacks. 
	// ---------------------------------------------------------------------------
	std::cout << "FYI: Registering the callback Functions with the CAS BACnet Stack" << std::endl;

	// Message Callback Functions
	fpRegisterCallbackReceiveMessage(CallbackReceiveMessage);
	fpRegisterCallbackSendMessage(CallbackSendMessage);

	// System Time Callback Functions
	fpRegisterCallbackGetSystemTime(CallbackGetSystemTime);

	// Get Property Callback Functions
	fpRegisterCallbackGetPropertyCharacterString(CallbackGetPropertyCharString);
	fpRegisterCallbackGetPropertyUnsignedInteger(CallbackGetPropertyUInt);


	// 4. Setup the BACnet device. 
	// ---------------------------------------------------------------------------
	std::cout << "Setting up server device. device.instance=[" << g_database.device.instance << "]" << std::endl; 

	// Create the Device
	if (!fpAddDevice(g_database.device.instance)) {
		std::cerr << "Failed to add Device." << std::endl;
		return false;
	}
	std::cout << "Created Device." << std::endl; 

	fpSetPropertyEnabled(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE, g_database.device.instance, CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_INFO_FRAMES, true);
	fpSetPropertyEnabled(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE, g_database.device.instance, CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_MASTER, true);


	// 4. Send I-Am of this device
	// ---------------------------------------------------------------------------
	// To be a good citizen on a BACnet network. We should annouce ourselfs when we start up. 
	std::cout << "FYI: Sending I-AM broadcast" << std::endl;
	uint8_t connectionString[1] = { 0xFF };
	if (!fpSendIAm(g_database.device.instance, connectionString, 1, CASBACnetStackExampleConstants::NETWORK_TYPE_MSTP, true, 65535, NULL, 0)) {
		std::cerr << "Unable to send IAm broadcast" << std::endl ; 
		return false;
	}

	// 5. Start the main loop
	// ---------------------------------------------------------------------------
	std::cout << "FYI: Entering main loop..." << std::endl ;
	for (;;) {
		MSTP_Loop();

		// Call the DLLs loop function which checks for messages and processes them.
		fpLoop();		

		// Update values in the example database
		g_database.Loop();

		// Call Sleep to give some time back to the system
		#ifdef _WIN32
			Sleep(0); // Windows 
		#else
			sleep(0); // Linux 
		#endif
	}

	// All done. 
	return 0;
}


// Callback used by the BACnet Stack to check if there is a message to process
uint16_t CallbackReceiveMessage(uint8_t* message, const uint16_t maxMessageLength, uint8_t* receivedConnectionString, const uint8_t maxConnectionStringLength, uint8_t* receivedConnectionStringLength, uint8_t* networkType)
{
	// Check parameters
	if (message == NULL || maxMessageLength == 0) {
		std::cerr << "Invalid input buffer" << std::endl;
		return 0;
	}
	if (receivedConnectionString == NULL || maxConnectionStringLength == 0) {
		std::cerr << "Invalid connection string buffer" << std::endl;
		return 0;
	}

	if (g_incomingFrame.size() <= 0) {
		return 0; // Nothing recived. 
	}

	for (std::list<MSTPAPDUFrame>::iterator itr = g_incomingFrame.begin(); itr != g_incomingFrame.end(); itr++) {
		if ((*itr).length > maxMessageLength) {
			// This is too big to fit in the message buffer. it probably never will be able to be used. lets get ride of it 
			g_incomingFrame.erase(itr);
			return 0;
		}

		memcpy(message, (*itr).buffer, (*itr).length);
		receivedConnectionString[0] = (*itr).destination;
		*receivedConnectionStringLength = 1;
		*networkType = CASBACnetStackExampleConstants::NETWORK_TYPE_MSTP; // 1 == MSTP 
		uint16_t bytesRead = (*itr).length;

		// Done with this frame. 
		g_incomingFrame.erase(itr);

		// Process the message as XML
		static char xmlRenderBuffer[MAX_XML_RENDER_BUFFER_LENGTH];
		if (fpDecodeAsXML((char*)message, bytesRead, xmlRenderBuffer, MAX_XML_RENDER_BUFFER_LENGTH) > 0) {
			std::cout << xmlRenderBuffer << std::endl;
			memset(xmlRenderBuffer, 0, MAX_XML_RENDER_BUFFER_LENGTH);
		}

		// Done 
		return bytesRead;
	}
	return 0;
}

// Callback used by the BACnet Stack to send a BACnet message
uint16_t CallbackSendMessage(const uint8_t* message, const uint16_t messageLength, const uint8_t* connectionString, const uint8_t connectionStringLength, const uint8_t networkType, bool broadcast)
{
	if (message == NULL || messageLength == 0) {
		std::cout << "Nothing to send" << std::endl;
		return 0;
	}
	if (connectionString == NULL || connectionStringLength == 0) {
		std::cout << "No connection string" << std::endl;
		return 0;
	}

	// Verify Network Type
	if (networkType != CASBACnetStackExampleConstants::NETWORK_TYPE_MSTP) {
		std::cout << "Message for different network" << std::endl;
		return 0;
	}

	if (!MSTP_InsertFrameToSend(message, broadcast ? 0xFF : connectionString[0], messageLength)) {
		std::cerr << "Error: Could not send message to MSTP stack" << std::endl;
		return 0; // Could not send message 
	}

	// Get the XML rendered version of the just sent message
	static char xmlRenderBuffer[MAX_XML_RENDER_BUFFER_LENGTH];
	if (fpDecodeAsXML((char*)message, messageLength, xmlRenderBuffer, MAX_XML_RENDER_BUFFER_LENGTH) > 0) {
		std::cout << xmlRenderBuffer << std::endl;
		memset(xmlRenderBuffer, 0, MAX_XML_RENDER_BUFFER_LENGTH);
	}

	// Everything looks good. 
	return messageLength;
}

// Callback used by the BACnet Stack to get the current time
time_t CallbackGetSystemTime()
{
	return time(0) - g_database.device.currentTimeOffset;
}

// Callback used by the BACnet Stack to get Character String property values from the user
bool CallbackGetPropertyCharString(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, char* value, uint32_t* valueElementCount, const uint32_t maxElementCount, uint8_t* encodingType, const bool useArrayIndex, const uint32_t propertyArrayIndex)
{
	// Example of Object Name property
	if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_OBJECT_NAME) {

		size_t stringSize = 0;
		if (objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE && objectInstance == g_database.device.instance) {
			stringSize = g_database.device.objectName.size();
			if (stringSize > maxElementCount) {
				std::cerr << "Error: Not enough space to store full name of objectType=[" << objectType << "], objectInstance=[" << objectInstance << " ]" << std::endl;
				return false;
			}
			memcpy(value, g_database.device.objectName.c_str(), stringSize);
			*valueElementCount = (uint32_t)stringSize;
			return true;
		}
	}
	return false;
}


// Callback used by the BACnet Stack to get Unsigned Integer property values from the user
bool CallbackGetPropertyUInt(uint32_t deviceInstance, uint16_t objectType, uint32_t objectInstance, uint32_t propertyIdentifier, uint32_t* value, bool useArrayIndex, uint32_t propertyArrayIndex)
{
	if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_INFO_FRAMES && objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE) {
		*value = g_database.device.maxInfoFrames; 
		return true; 
	} else if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_MASTER && objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE) {
		*value = g_database.device.maxMaster;
		return true;
	}
	return false; 
}

// MSTP callbacks 
// ----------------------------------------------------------------------------

// Check the Serial buffer for incoming bytes. 
// Return 
//   True - Found a byte and stored the value in byte prameter. 
//   False - No bytes avaliable. 
bool RecvByte(unsigned char* byte) {
	if (g_serial.ReadData(byte, 1) <= 0) {
		// no bytes found 
		return false;
	}

	// Found a byte. 
	// std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) *byte ;
	// std::cout << " ";
	std::cout << ".";
	return true;
}

// Attempts to send one or more bytes out the serial port. 
// Return
//   True  - The bytes where succesfuly sent. 
//   False - Could not send the bytes. Try again later. 
bool SendByte(unsigned char* byte, UINT length) {

	std::cout << "+";
	/*
	std::cout << std::endl << "TX: ";
	for (unsigned int offset = 0; offset < length; offset++) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) byte[offset];
		std::cout << " "; 
	}
	std::cout << std::endl; 
	*/

	if (g_serial.SendData( (char *)byte, length) != length) {
		// Bytes could not be sent 
		return false;
	}

	return true;
}


#ifdef __GNUC__
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
void Sleep(int milliseconds) {
	sleep(milliseconds);
}
#endif //

// The MSTP thread does not need attention for the next "length" amount of time. 
// This function can do nothing, and the MSTP thread will consumed as much proccessing power 
// that you will give it. 
void ThreadSleep(uint32_t length) {
	if (length < 1) {
		return;
	}
	Sleep((DWORD)length);
}

// Reset the high speed timer back to zero and start counting. 
void TimerReset() {
	g_timer.Reset();  
}

// Find the differen in time between the last time that the TimerReset was called and now. 
uint32_t TimerDifference() {
	return g_timer.DiffTimeMicroSeconds(); 
}

// When ever the MSTP stack recives a APDU message it will pass it up to this function for processing 
// by the BACnet API stack. 
void APDUCallBack(unsigned char* buffer, UINT length, unsigned char source) {
	g_incomingFrame.push_back(MSTPAPDUFrame(source, buffer, length));
}