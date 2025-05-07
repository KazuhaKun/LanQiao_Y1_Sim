#include <STC15F2K60S2.H>	//单片机头文件
#include "iic.h"	//IIC相关代码,直接调用EEPROM和AD/DA(Da_Write;Ad_Read;EEPROM_Read;EEPROM_Write)
#include "ds1302.h"		//直接操作rtc时钟(Read_Rtc;Set_Rtc)
#include "onewire.h"	//直接读出24C20温度(rd_temperature)
#include <string.h>		//memset需要
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
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
unsigned char ucRtc[3] = {11,12,13};	//Rtc时钟数值
unsigned int Slow_Down;		//用于计时中断服务函数
bit Seg_Flag, Key_Flag;		//同上
bit Uart_Flag;				//同上
unsigned int Time_1s;		//同上,主要负责频率的处理
unsigned int Freq;			//频率值
unsigned int Sys_Tick;		//
unsigned char Seg_Disp_Mode;	//0 温度显示 1 参数设置 2 DAC
unsigned char DAC_Mode=0;
float T;
unsigned int temp_disp;
unsigned char temp_set=25;
unsigned char temp_DAC_set=25;
float DAC_Set;
unsigned int DAC_Disp;

void Key_Proc(){
	if(Key_Flag)return;
	Key_Flag = 1;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;

	switch(Key_Down){
		case 4:
			if(++Seg_Disp_Mode==3) Seg_Disp_Mode=0;
			if(Seg_Disp_Mode==2) temp_DAC_set=temp_set;
			break;
		case 8:
			if(Seg_Disp_Mode==1) {
				// temp_set--;
				if(--temp_set==255) temp_set=0;

			}
			break;
		case 9:
			if(Seg_Disp_Mode==1) {
				// if(++temp_set==100) temp_set=99;
				if(++temp_set==0) temp_set=255;
			}
			break;
		case 5:
			if(++DAC_Mode==2) DAC_Mode=0;
			break;
		
	}
}

void Led_Proc(){
	ucLed[0]=(DAC_Mode==0);	
	ucLed[1]=(Seg_Disp_Mode==0);
	ucLed[2]=(Seg_Disp_Mode==1);
	ucLed[3]=(Seg_Disp_Mode==2);
}

void Seg_Proc(){
	if(Seg_Flag) return;
	Seg_Flag = 1;

	T=rd_temperature();
	// T=15.0;
	temp_disp = T * 100;
	//
	if(Seg_Disp_Mode==0){
		Seg_Buf[0]=12;
		Seg_Buf[4]=temp_disp/1000%10;
		Seg_Buf[5]=temp_disp/100%10;
		Seg_Point[5]=1;
		Seg_Buf[6]=temp_disp/10%10;
		Seg_Buf[7]=temp_disp%10;
	}
	if(Seg_Disp_Mode==1){
		Seg_Buf[0]=18;
		Seg_Buf[4]=17;
		Seg_Buf[5]=17;
		Seg_Point[5]=0;
		Seg_Buf[6]=temp_set/10%10;
		Seg_Buf[7]=temp_set%10;
	}
	if(Seg_Disp_Mode==2){
		Seg_Buf[0]=10;
		Seg_Buf[4]=17;
		Seg_Buf[5]=DAC_Disp/100%10;
		Seg_Point[5]=1;
		Seg_Buf[6]=DAC_Disp/10%10;
		Seg_Buf[7]=DAC_Disp%10;
	}
}

void DA_Proc(){
	if(DAC_Mode==0) {
		if(T<temp_DAC_set) DAC_Set=0;
		else DAC_Set=255;
	}
	if(DAC_Mode==1) {
		if(T<=20) DAC_Set=51;
		else if(T<=40) DAC_Set=7.65*T-102;
		else if(T>40) DAC_Set=204;
	}
	DAC_Disp = DAC_Set*100/51.0;
	Da_Write(DAC_Set);
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
	Seg_Disp(Slow_Down % 8, Seg_Buf[Slow_Down % 8], Seg_Point[Slow_Down % 8]);
	Led_Disp(Slow_Down % 8, ucLed[Slow_Down % 8]);
	// if (Slow_Down % 100 == 0) Da_Write(DAC_Set);
}

void main(){
	InitSys();
	Timer1_Init();
	Set_Rtc(ucRtc);
	// Init_18B20();
	while(1){
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		DA_Proc();
	}
}