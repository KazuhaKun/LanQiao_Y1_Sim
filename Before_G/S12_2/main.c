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
/* 变量声明区 */
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量

unsigned char Seg_Buf[8] = {17,17,17,17,17,17,17,17};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
idata unsigned int Seg_Slow_Down;//数码管减速专用变量

idata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
idata unsigned char ucRtc[3] = {11,12,13};

/* 串口数据 */
idata unsigned char Uart_Recv[10];//串口接收数据储存数组 默认10个字节 若接收数据较长 可更改最大字节数
idata unsigned char Uart_Recv_Index;//串口接收数组指针
idata unsigned char Uart_Recv_Tick;     // 串口接收时间标志
idata unsigned char Uart_Rx_Flag;

/* NE555数据处理 */
idata unsigned int Time_1s;
idata unsigned int Freq;

unsigned char Seg_Disp_Mode;	//0 温度显示 1 参数设置 2 DAC
unsigned char DAC_Mode=0;
float T;
unsigned int temp_disp;
unsigned char temp_set=25;
unsigned char temp_DAC_set=25;
float DAC_Set;
unsigned int DAC_Disp;
idata unsigned int DAC_Slow_Down;

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

/* 信息处理函数 */
void Seg_Proc()
{
    if(Seg_Slow_Down<500) return;
    Seg_Slow_Down = 0;//数码管减速程序

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


/* 其他显示函数 */
void Led_Proc()
{
    ucLed[0]=(DAC_Mode==0);	
	ucLed[1]=(Seg_Disp_Mode==0);
	ucLed[2]=(Seg_Disp_Mode==1);
	ucLed[3]=(Seg_Disp_Mode==2);
}

// /* 串口处理函数 */
// void Uart_Proc()
// {
//     if (Uart_Recv_Index == 0) return; // 无数据，直接返回
//     if (Uart_Recv_Tick >= 10)
//     { // 若接收超时，对数据读取，清空缓存区
//         Uart_Rx_Flag = 0;
//         Uart_Recv_Tick = 0;

//         memset(Uart_Recv, 0, Uart_Recv_Index); // 清空接收数据
//         Uart_Recv_Index = 0;
//     }
// }

// /* 定时器0中断初始化函数 */
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

/* 定时器0中断服务函数 */
void Timer1Server() interrupt 3
{  
    Key_Slow_Down ++;
    Seg_Slow_Down ++;
	DAC_Slow_Down ++;

    if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
     // 数码管显示处理
    if (Seg_Buf[Seg_Pos] > 20)
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - ',', 1); // 带小数点
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0); // 无小数点
    
    Led_Disp(ucLed); 

    // if (++Time_1s == 1000){
	// 	Time_1s = 0;
	// 	Freq = TH0 << 8 | TL0;
	// 	TH0 = 0;
	// 	TL0 = 0;
	// }
    
    // if(Uart_Rx_Flag == 1)
    //     Uart_Recv_Tick ++;
}

// /* 串口1中断服务函数 */
// void Uart1Server() interrupt 4
// {
//     if(RI == 1) //串口接收数据
//     {
//         Uart_Rx_Flag = 1;
//         Uart_Recv_Tick = 0;
//         Uart_Recv[Uart_Recv_Index] = SBUF;
//         Uart_Recv_Index++;
//         RI = 0;
//         if(Uart_Recv_Index>10)
//         {
//             Uart_Recv_Index = 0;
//             memset(Uart_Recv,0,10);
//         }
//     }
// }

/* Main */
void main()
{
    System_Init();
    // Uart_Init();
    Timer1Init();
    Set_Rtc(ucRtc);
    while (1)
    {
        Key_Proc();
        Seg_Proc();
        Led_Proc();
        DA_Proc();

        // Uart_Proc();
    }
}