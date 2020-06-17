/*
 * BACnet Server MSTP Example C++
 * ----------------------------------------------------------------------------
 * BACnetServerMSTPExample.cpp
 * 
 * In this CAS BACnet Stack example, we create a minimal BACnet MSTP server.
 *
 * More information https://github.com/chipkin/BACnetServerMSTPExampleCPP
 * 
 * This file contains the 'main' function. Program execution begins and ends there.
 * 
 * Created by: Steven Smethurst
*/


#include "CASBACnetStackAdapter.h"
#include "CASBACnetStackExampleConstants.h"
#include "CASBACnetStackExampleDatabase.h"
#include "CIBuildVersion.h"

// MSTP
#include "MSTP.h"
#include "CHiTimer.h"
#include "rs232.h"

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

#ifndef __GNUC__ // Windows
#include <conio.h> // _kbhit
#else // Linux 
#include <sys/ioctl.h>
#include <termios.h>
bool _kbhit() {
	termios term;
	tcgetattr(0, &term);
	termios term2 = term;
	term2.c_lflag &= ~ICANON;
	tcsetattr(0, TCSANOW, &term2);
	int byteswaiting;
	ioctl(0, FIONREAD, &byteswaiting);
	tcsetattr(0, TCSANOW, &term);
	return byteswaiting > 0;
	}
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
void Sleep(int milliseconds) {
	usleep(milliseconds * 1000);
}
#endif // __GNUC__

// Globals
// =======================================
ExampleDatabase g_database; // The example database that stores current values.

// Constants
// =======================================
const std::string APPLICATION_VERSION = "0.0.1";  // See CHANGELOG.md for a full list of changes.
const uint32_t MAX_XML_RENDER_BUFFER_LENGTH = 1024 * 20;

// MSTP callbacks 
bool RecvByte(unsigned char* byte);
bool SendByte(unsigned char* byte, uint16_t length);
void ThreadSleep(uint32_t length);
void TimerReset();
uint32_t TimerDifference();
void APDUCallBack(unsigned char* buffer, uint16_t length, unsigned char source);
void MSTPFrameCallBack(uint8_t* buffer, uint16_t length); 
void MSTPDebugLog(const char* message);

// Helper functions
void DebugMSTPFrame(uint8_t* buffer, uint16_t length);

std::map<std::string, uint32_t> g_statCounter;
void printMSTPStats(); 

