#ifndef __DS1302_H__
#define __DS1302_H__

#include <STC15F2K60S2.H>

void Set_Rtc(unsigned char *ucRtc);
void Read_Rtc(unsigned char *ucRtc);

#endif