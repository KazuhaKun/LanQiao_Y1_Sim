#include <STC15F2K60S2.H>	//单片机头文件
#include "iic.h"	//IIC相关代码,直接调用EEPROM和AD/DA(Da_Write;Ad_Read;EEPROM_Read;EEPROM_Write)
#include "ds1302.h"		//直接操作rtc时钟(Read_Rtc;Set_Rtc)
#include "onewire.h"	//直接读出24C20温度(rd_temperature)
#include <stdio.h>		//串口部分使用putchar调用
#include <string.h>		//串口部分extern调用(将putchar从int转换为char)
#include <intrins.h>	//延时函数nop需要

sbit Tx = P1^0;		//超声波发射
sbit Rx = P1^1;		//超声波接收

void Delay(unsigned int t){
	while(t--){
	}
}

//超声波专用12us函数(40khz)
void Delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}
//HC138的选择
void SetHC138(unsigned char channel,unsigned char dat){
	P2 = (P2 & 0x1f) | (channel << 5);
	P0 = dat;
}
//系统初始化相关的函数
void Disable_LED(){
	SetHC138(4,0xff);
}
void Disbale_Buzzers(){
	SetHC138(5,0xff);
}
void InitSys(){
	Disable_LED();
	Disbale_Buzzers();
}
//超多变量
code unsigned char Seg_dula[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e,0x40,0xff};	//数码管段码
code unsigned char Seg_wela[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//数码管位码
unsigned char Key_Val, Key_Down, Key_Old, Key_Up;	//按键相关变量
unsigned char Seg_Buf[8]={17,17,17,17,17,17,17,17};		//数码管对应位置显示值存储数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};		//数码管对应位置小数点存储数组
unsigned char Seg_Pos;		//数码管刷新位置值
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};		//LED对应位置状态存储数组
unsigned char Uart_Recv[10];	//串口接收存储数组
unsigned char Uart_Recv_Index;	//串口数据索引
unsigned char ucRtc[3] = {11,12,13};	//Rtc时钟数值
unsigned int Slow_Down;		//用于计时中断服务函数
bit Seg_Flag, Key_Flag;		//同上
bit Uart_Flag;				//同上
unsigned int Time_1s;		//同上,主要负责频率的处理
unsigned int Freq;			//频率值
unsigned int Sys_Tick;		//

unsigned char Key_Read(){
	unsigned char temp = 0;
	P44 = 0;
	P42 = 1;
	P35 = 1;
	P34 = 1;
	if(P30 == 0) temp = 7;
	if(P31 == 0) temp = 6;
	if(P32 == 0) temp = 5;
	if(P33 == 0) temp = 4;
	P44 = 1;
	P42 = 0;
	P35 = 1;
	P34 = 1;
	if(P30 == 0) temp = 11;
	if(P31 == 0) temp = 10;
	if(P32 == 0) temp = 9;
	if(P33 == 0) temp = 8;
	P44 = 1;
	P42 = 1;
	P35 = 0;
	P34 = 1;
	if(P30 == 0) temp = 15;
	if(P31 == 0) temp = 14;
	if(P32 == 0) temp = 13;
	if(P33 == 0) temp = 12;
	P44 = 1;
	P42 = 1;
	P35 = 1;
	P34 = 0;
	if(P30 == 0) temp = 19;
	if(P31 == 0) temp = 18;
	if(P32 == 0) temp = 17;
	if(P33 == 0) temp = 16;
	return temp;
}
void Key_Proc(){
	if(Key_Flag)return;
	Key_Flag = 1;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;
}

void Led_Disp(unsigned char addr,enable){
	static unsigned char temp;
	static unsigned char temp_old;
	if(enable){
		temp |= 0x01 << addr; 
	}
	else{
		temp &= ~(0x01 << addr);
	}
	if(temp != temp_old){
		P0 = ~temp;
		P2 = P2 & 0x1f | 0x80;
		P2 &= 0x1f;
		temp_old = temp;
	}
}
void Led_Proc(){}

void Seg_Disp(unsigned char wela,dula,point){
	P0 = 0xff;
	P2 = P2 & 0x1f | 0xe0;
	P2 &= 0x1f;

	P0 = Seg_wela[wela];
	P2 = P2 & 0x1f | 0xc0;
	P2 &= 0x1f;

	P0 = Seg_dula[dula];
	P2 = P2 & 0x1f | 0xe0;
	P2 &= 0x1f;
}

void Seg_Proc(){
	if(Seg_Flag) return;
	Seg_Flag = 1;
	//
}
void UartInit(void)		//9600bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x01;		//串口1选择定时器2为波特率发生器
	AUXR |= 0x04;		//定时器2时钟为Fosc,即1T
	T2L = 0xC7;		//设定定时初值
	T2H = 0xFE;		//设定定时初值
	AUXR |= 0x10;		//启动定时器2
	ES = 1;
	EA = 1;
}

extern char putchar(char ch)
{
	SBUF = ch;
	while(TI ==0);
	TI = 0;
	return ch;
}

void Uart_Proc(){
	if(Uart_Recv_Index == 0)return;
	if(Sys_Tick >= 10){
		Sys_Tick = Uart_Flag = 0;
		\\逻辑处理

		memset(Uart_Recv, 0, Uart_Recv_Index);
		Uart_Recv_Index = 0;
	}
}

void Timer0_Init(void){
	AUXR &= 0x7f;
	TMOD &= 0xf0;
	TMOD |= 0x05;
	TL0 = 0;
	TH0 = 0;
	TF0 = 0;
	TR0 = 1;
}
void Timer1_Init(void){
	AUXR &= 0xbf;
	TMOD &= 0x0f;
	TL1 = (65536-1000)%256;
	TH1 = (65536-1000)/256;
	TF1 = 0;
	TR1 = 1;
	ET1 = 1;
	EA = 1;
}

void Timer1_Isr(void) interrupt 3
{
	if (++Slow_Down == 400){
		Seg_Flag = Slow_Down =0;
	}
	if (Slow_Down % 10 == 0){
		Key_Flag = 0;
	}
	if (Uart_Flag) Sys_Tick++;
	if (++Time_1s == 1000){
		Time_1s = 0;
		Freq = TH0 << 8 | TL0;
		TH0 = 0;
		TL0 = 0;
	}
	Seg_Disp(Slow_Down % 8, Seg_Buf[Slow_Down % 8], Seg_Point[Slow_Down % 8]);
	Led_Disp(Slow_Down % 8, ucLed[Slow_Down % 8]);
}

void Uart1Server() interrupt 4
{
	if(RI == 1){
		Uart_Flag = 1;
		Sys_Tick = 0;
		Uart_Recv[Uart_Recv_Index] = SBUF;
		Uart_Recv_Index++;
		RI = 0;
	}
	if(Uart_Recv_Index > 10) Uart_Recv_Index =0;
}

void Ut_Wave_Init(){
	unsigned char i;
	for(i=0;i<8;i++){
		Tx = 1;
		Delay12us();
		Tx = 0;
		Delay12us();
	}
}

unsigned char Ut_Wave_Date(){
	unsigned int time;
	CMOD = 0x00;
	CH = CL = 0;
	Ut_Wave_Init();
	CR = 1;
	while((Rx == 1) && (CF == 0));
	CR = 0;
	if(CF == 0){
		time = CH << 8 | CL;
		return (time * 0.017);
	}
	else{
		CF = 0;
		return 0;
	}
}

void main(){
	InitSys();
	Timer1_Init();
	Set_Rtc(ucRtc);
	UartInit();
	while(1){
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}