// MSTP Frame
struct MSTPAPDUFrame : public MSTPFrame
{
	MSTPAPDUFrame(uint8_t destination, uint8_t* buffer, uint16_t length) {
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
bool CallbackGetPropertyReal(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, float* value, const bool useArrayIndex, const uint32_t propertyArrayIndex);

// Set Property Functions
bool CallbackSetPropertyReal(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, const float value, const bool useArrayIndex, const uint32_t propertyArrayIndex, const uint8_t priority, unsigned int* errorCode);

int main(int argc, char* argv[])
{
	// Print the application version information 
	std::cout << "CAS BACnet Stack Server MSTP Example v" << APPLICATION_VERSION << "." << CIBUILDNUMBER << std::endl; 
	std::cout << "https://github.com/chipkin/BACnetServerMSTPExampleCPP" << std::endl;
	std::cout << "MSTP [serial port] [baud rate] [mac address]" << std::endl << std::endl;

	// Check the command line arguments 
	if (argc >= 2) {
		int32_t port = RS232_GetPortnr(argv[1]);
		if (port < 0) {
			std::cerr << "Could not detect port number from string. port=[" << argv[2] << "]... Accepted string COM4, ttyS4, etc... " << std::endl;
			return 0; 
		}
		g_database.device.serialPort = (uint8_t)port; 
		std::cout << "FYI: Using comport=[" << argv[1] << "], Portnr=[" << (int) g_database.device.serialPort << "]" << std::endl ;
	}
	if (argc >= 3) {
		g_database.device.baudRate = atoi(argv[2]);
	}
	if (argc >= 4) {
		g_database.device.macAddress = (uint8_t)atoi(argv[3]);
		std::cout << "FYI: Using macAddress=[" << (int) g_database.device.macAddress << "]" << std::endl;
	}


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
	std::cout << "FYI: Connecting serial resource to serial port. baudrate=[" << g_database.device.baudRate << "], ... ";

	if (RS232_OpenComport(g_database.device.serialPort, g_database.device.baudRate, "8N1", 0) == 1) {
		std::cerr << "Error: Failed to open serial port" << std::endl;
		return 0;
	}
	std::cout << "OK" << std::endl;

	// Configure the highspeed timer
	CHiTimer_Init();
	
	// Configure the MSTP thread 
	if (!MSTP_Init(RecvByte, SendByte, ThreadSleep, APDUCallBack, TimerReset, TimerDifference, g_database.device.macAddress, MSTPFrameCallBack, MSTPDebugLog)) {
		std::cerr << "Error: Failed to start MSTP stack" << std::endl;
		return 0;
	}
	
	// Set the baud rate
	MSTP_SetBaudRate(g_database.device.baudRate);
	std::cout << "OK, Connected to port" << std::endl;

	// Set the MSTP stack's max master 
	MSTP_SetMaxMaster(g_database.device.maxMaster);


	// 3. Setup the callbacks
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
	fpRegisterCallbackGetPropertyReal(CallbackGetPropertyReal);

	// Set Property Callback Functions
	fpRegisterCallbackSetPropertyReal(CallbackSetPropertyReal);


	// 4. Setup the BACnet device 
	// ---------------------------------------------------------------------------
	std::cout << "Setting up server device. device.instance=[" << g_database.device.instance << "]" << std::endl; 

	// Create the Device
	if (!fpAddDevice(g_database.device.instance)) {
		std::cerr << "Failed to add Device." << std::endl;
		return false;
	}

	// Add AnalogValue object to device
	if (!fpAddObject(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_ANALOG_VALUE, g_database.analogValue.instance)) {
		std::cerr << "Failed to add Analog value(2)." << std::endl;
		return false;
	}
	std::cout << "Created Device." << std::endl;

	// Enable services 
	// Enable write property
	std::cout << "Enabling WriteProperty service... ";
	if (!fpSetServiceEnabled(g_database.device.instance, CASBACnetStackExampleConstants::SERVICE_WRITE_PROPERTY, true)) {
		std::cerr << "Failed to enabled the WriteProperty" << std::endl;
		return false;
	}
	std::cout << "OK" << std::endl;

	// Enable properties 
	std::cout << "Enabling device optional properties. Max_info_frames, max_master... ";
	fpSetPropertyEnabled(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE, g_database.device.instance, CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_INFO_FRAMES, true);
	fpSetPropertyEnabled(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE, g_database.device.instance, CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_MASTER, true);
	fpSetPropertyWritable(g_database.device.instance, CASBACnetStackExampleConstants::OBJECT_TYPE_ANALOG_VALUE, g_database.analogValue.instance, CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_PRESENT_VALUE, true);
	std::cout << "OK" << std::endl;


	// 4. Send I-Am of this device
	// ---------------------------------------------------------------------------
	// To be a good citizen on a BACnet network. We should annouce ourselfs when we start up. 
	std::cout << "FYI: Sending I-AM broadcast" << std::endl;
	uint8_t connectionString[1] = { 0xFF };
	if (!fpSendIAm(g_database.device.instance, connectionString, 1, CASBACnetStackExampleConstants::NETWORK_TYPE_MSTP, true, 65535, NULL, 0)) {
		std::cerr << "Unable to send I-Am broadcast" << std::endl; 
		return false;
	}

	// Clear the RX TX Buffer
	RS232_flushRXTX(g_database.device.serialPort);


	// 5. Start the main loop
	// ---------------------------------------------------------------------------
	std::cout << "FYI: Entering main loop..." << std::endl;
	for (;;) {
		MSTP_Loop();

		// Call the DLLs loop function which checks for messages and processes them.
		fpLoop();		

		// Update values in the example database
		g_database.Loop();

		// Check to see if the user hit any key
		if (_kbhit()) {
			printMSTPStats();
			getchar(); // Remove the key from the buffer. 
		}
	}
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
		// Call Sleep to give some time back to the system
		Sleep(1);
		return 0; // Nothing received
	}

	for (std::list<MSTPAPDUFrame>::iterator itr = g_incomingFrame.begin(); itr != g_incomingFrame.end(); itr++) {
		if ((*itr).length > maxMessageLength) {
			// This is too big to fit in the message buffer. It probably never will be able to be used, so get rid of it.
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
	
	// Everything looks good. 
	return messageLength;
}

// Callback used by the BACnet Stack to get the current time
time_t CallbackGetSystemTime()
{
	return time(0) - g_database.device.currentTimeOffset;
}

// Callback used by the BACnet Stack to get Character String property values from the user
bool CallbackGetPropertyCharString(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, char* value, uint32_t* valueElementCount, const uint32_t maxElementCount, uint8_t* /*encodingType*/, const bool /*useArrayIndex*/, const uint32_t /*propertyArrayIndex*/)
{
	if (deviceInstance != g_database.device.instance) {
		return false;
	}

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
		else if (objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_ANALOG_VALUE && objectInstance == g_database.analogValue.instance) {
			stringSize = g_database.analogValue.objectName.size();
			if (stringSize > maxElementCount) {
				std::cerr << "Error: Not enough space to store full name of objectType=[" << objectType << "], objectInstance=[" << objectInstance << " ]" << std::endl;
				return false;
			}
			memcpy(value, g_database.analogValue.objectName.c_str(), stringSize);
			*valueElementCount = (uint32_t)stringSize;
			return true;
		}
	}
	return false;
}


// Callback used by the BACnet Stack to get Unsigned Integer property values from the user
bool CallbackGetPropertyUInt(uint32_t deviceInstance, uint16_t objectType, uint32_t objectInstance, uint32_t propertyIdentifier, uint32_t* value, bool /*useArrayIndex*/, uint32_t /*propertyArrayIndex*/)
{
	if (deviceInstance != g_database.device.instance) {
		return false;
	}

	if (objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_DEVICE && objectInstance == g_database.device.instance) {
		if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_INFO_FRAMES) {
			*value = g_database.device.maxInfoFrames;
			return true;
		}
		else if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_MAX_MASTER) {
			*value = g_database.device.maxMaster;
			return true;
		}
	}
	return false; 
}

