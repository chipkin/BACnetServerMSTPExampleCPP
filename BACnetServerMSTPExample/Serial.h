// Serial.h
// http://www.codeguru.com/Cpp/I-N/network/serialcommunications/article.php/c2503

#ifndef __SERIAL_H__
#define __SERIAL_H__
#pragma once


#include <stdio.h>
#include <stdarg.h>
#include <windows.h>


#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
#define FC_XONXOFF      0x04
#define ASCII_BEL       0x07
#define ASCII_BS        0x08
#define ASCII_LF        0x0A
#define ASCII_CR        0x0D
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

typedef void *HANDLE;

class CSerial
{

public:
	CSerial();
	~CSerial();

	bool Open(	int		nPort		= 2, 
				int		nBaud		= 9600,
				char	parity		= 'N', 
				unsigned int databits	= 8, 
				unsigned int stopsbits	= 1 );
	bool Close( void );

	int ReadData( void *, int );
	int SendData( const char *, int );
	int ReadDataWaiting( void );

	bool IsOpened( void ){ return( m_bOpened ); }

protected:
	bool WriteCommByte( unsigned char );
	bool WriteMultipleCommByte( unsigned char * ucByte, int iSize  );

	HANDLE m_hIDComDev;
	OVERLAPPED m_OverlappedRead, m_OverlappedWrite;
	bool m_bOpened;

};

#endif
