#include "led.h"

idata unsigned char temp0=0x00;
idata unsigned char temp0_old=0xff;

void Led_Disp(unsigned char *ucLed){
    unsigned char temp;
    temp0 = 0x00;
    temp0 = ucLed[0] << 0 | ucLed[1] << 1 | ucLed[2] << 2 | ucLed[3] << 3 | ucLed[4] << 4 | ucLed[5] << 5 | ucLed[6] << 6 | ucLed[7] << 7;
    if(temp0 != temp0_old){
        P0 = ~temp0;

        temp = P2 & 0x1f;
        temp = temp | 0x80;
        P2 = temp;
        temp = P2 & 0x1f;
        P2 = temp;

        temp0_old=temp0;
    }
}

void Led_Off(){
    unsigned char temp;
    P0 = 0xff;

    temp = P2 & 0x1f;
    temp = temp | 0x80;
    P2 = temp;
    temp = P2 & 0x1f;
    P2 = temp;

    temp0_old=0x00;
}

idata unsigned char temp1=0x00;
idata unsigned char temp1_old=0xff;

void Beep(bit enable){
    unsigned char temp;
    if(enable) temp1 &= 0x40;
    else temp1 |= ~(0x40);

    if(temp1 != temp1_old){
        P0 = temp1;

        temp = P2 & 0x1f;
        temp = temp | 0xa0;
        P2 = temp;
        temp = P2 & 0x1f;
        P2 = temp;

        temp1_old=temp1;
    }
}

void Relay(bit enable){
    unsigned char temp;
    if(enable) temp1 &= 0x10;
    else temp1 |= ~(0x10);

    if(temp1 != temp1_old){
        P0 = temp1;

        temp = P2 & 0x1f;
        temp = temp | 0xa0;
        P2 = temp;
        temp = P2 & 0x1f;
        P2 = temp;

        temp1_old=temp1;
    }
}

void Motor(bit enable){
    unsigned char temp;
    if(enable) temp1 &= 0x20;
    else temp1 |= ~(0x20);

    if(temp1 != temp1_old){
        P0 = temp1;

        temp = P2 & 0x1f;
        temp = temp | 0xa0;
        P2 = temp;
        temp = P2 & 0x1f;
        P2 = temp;

        temp1_old=temp1;
    }
}