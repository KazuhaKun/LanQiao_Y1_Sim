#include "uart.h"

void Uart_Init(void) // 9600bps@12.000MHz
{
  SCON = 0x50;  
  AUXR |= 0x01; 
  AUXR &= 0xFB; 
  T2L = 0xE6;  
  T2H = 0xFF;  
  AUXR |= 0x10; 
  ES = 1;       
  EA = 1;
}

extern char putchar(char ch)
{
  SBUF = ch;
  while (TI == 0)
    ;
  TI = 0;
  return ch;
}
