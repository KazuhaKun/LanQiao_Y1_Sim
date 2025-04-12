#include "key.h"
unsigned char Key_Read()
{
  unsigned char temp = 0;
  AUXR &= ~(0x10); // 定时器2停止计时

  P44 = 0;
  P42 = 1;
  P35 = 1;
  // P34 = 1;
  if (P33 == 0)
    temp = 4;
  if (P32 == 0)
    temp = 5;
  if (P31 == 0)
    temp = 6;
  if (P30 == 0)
    temp = 7;

  P44 = 1;
  P42 = 0;
  P35 = 1;
  // P34 = 1;
  if (P33 == 0)
    temp = 8;
  if (P32 == 0)
    temp = 9;
  if (P31 == 0)
    temp = 10;
  if (P30 == 0)
    temp = 11;

  P44 = 1;
  P42 = 1;
  P35 = 0;
  // P34 = 1;
  if (P33 == 0)
    temp = 12;
  if (P32 == 0)
    temp = 13;
  if (P31 == 0)
    temp = 14;
  if (P30 == 0)
    temp = 15;
  /*
    P44 = 1;
    P42 = 1;
    P35 = 1;
    P34 = 0;
    if (P33 == 0) temp = 16;
    if (P32 == 0) temp = 17;
    if (P31 == 0) temp = 18;
    if (P30 == 0) temp = 19;
  */
  P3 = 0xff;
  AUXR |= 0x10; // 定时器2开始计时
  return temp;
}