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
#include <ultrasound.h>
#include <stdio.h>
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量

unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
idata unsigned int Seg_Slow_Down;//数码管减速专用变量

idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组

/* 串口数据 */
idata unsigned char Uart_Recv[10];//串口接收数据储存数组 默认10个字节 若接收数据较长 可更改最大字节数
idata unsigned char Uart_Recv_Index;//串口接收数组指针
idata unsigned char Uart_Recv_Tick;     // 串口接收时间标志
idata unsigned char Uart_Rx_Flag;

/* NE555数据处理 */
idata unsigned int Time_1s;
idata unsigned int Freq;



/* 键盘处理函数 */
void Key_Proc()
{
    if(Key_Slow_Down<10) return;
    Key_Slow_Down = 0;//键盘减速程序

    Key_Val = Key_Read();//实时读取键码值
    Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降沿
    Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上降沿
    Key_Old = Key_Val;//辅助扫描变量


}

/* 信息处理函数 */
void Seg_Proc()
{
    if(Seg_Slow_Down<500) return;
    Seg_Slow_Down = 0;//数码管减速程序


}

/* 其他显示函数 */
void Led_Proc()
{

}

/* 串口处理函数 */
void Uart_Proc()
{
    if (Uart_Recv_Index == 0) return; // 无数据，直接返回
    if (Uart_Recv_Tick >= 10)
    { // 若接收超时，对数据读取，清空缓存区
        Uart_Rx_Flag = 0;
        Uart_Recv_Tick = 0;
        switch(Uart_Recv[0]){
            case 1:
                printf("Welcome");
        }
        memset(Uart_Recv, 0, Uart_Recv_Index); // 清空接收数据
        Uart_Recv_Index = 0;
    }

}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1微秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TMOD |= 0x05;		//设置定时器模式
	TL0 = 0;		//设置定时初值
	TH0 = 0;		//设置定时初值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
}


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

/* 定时器0中断服务函数 */
void Timer1Server() interrupt 3
{  
    Key_Slow_Down ++;
    Seg_Slow_Down ++;

    // //双击超时
    // if(KeyN_Last&&++KeyN_Slow_Down>=30){
    //     KeyN_Last=0;
    //     KeyN_Slow_Down=0;
    // }
    if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
     // 数码管显示处理
    if (Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1); // 带小数点
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0); // 无小数点
    
    Led_Disp(ucLed); 

    if (++Time_1s == 1000){
		Time_1s = 0;
		Freq = TH0 << 8 | TL0;
		TH0 = 0;
		TL0 = 0;
	}
    
    if(Uart_Rx_Flag == 1)
        Uart_Recv_Tick ++;
}

/* 串口1中断服务函数 */
void Uart1Server() interrupt 4
{
    if(RI == 1) //串口接收数据
    {
        Uart_Rx_Flag = 1;
        Uart_Recv_Tick = 0;
        Uart_Recv[Uart_Recv_Index] = SBUF;
        Uart_Recv_Index++;
        RI = 0;
        if(Uart_Recv_Index>10)
        {
            Uart_Recv_Index = 0;
            memset(Uart_Recv,0,10);
        }
    }
}

/* Main */
void main()
{
    System_Init();
    Uart_Init();
    Timer1Init();
    while (1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
        Uart_Proc();
    }
}