/*******************************************************************************
                 CHiTimer.cpp
********************************************************************************

   Written by:   Steven Smethust 
   Email:        funvill@funvill.com 
   
   No warranty of any kind, express or implied, is included with this
   software; use at your own risk
   
   If you find this code useful, credits would be nice!
   Feel free to distribute it, and email suggested changes to me.
   Free for commercial and non-commercial use
   
********************************************************************************

 Versions
 1.00aA  25 Nov 02 SWS  Created
 1.00aB  11 Ded 06 SWS  Added License 

********************************************************************************/
 
#include "CHiTimer.h"
#include <chrono> 

std::chrono::high_resolution_clock::time_point CHiTimer_start;
std::chrono::high_resolution_clock::time_point CHiTimer_stop;

void CHiTimer_Init() {
    CHiTimer_Reset(); 
}
void CHiTimer_Reset() {
    CHiTimer_start = std::chrono::high_resolution_clock::now();
}
uint32_t CHiTimer_DiffTimeMicroSeconds() {
    CHiTimer_stop = std::chrono::high_resolution_clock::now();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds> (CHiTimer_stop - CHiTimer_start);
    

    // Return a minimum of 10 micro seconds even if the count is zero. 
    uint32_t ret = (uint32_t)microseconds.count();
    return ret < 10 ? 10 : ret;
}