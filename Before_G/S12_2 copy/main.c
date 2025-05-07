#include <STC15F2K60S2.H>
#include <stdio.h>
#include <string.h>
#include "ds1302.h"
#include "iic.h"
#include "init.h"
#include "key.h"
#include "led.h"
#include "onewire.h"
#include "seg.h"
#include "uart.h"
#include "ultrawave.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;

idata unsigned char Seg_Buf[8] = {17,17,17,17,17,17,17,17};//数码管显示数据存放数组
idata unsigned char Seg_Pos;
idata unsigned int Seg_Slow_Down;

idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};

idata unsigned char ucRtc[3]={11,12,13};

idata unsigned char Uart_Recv[10];
idata unsigned char Uart_Recv_Index;
idata unsigned char Uart_Recv_Tick;
idata unsigned char Uart_Rx_Flag;

idata unsigned int Time_1s;
idata unsigned int Freq;

// idata unsigned char KeyN_Last;
// idata unsigned char KeyN_Slow_Down;

unsigned char Seg_Disp_Mode;	//0 温度显示 1 参数设置 2 DAC
unsigned char DAC_Mode=0;
float T;
unsigned int temp_disp;
unsigned char temp_set=25;
unsigned char temp_DAC_set=25;
float DAC_Set;
unsigned int DAC_Disp;
idata unsigned int DAC_Slow_Down;



void Key_Proc(){
    if(Key_Slow_Down < 10) return;
    Key_Slow_Down=0;

    Key_Val=Key_Read();
    Key_Down=Key_Val&(Key_Old^Key_Val);
    Key_Up=~Key_Val&(Key_Old^Key_Val);
    Key_Old=Key_Val;

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

void Seg_Proc(){
    if(Seg_Slow_Down<500) return;
    Seg_Slow_Down=0;

    T=rd_temperature();
	// T=15.0;
	temp_disp = T * 100;
	//
	if(Seg_Disp_Mode==0){
		Seg_Buf[0]=12;
		Seg_Buf[4]=temp_disp/1000%10;
		Seg_Buf[5]=temp_disp/100%10 + ',';
		Seg_Buf[6]=temp_disp/10%10;
		Seg_Buf[7]=temp_disp%10;
	}
	if(Seg_Disp_Mode==1){
		Seg_Buf[0]=18;
		Seg_Buf[4]=17;
		Seg_Buf[5]=17;
		Seg_Buf[6]=temp_set/10%10;
		Seg_Buf[7]=temp_set%10;
	}
	if(Seg_Disp_Mode==2){
		Seg_Buf[0]=10;
		Seg_Buf[4]=17;
		Seg_Buf[5]=DAC_Disp/100%10 + ',';
		Seg_Buf[6]=DAC_Disp/10%10;
		Seg_Buf[7]=DAC_Disp%10;
	}




}

void DA_Proc(){
	// if(DAC_Slow_Down < 300) return;
	// DAC_Slow_Down = 0;
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

void Led_Proc(){
    ucLed[0]=(DAC_Mode==0);	
	ucLed[1]=(Seg_Disp_Mode==0);
	ucLed[2]=(Seg_Disp_Mode==1);
	ucLed[3]=(Seg_Disp_Mode==2);

}

// void Uart_Proc(){
//     if(Uart_Recv_Index == 0) return;
//     if(Uart_Recv_Tick >= 10){
//         Uart_Rx_Flag=0;
//         Uart_Recv_Tick=0;

//         memset(Uart_Recv,0,Uart_Recv_Index);
//         Uart_Recv_Index=0;
//     }
// }

// void Timer0Init(void)		//1微秒@12.000MHz
// {
// 	AUXR &= 0x7F;		//定时器时钟12T模式
// 	TMOD &= 0xF0;		//设置定时器模式
// 	TMOD |= 0x05;		//设置定时器模式
// 	TL0 = 0;		//设置定时初值
// 	TH0 = 0;		//设置定时初值
// 	TF0 = 0;		//清除TF0标志
// 	TR0 = 1;		//定时器0开始计时
// }

void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x18;		//设置定时初值
	TH1 = 0xFC;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1;		//定时器1开始计时
    ET1 = 1;
    EA = 1;
}

void Timer1Isr() interrupt 3
{
    Key_Slow_Down++;
    Seg_Slow_Down++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

    // if(++Time_1s==1000){
    //     Time_1s=0;
    //     Freq = TH0<<8|TL0;
    //     TH0 = 0;
    //     TL0 = 0;
    // }

    // if(Uart_Rx_Flag==1){
    //     Uart_Recv_Tick++;
    // }

    // if(KeyN_Last&&++KeyN_Slow_Down>=30){
    //     KeyN_Last=0;
    //     KeyN_Slow_Down=0;
    // }
}

// void Uart1Isr() interrupt 4
// {
//     if(RI==1){
//         Uart_Rx_Flag=1;
//         Uart_Recv_Tick=0;
//         Uart_Recv[Uart_Recv_Index]=SBUF;
//         Uart_Recv_Index++;
//         RI=0;
//         if(Uart_Recv_Index>10){
//             Uart_Recv_Index=0;
//             memset(Uart_Recv,0,10);
//         }
//     }
// }

void main(){
    InitSys();
    // UartInit();
    Timer1Init();
	Set_Rtc(ucRtc);
    while(1){
        Key_Proc();
        Led_Proc();
        Seg_Proc();
		DA_Proc();

        //Uart_Proc();
    }
}