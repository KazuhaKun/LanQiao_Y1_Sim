#include "init.h"

void InitSys(){
    P0 = 0xff;
    P2 = (P2 & 0x1f) | 0x80;
    P2 &= 0x1f;

    P0 = 0x00;
    P2 = (P2 & 0x1f) | 0xa0;
    P2 &= 0x1f;
}