/*******************************************************************************
                 CHiTimer.h
********************************************************************************

   Written by:   Steven Smethust 
   Email:        funvill@funvill.com 
   
   No warrantee of any kind, express or implied, is included with this
   software; use at your own risk
   
   If you find this code useful, credits would be nice!
   Feel free to distribute it, and email suggested changes to me.
   Free for commercial and non-commercial use
   
********************************************************************************

 Versions
 1.00aA  25 Nov 02 SWS  Created
 1.00aB  11 Ded 06 SWS  Added Licence 

********************************************************************************/
#ifndef __CHiTimer_h__
#define __CHiTimer_h__

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <stdint.h>


class CHiTimer
{
public:
	CHiTimer();
	void			Reset(void); // Starts the timer 
	uint32_t		DiffTimeMicroSeconds(void);	// Gets the time between star and stop. 

private:
	LARGE_INTEGER	start;
	LARGE_INTEGER	stop;
	LARGE_INTEGER	frequency;
};



#endif // __CHiTimer_h__