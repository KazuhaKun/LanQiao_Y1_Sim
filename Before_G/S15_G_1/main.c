/***************************************************************************************
* Copyright (C) 2025 米醋电子工作室
*
* 版本         ：V4T
* 作者         ：米醋电子工作室
* 描述         ：该版本为V2版本的改进版，针对4T平台做了专项优化，修复了此前版本的若干Bug
*
* 版权归米醋电子工作室所有，并保留所有权力。
* 未经明确书面许可，任何个人或机构不得擅自用于商业用途。
***********************************************************************************************/

/* 头文件声明区 */
#include <STC15F2K60S2.H>//单片机寄存器专用头文件
#include <Init.h>//初始化底层驱动专用头文件
#include <Led.h>//Led底层驱动专用头文件
#include <Key.h>//按键底层驱动专用头文件
#include <Seg.h>//数码管底层驱动专用头文件
#include <Uart.h>//串口底层驱动专用头文件
#include <string.h>
#include <iic.h>
#include <ds1302.h>
#include <onewire.h>
#include <ultrawave.h>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
idata unsigned int Seg_Slow_Down;//数码管减速专用变量
idata unsigned char L3_Count=0;
idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
idata unsigned char Uart_Slow_Down;//串口减速专用变量
idata unsigned char Uart_Recv[10];//串口接收数据储存数组 默认10个字节 若接收数据较长 可更改最大字节数
idata unsigned char Uart_Recv_Index;//串口接收数组指针
idata unsigned char Uart_Send[10];//串口接收数据储存数组 默认10个字节 若发送数据较长 可更改最大字节数
/* 串口数据 */
pdata unsigned char Uart_Buf[10] = {0}; // 串口接收缓冲区
idata unsigned char Uart_Rx_Index;      // 串口接收索引
idata unsigned char Uart_Recv_Tick;     // 串口接收时间标志
idata unsigned char Uart_Rx_Flag;
/* 坐标变量 */
unsigned int X,Y;
unsigned int X_Set,Y_Set;
unsigned int XY_Uart;
/* 模式标志 */
unsigned char Seg_Mode=0;	//0坐标 1速度 2参数
unsigned char Run_Status=0;  //0空闲 1运行 2等待
/* 速度 */
unsigned int Speed,Speed_Disp;
/* 距离 */
unsigned int Juli;
bit Wait_Flag;
/* 参数 */
unsigned char _R=10;
char _B=0;
bit p_mode=0;	// 0R 1B
/* LED闪烁标志位 */
bit Flash_Flag=0;
/* 日夜场景标志位 */
bit Night_Flag=0;
/* 到达标志位 */
bit Arv_Flag=0;

/* 键盘处理函数 */
void Key_Proc()
{
	if(Key_Slow_Down<10) return;
	Key_Slow_Down = 0;//键盘减速程序

	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down){
		case 4:
			if(Run_Status == 0 && (X_Set !=0 | Y_Set != 0)) Run_Status = 1;
			if(Run_Status == 2 && Wait_Flag == 0) Run_Status = 1;
			if(Run_Status == 1) Run_Status = 2;
			break;
		case 5:
			if(Run_Status == 0) {X=0;Y=0;}
			break;
		case 8:
			if(++Seg_Mode == 3) Seg_Mode=0;
			break;
		case 9:
			if(Seg_Mode == 2) p_mode = !p_mode;
			break;
		case 12:
			if(Seg_Mode == 2){
				if(p_mode==0) if(++_R==21) _R=10;
				if(p_mode==1) if((_B+5)>90) _B=-90;
			} 
			break;
		case 13:
			if(Seg_Mode == 2){
				if(p_mode==0) if(--_R==9) _R=20;
				if(p_mode==1) if((_B-5)<-90) _B=90;
			} 
			break;

	}
}

