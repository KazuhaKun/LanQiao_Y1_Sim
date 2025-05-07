#include <STC15F2K60S2.H>	//单片机头文件
#include <stdio.h>		//串口部分使用putchar调用；串口部分extern调用(将putchar从int转换为char)

void UartInit(void);