#include "ultrawave.h"
#include "intrins.h"

sbit Tx = P1^0;
sbit Rx = P1^1;

void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}

void Ut_Wave_Init(){
    unsigned char i;
    EA = 0;
    for(i=0;i<8;i++){
        Tx=1;
        Delay12us();
        Tx=0;
        Delay12us();
    }
    EA = 1;
}

unsigned char Ut_Wave_Data(){
    unsigned char time;
    CMOD = 0x00;
    CH=CL=0;
    Ut_Wave_Init();
    CR=1;
    while((Rx==1)&(CF==0));
    CR=0;
    if(CF==0){
        time = CH << 8 | CL;
        return time*0.017;
    }
    else {
        CF=0;
        return 0;
    }
}