/* 信息处理函数 */
void Seg_Proc()
{
	if(Seg_Slow_Down<500) return;
	Seg_Slow_Down = 0;//数码管减速程序
	

	Juli = Ut_Wave_Data();
	if(Juli<30) Wait_Flag=1;
	else Wait_Flag = 0;

	if(X==X_Set && Y==Y_Set) Arv_Flag=1;
	else Arv_Flag=0;

	if(Arv_Flag==1){
		if(++L3_Count==6) Arv_Flag=0;
	}

	Speed = 3.14 * (R/10.0) * F / (100.0 + B);
	Speed_Disp = 10 * Speed;
	
	// if(Ad_Read(0x94)>12) Night_Flag=0;
	// else Night_Flag=1;

	switch(Seg_Mode){
		case 0:
			switch(Run_Status){
				case 0:
					Seg_Buf[0] = 18;
					if(X/100%10 != 0) Seg_Buf[1] = X/100%10;
					else Seg_Buf[1] = 16;
					if(X/10%10 != 0) Seg_Buf[2] = X/10%10;
					else Seg_Buf[2] = 16;
					Seg_Buf[3] = X%10;
					Seg_Buf[4] = 17;
					if(Y/100%10 != 0) Seg_Buf[5] = Y/100%10;
					else Seg_Buf[5] = 16;
					if(Y/10%10 != 0) Seg_Buf[6] = Y/10%10;
					else Seg_Buf[6] = 16;
					Seg_Buf[7] = Y%10;
					break;
				case 1:
				case 2:
					Seg_Buf[0] = 18;
					if(X_Set/100%10 != 0) Seg_Buf[1] = X_Set/100%10;
					else Seg_Buf[1] = 16;
					if(X_Set/10%10 != 0) Seg_Buf[2] = X_Set/10%10;
					else Seg_Buf[2] = 16;
					Seg_Buf[3] = X_Set%10;
					Seg_Buf[4] = 17;
					if(Y_Set/100%10 != 0) Seg_Buf[5] = Y_Set/100%10;
					else Seg_Buf[5] = 16;
					if(Y_Set/10%10 != 0) Seg_Buf[6] = Y_Set/10%10;
					else Seg_Buf[6] = 16;
					Seg_Buf[7] = Y_Set%10;
					break;
			}
			break;
		case 1:
			switch(Run_Status){
				case 0:
					Seg_Buf[0] = 14;
					Seg_Buf[1] = 2;
					Seg_Buf[2] = 16;
					Seg_Buf[3] = 17;
					Seg_Buf[4] = 17;
					Seg_Buf[5] = 17;
					Seg_Buf[6] = 17;
					Seg_Buf[7] = 17;
					break;
				case 1:
					Seg_Buf[0] = 14;
					Seg_Buf[1] = 1;
					Seg_Buf[2] = 16;
					if(Speed_Disp/10000%10 != 0) Seg_Buf[3] = Speed_Disp/10000%10;
					else Seg_Buf[3] = 16;
					if(Speed_Disp/1000%10 != 0) Seg_Buf[4] = Speed_Disp/1000%10;
					else Seg_Buf[4] = 16;
					if(Speed_Disp/100%10 != 0) Seg_Buf[5] = Speed_Disp/100%10;
					else Seg_Buf[5] = 16;
					if(Speed_Disp/10%10 != 0) Seg_Buf[6] = Speed_Disp/10%10 + ',';
					else Seg_Buf[6] = 16;
					Seg_Buf[7] = Speed_Disp%10;
					break;
				case 2:
					Seg_Buf[0] = 14;
					Seg_Buf[1] = 3;
					Seg_Buf[2] = 16;
					if(Juli/10000%10 != 0) Seg_Buf[3] = Juli/10000%10;
					else Seg_Buf[3] = 16;
					if(Juli/1000%10 != 0) Seg_Buf[4] = Juli/1000%10;
					else Seg_Buf[4] = 16;
					if(Juli/100%10 != 0) Seg_Buf[5] = Juli/100%10;
					else Seg_Buf[5] = 16;
					if(Juli/10%10 != 0) Seg_Buf[6] = Juli/10%10;
					else Seg_Buf[6] = 16;
					Seg_Buf[7] = Juli%10;
					break;
			}
		case 2:
			Seg_Buf[0] = 19;
			Seg_Buf[1] = 16;
			Seg_Buf[2] = _R/10%10 + ',';
			Seg_Buf[3] = _R%10;
			Seg_Buf[4] = 16;
			if(_B<0 && _B/10!=0) Seg_Buf[5] = 17;
			else Seg_Buf[5] = 16;
			if(_B<0 && _B/10==0) Seg_Buf[6] = 17;
			else if(_B/10==0) Seg_Buf[6] = 16;
			else Seg_Buf[6] = _B/10%10;
			Seg_Buf[7] = _B%10;
			break;
	}

}

