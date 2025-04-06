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
unsigned char ucRtc[3] = {13,03,05};	//Rtc时钟数值
unsigned int Slow_Down;		//用于计时中断服务函数
bit Seg_Flag, Key_Flag;		//同上
bit Uart_Flag;				//同上
unsigned int Time_1s;		//同上,主要负责频率的处理
unsigned int Freq;			//频率值
unsigned int Freq_Disp,Freq_Max;
bit Freq_Disp_Flag;
unsigned int Sys_Tick;		//
float T;
unsigned int T_Disp;
char T_Set=30;
unsigned char T_Max;
bit Trg_Flag;
unsigned char Trg_Ct;
unsigned char Trg_Time[2];
unsigned char Seg_Mode=0;
unsigned char Disp_Mode=0;


void Key_Proc(){
	if(Key_Flag)return;
	Key_Flag = 1;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;

	switch(Key_Down){
		case 4:
			if(++Seg_Mode==4) Seg_Mode=0;
			break;
		case 5:
			if(Seg_Mode==1 && ++Disp_Mode==3) Disp_Mode=0;
			break;
		case 8:
			if(Seg_Mode==2 && ++T_Set==99) T_Set=0;
			break;
		case 9:
			if(Seg_Mode==2 && --T_Set==-1) T_Set=99;
			break;
	}

}

void Led_Proc(){}

void Seg_Proc(){
	if(Seg_Flag) return;
	Seg_Flag = 1;
	//温度处理
	T=rd_temperature();
	T_Disp = T*10;
	if(T>T_Max) T_Max = T;
	//湿度处理
	if(Freq<200) Freq_Disp_Flag=0;
	else if(Freq<=2000) {Freq_Disp_Flag=1;Freq_Disp = 4*Freq/9.0 + 100/9.0;}
	else Freq_Disp_Flag=0;
	if(Freq_Disp>Freq_Max) Freq_Max = Freq_Disp;
	//
	if(Seg_Mode==0){
		Seg_Buf[0] = ucRtc[0]/10%10;
		Seg_Buf[1] = ucRtc[0]%10;
		Seg_Buf[2] = 16;
		Seg_Buf[3] = ucRtc[1]/10%10;
		Seg_Buf[4] = ucRtc[1]%10;
		Seg_Buf[5] = 16;
		Seg_Buf[6] = ucRtc[2]/10%10;
		Seg_Buf[7] = ucRtc[2]%10;
	}
	if(Seg_Mode==1){
		if(Disp_Mode==0){
			Seg_Buf[0] = 12;
			Seg_Buf[1] = 17;
			Seg_Buf[2] = T_Max/10%10;
			Seg_Buf[3] = T_Max%10;
			Seg_Buf[4] = 16;
			Seg_Buf[5] = T_Disp/100%10;
			Seg_Buf[6] = T_Disp/10%10;
			Seg_Point[6] = 1;
			Seg_Buf[7] = T_Disp%10;
		}
		if(Disp_Mode==1){
			Seg_Buf[0] = 18;
			Seg_Buf[1] = 17;
			Seg_Buf[2] = Freq_Max/100%10;
			Seg_Buf[3] = Freq_Max/10%10;
			Seg_Buf[4] = 16;
			Seg_Buf[5] = Freq_Disp/100%10;
			Seg_Buf[6] = Freq_Disp/10%10;
			Seg_Point[6] = 1;
			Seg_Buf[7] = Freq_Disp%10;
		}
		if(Disp_Mode==2){
			Seg_Buf[0] = 15;
			Seg_Buf[1] = Trg_Ct/10%10;
			Seg_Buf[2] = Trg_Ct%10;
			Seg_Buf[3] = Trg_Time[0]/10%10;
			Seg_Buf[4] = Trg_Time[0]%10;
			Seg_Buf[5] = 16;
			Seg_Buf[6] = Trg_Time[1]/10%10;
			Seg_Point[6] = 0;
			Seg_Buf[7] = Trg_Time[1]%10;
		}
	}
	if(Seg_Mode==2){
		Seg_Buf[0] = 19;
		Seg_Buf[1] = 17;
		Seg_Buf[2] = 17;
		Seg_Buf[3] = 17;
		Seg_Buf[4] = 17;
		Seg_Buf[5] = 17;
		Seg_Buf[6] = T_Set/10%10;
		Seg_Point[6] = 0;
		Seg_Buf[7] = T_Set%10;
	}
	if(Seg_Mode==3){
		Seg_Buf[0] = 14;
		Seg_Buf[1] = 17;
		Seg_Buf[2] = 17;
		Seg_Buf[3] = T_Disp/10%10;
		Seg_Buf[4] = T_Disp%10;
		Seg_Buf[5] = 16;
		Seg_Buf[6] = Freq_Disp/10%10;
		Seg_Buf[7] = Freq_Disp%10;
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

void main(){
	InitSys();
	Timer0_Init();
	Timer1_Init();
	Set_Rtc(ucRtc);
	while(1){
		Read_Rtc(ucRtc);
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}