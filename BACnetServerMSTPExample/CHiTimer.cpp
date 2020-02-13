/*******************************************************************************
                 CHiTimer.cpp
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
 
#include "CHiTimer.h"

CHiTimer::CHiTimer()
{
	QueryPerformanceFrequency(&this->frequency);
    Reset(); 
}
void  CHiTimer::Reset(void)
{
	QueryPerformanceCounter(&start);
	QueryPerformanceCounter(&stop);
}

// returns the time between the last two frams calls 
uint32_t CHiTimer::DiffTimeMicroSeconds()
{
	QueryPerformanceCounter(&stop);
	/*
	 * QueryPerformanceFrequency() gives you the number of counter ticks per second. 
	 * QueryPerformanceCounter() tells you what the hi-res tick count is. So, subtract 
	 * tSart from tStop, giving the number of ticks over the period to measure. Multiply 
	 * this result by the number of microseconds in a second (1000000) and finally divide 
	 * the value by the resolution of the timer. This gives the final answer in microseconds. 
	 * If you require an answer in milliseconds then just divide again by 1000.
	 */
	return (uint32_t) ( (this->stop.QuadPart - this->start.QuadPart) * 1000000 / this->frequency.QuadPart ) ;
}