/* 其他显示函数 */
void Led_Proc()
{
	if(Run_Status==2) ucLed[0]=Flash_Flag;
	else ucLed[0]=Run_Status;

	if(Run_Status==1){
		if(Night_Flag) ucLed[1]=0;
		else ucLed[1]=1;
	}
	else ucLed[1]=0;

	ucLed[2]=Arv_Flag;

}

/* 串口处理函数 */
void Uart_Proc()
{
	if(Uart_Slow_Down<10)return;
	Uart_Slow_Down=0;
	
  if (Uart_Rx_Index == 0)
    return; // 无数据，直接返回
  if (Uart_Recv_Tick >= 10)
  { // 若接收超时，对数据读取，清空缓存区
    Uart_Recv_Tick = 0;
    Uart_Rx_Flag = 0;
    memset(Uart_Buf, 0, Uart_Rx_Index); // 清空接收数据
    Uart_Rx_Index = 0;
  }

	switch(Uart_Buf[0]){
		case '(':
			if(Run_Status==0){
				Uart_Send[0] = 'G';
				Uart_Send[1] = 'o';
				Uart_Send[2] = 't';
				Uart_Send[3] = ' ';
				Uart_Send[4] = 'i';
				Uart_Send[5] = 't';
			}
			else {
				Uart_Send[0] = 'B';
				Uart_Send[1] = 'u';
				Uart_Send[2] = 's';
				Uart_Send[3] = 'y';
			}
			break;
		case '?':
			if(Run_Status==0){
				Uart_Send[0] = 'I';
				Uart_Send[1] = 'd';
				Uart_Send[2] = 'l';
				Uart_Send[3] = 'e';
			}
			else if(Run_Status==1){
				Uart_Send[0] = 'B';
				Uart_Send[1] = 'u';
				Uart_Send[2] = 's';
				Uart_Send[3] = 'y';
			}
			else if(Run_Status==2){
				Uart_Send[0] = 'W';
				Uart_Send[1] = 'a';
				Uart_Send[2] = 'i';
				Uart_Send[3] = 't';
			}
			break;
	}   
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}

/* 定时器0中断服务函数 */
void Timer0Server() interrupt 1
{  
	Key_Slow_Down ++;
	Seg_Slow_Down ++;
	Uart_Slow_Down++;
	if(Seg_Slow_Down % 100 == 0) Flash_Flag=!Flash_Flag;
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	 // 数码管显示处理
  if (Seg_Buf[Seg_Pos] > 20)
    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1); // 带小数点
  else
    Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0); // 无小数点
   Led_Disp(ucLed); 
}

/* 串口1中断服务函数 */
void Uart1Server() interrupt 4
{
	if(RI == 1) //串口接收数据
	{
		Uart_Recv[Uart_Recv_Index] = SBUF;
		Uart_Recv_Index++;
		RI = 0;
	}
}

/* Main */
void main()
{
	System_Init();
	Timer0Init();
	UartInit();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
} 