bool CallbackGetPropertyReal(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, float* value, const bool , const uint32_t )
{
	if (deviceInstance != g_database.device.instance) {
		return false;
	}

	if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_PRESENT_VALUE) {
		if (objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_ANALOG_VALUE && objectInstance == g_database.analogValue.instance) {
			*value = g_database.analogValue.presentValue;
			return true;
		}
	}
	return false;
}

bool CallbackSetPropertyReal(const uint32_t deviceInstance, const uint16_t objectType, const uint32_t objectInstance, const uint32_t propertyIdentifier, const float value, const bool , const uint32_t , const uint8_t , unsigned int* ) {
	if (deviceInstance != g_database.device.instance) {
		return false; // Not this device.
	}
	if (propertyIdentifier == CASBACnetStackExampleConstants::PROPERTY_IDENTIFIER_PRESENT_VALUE) {
		// Example of setting Analog Value Present Value Property
		if (objectType == CASBACnetStackExampleConstants::OBJECT_TYPE_ANALOG_VALUE && objectInstance == g_database.analogValue.instance) {
			g_database.analogValue.presentValue = value;
			return true;
		}
	}
	return false; 

}

// ----------------------------------------------------------------------------
void printMSTPStats() {
	const int PARAMETER_NAME_WIDTH = 25;
	const int PARAMETER_VALUE_WIDTH = 5;

	std::cout << std::endl;
	std::cout << std::endl;

#ifdef MSTP_DEBUG
	std::cout << "MSTP Stats:" << std::endl; 
	std::cout << "-----------" << std::endl;

	// Print stats. 
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentBytes: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentBytes;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recvBytes: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.recvBytes;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "eatAnOctet: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.eatAnOctet;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentFrames: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentFrames;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recvFrames: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.recvFrames;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentPollForMaster: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentFramePollForMaster;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentAPDU: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentAPDU;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recvAPDU: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.recvAPDU;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentReplyPollForMaster: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentFrameReplyPollForMaster;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentLastType: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentLastFrameType;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recvLastType: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.recvLastFrameType;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "sentToken: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.sentFrameToken;
	std::cout << std::endl;
	
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << " " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << " ";
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recvInvalid: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.recvInvalidFrame;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "hasToken: " << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_stats.hasToken;
	std::cout << std::endl;	

#endif // MSTP_DEBUG
	
	// Print status 
	std::cout << "MSTP State:" << std::endl;
	std::cout << "-----------" << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "This station (TS):" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.TS;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "EventCount:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.EventCount;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "master_state:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.master_state;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "Next Station (NS):" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.NS;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "FrameCount:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.FrameCount;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "recv_state:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.recv_state;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "Poll Station (PS):" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.PS;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "TokenCount:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.TokenCount;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "SoleMaster:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.SoleMaster;
	std::cout << std::endl;

	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "Previous Station (AS):" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.AS;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "RetryCount:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (int)mstp_runningVariables.RetryCount;
	std::cout << std::setw(PARAMETER_NAME_WIDTH) << std::setfill(' ') << "SilenceTimerMS:" << std::dec << std::setw(PARAMETER_VALUE_WIDTH) << (uint32_t)mstp_runningVariables.SilenceTimerMicroSeconds;
	std::cout << std::endl;	
	std::cout << std::endl;

#ifdef MSTP_DEBUG
	std::cout << "MSTP Events:" << std::endl;
	std::cout << "------------" << std::endl;

	int count = 0; 
	for (std::map<std::string, uint32_t>::iterator itr = g_statCounter.begin(); itr != g_statCounter.end(); itr++) {
		count++; 
		std::cout << std::setw(PARAMETER_NAME_WIDTH-1) << std::setfill(' ')  << (*itr).first ;
		std::cout << ": ";
		std::cout << std::setw(PARAMETER_VALUE_WIDTH-1) << std::setfill(' ') << std::dec << (*itr).second; 

		if (count % 3 == 0) {
			std::cout << std::endl;
		}
	}
	std::cout << std::endl;
#endif // MSTP_DEBUG

}

