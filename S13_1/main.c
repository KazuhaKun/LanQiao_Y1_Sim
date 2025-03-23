#include <STC15F2K60S2.H>	//单片机头文件
#include "iic.h"	//IIC相关代码,直接调用EEPROM和AD/DA(Da_Write;Ad_Read;EEPROM_Read;EEPROM_Write)
#include "ds1302.h"		//直接操作rtc时钟(Read_Rtc;Set_Rtc)
#include "onewire.h"	//直接读出24C20温度(rd_temperature)
#include <string.h>		//memset需要
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "uart.h"
#include "ultrawave.h"

void Delay(unsigned int t){
	while(t--){
	}
}

//HC138的选择
void SetHC138(unsigned char channel,unsigned char dat){
	P2 = (P2 & 0x1f) | (channel << 5);
	P0 = dat;
}

//超多变量
unsigned char Key_Val, Key_Down, Key_Old, Key_Up;	//按键相关变量
unsigned char Seg_Buf[8]={17,17,17,17,17,17,17,17};		//数码管对应位置显示值存储数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};		//数码管对应位置小数点存储数组
unsigned char Seg_Pos;		//数码管刷新位置值
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};		//LED对应位置状态存储数组
unsigned char Uart_Recv[10];	//串口接收存储数组
unsigned char Uart_Recv_Index;	//串口数据索引
unsigned char ucRtc[3] = {8,59,50};	//Rtc时钟数值
unsigned int Slow_Down;		//用于计时中断服务函数
bit Seg_Flag, Key_Flag;		//同上
bit Uart_Flag;				//同上
unsigned int Time_1s;		//同上,主要负责频率的处理
unsigned int Freq;			//频率值
unsigned int Sys_Tick;		//
unsigned char Seg_Disp_Mode=0;
bit Relay_Mode=0;
bit Relay_Flag=0;
unsigned int temp_set=23;
bit time_disp_flag=0;
bit Flash_Flag;
float T;
unsigned int temp_disp;
unsigned int temp_set_disp;

void Key_Proc(){
	if(Key_Flag)return;
	Key_Flag = 1;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;


	switch(Key_Down){
		case 12:
			if(++Seg_Disp_Mode==3) Seg_Disp_Mode=0;
			break;
		case 13:
			Relay_Mode = !Relay_Mode;
			break;
		case 16:
			if(Seg_Disp_Mode==2) if(++temp_set==99) temp_set=0;
			break;
		case 17:
			if(Seg_Disp_Mode==2) if(--temp_set==0) temp_set=99;
			if(Seg_Disp_Mode==1) time_disp_flag = 1;
			break;
	}
	switch(Key_Up){
		case 17:
			if(Seg_Disp_Mode==1) time_disp_flag = 0;
			break;
	}
	// if(Seg_Disp_Mode==1){
	// 	if(Key_Old==17) time_disp_flag = 1;
	// 	else time_disp_flag = 0;
	// }
}

void Led_Proc(){
	if(ucRtc[1]==0 && ucRtc[2]<=5) ucLed[0]=1;
	else ucLed[0]=0;
	if(Relay_Mode==0) ucLed[1]=1;
	else ucLed[1]=0;
	if(Relay_Flag==1) ucLed[2]=Flash_Flag;
	else ucLed[2]=0;
}

void Relay_Proc(){
	if(Relay_Mode==0){
		if(T>temp_set) Relay_Flag=1;
		else Relay_Flag=0;
	}
	if(Relay_Mode==1){
		if(ucRtc[1]==0 && ucRtc[2]<=5) Relay_Flag=1;
		else Relay_Flag=0;
	}
}
void Seg_Proc(){
	if(Seg_Flag) return;
	Seg_Flag = 1;

	T=rd_temperature();
	temp_disp=T*10;
	// temp_set_disp=temp_set;
	
	if(Seg_Disp_Mode==0){
		Seg_Buf[0]=18;
		Seg_Buf[1]=1;
		Seg_Buf[5]=temp_disp/100%10;
		Seg_Buf[6]=temp_disp/10%10;
		Seg_Point[6]=1;
		Seg_Buf[7]=temp_disp%10;
	}
	if(Seg_Disp_Mode==1){
		Seg_Buf[0]=18;
		Seg_Buf[1]=2;
		if(time_disp_flag==0){
			Seg_Buf[3]=ucRtc[0]/10%10;
			Seg_Buf[4]=ucRtc[0]%10;
			Seg_Buf[5]=16;
			Seg_Buf[6]=ucRtc[1]/10%10;
			Seg_Point[6]=0;
			Seg_Buf[7]=ucRtc[1]%10;
		}
		if(time_disp_flag==1){
			Seg_Buf[3]=ucRtc[1]/10%10;
			Seg_Buf[4]=ucRtc[1]%10;
			Seg_Buf[5]=16;
			Seg_Buf[6]=ucRtc[2]/10%10;
			Seg_Point[6]=0;
			Seg_Buf[7]=ucRtc[2]%10;
		}
	}
	if(Seg_Disp_Mode==2){
		Seg_Buf[0]=18;
		Seg_Buf[1]=3;
		Seg_Buf[3]=17;
		Seg_Buf[4]=17;
		Seg_Buf[5]=17;
		Seg_Buf[6]=temp_set/10%10;
		Seg_Buf[7]=temp_set%10;
	}
	
}

// void Timer0_Init(void){
// 	AUXR &= 0x7f;
// 	TMOD &= 0xf0;
// 	TMOD |= 0x05;
// 	TL0 = 0;
// 	TH0 = 0;
// 	TF0 = 0;
// 	TR0 = 1;
// }
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
	if (Slow_Down % 10 == 0){
		Read_Rtc(ucRtc);
	}
	if (Uart_Flag) Sys_Tick++;
	if (++Time_1s == 1000){
		Time_1s = 0;
		Freq = TH0 << 8 | TL0;
		TH0 = 0;
		TL0 = 0;
	}
	if (Slow_Down % 100 == 0){
		Flash_Flag = !Flash_Flag;
	}
	Seg_Disp(Slow_Down % 8, Seg_Buf[Slow_Down % 8], Seg_Point[Slow_Down % 8]);
	Led_Disp(Slow_Down % 8, ucLed[Slow_Down % 8]);
	Relay(Relay_Flag);
}

void main(){
	InitSys();
	Timer1_Init();
	Set_Rtc(ucRtc);
	while(1){
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Relay_Proc();
	}
}