// MSTP callbacks 
// ----------------------------------------------------------------------------

// Check the Serial buffer for incoming bytes. 
// Return 
//   True - Found a byte and stored the value in byte prameter. 
//   False - No bytes avaliable. 
bool RecvByte(unsigned char* byte) {
	if (RS232_PollComport(g_database.device.serialPort, byte, 1) <= 0) {
		// no bytes found 
		return false;
	}

#ifdef MSTP_DEBUG
	// Found a byte. 
	std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) *byte ;
	std::cout << "_";	
#endif // MSTP_DEBUG

	return true;
}

// Attempts to send one or more bytes out the serial port. 
// Return
//   True  - The bytes where succesfuly sent. 
//   False - Could not send the bytes. Try again later. 
bool SendByte(unsigned char* byte, uint16_t length) {

#ifdef MSTP_DEBUG
	// Debug print
	for (unsigned int offset = 0; offset < length; offset++) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) byte[offset];
		std::cout << " "; 
	}
	DebugMSTPFrame(byte, length);
	std::cout << std::endl; 	
#endif // MSTP_DEBUG

	if (RS232_SendBuf(g_database.device.serialPort, byte, length) != length) {
		// Bytes could not be sent 
		return false;
	}
	return true;
}

// The MSTP thread does not need attention for the next "length" amount of time. 
// This function can do nothing, and the MSTP thread will consume as much processing power 
// as you give it.
void ThreadSleep(uint32_t length) {
	if (length < 1) {
		return;
	}
	Sleep(length);
}

// Reset the high speed timer back to zero and start counting. 
void TimerReset() {
	CHiTimer_Reset();
}

// Find the difference in time between the last time that the TimerReset was called and now. 
uint32_t TimerDifference() {
	return CHiTimer_DiffTimeMicroSeconds();
}

// When ever the MSTP stack recives a APDU message it will pass it up to this function for processing 
// by the BACnet API stack. 
void APDUCallBack(unsigned char* buffer, uint16_t length, unsigned char source) {
	g_incomingFrame.push_back(MSTPAPDUFrame(source, buffer, length));
}

void MSTPDebugLog(const char* message) {
	#ifdef MSTP_DEBUG
		g_statCounter[std::string(message)]++;
		std::cout << "[" << message << "]" << std::endl; 
	#endif // MSTP_DEBUG
}

// Displays MSTP Frame Data
void DebugMSTPFrame(uint8_t* buffer, uint16_t length) {
	if (length < 3) {
		return;
	}

	// +2 is the preamble
	std::cout << "  {src=" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[2 + MSTP_HEADER_INDEX_SOURCE];
	std::cout << ", dest=" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[2 + MSTP_HEADER_INDEX_DESTINATION] << " ";
	
	switch (buffer[2 + MSTP_HEADER_INDEX_FRAMETYPE]) {
	case MSTP_FRAME_TYPE_TOKEN:
		std::cout << "TOKEN";
		break;
	case MSTP_FRAME_TYPE_POLL_FOR_MASTER:
		std::cout << "POLL_FOR_MASTER";
		break;
	case MSTP_FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER:
		std::cout << "REPLY_TO_POLL_FOR_MASTER";
		break;
	case MSTP_FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY:
		std::cout << "DATA_EXPECTING_REPLY";
		break;
	case MSTP_FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY:
		std::cout << "DATA_NOT_EXPECTING_REPLY";
		break;
	case MSTP_FRAME_TYPE_REPLY_POSTPONED:
		std::cout << "REPLY_POSTPONED";
		break;
	default:
		std::cout << "UNKNOWN=" << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[2];
		break;
	}

	std::cout << "}";
}

void MSTPFrameCallBack(uint8_t* buffer, uint16_t length) {

	#ifdef MSTP_DEBUG
		DebugMSTPFrame(buffer, length);
		std::cout << std::endl;
	#endif // MSTP_DEBUG
}

