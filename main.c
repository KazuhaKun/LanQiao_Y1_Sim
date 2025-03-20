#include "stdio.h"
#include "bsp_init.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_seg.h"
#include "bsp_onewire.h"
#include "bsp_iic.h"

//函数声明区
void key_proc();
void seg_proc();
void led_proc();
void task_proc();
//按键专用变量
unsigned char key_down,key_up,key_old,key_value;
//数码管专用变量
unsigned char seg_buf[8]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char pos;
unsigned char seg_string[10];
//led专用变量
unsigned char ucled;
//调度函数专用变量
unsigned char cnt_T;
unsigned char cnt_da;

//减速专用变量
unsigned char key_slow_down,led_slow_down,seg_slow_down;
//滴答定时器专用变量
unsigned long int ms_Tick;
//用户自定义变量区
float T;
float  DA;
unsigned char state_flag;
unsigned char mode_da=1;
unsigned char T_val=25;
unsigned char T_val_sure=25;

void main()
{
	cls_per_init();
	Timer1Init();
	rd_temp();
	Delay750ms();
	while(1)
	{
		
		key_proc();
		seg_proc();
		led_proc();
		task_proc();
		
	}
}


void tim_isr() interrupt 3
{
	ms_Tick++;
	
	if(cnt_T<100)  cnt_T++;
	if(cnt_da<160) cnt_da++;
	
	
	if(++key_slow_down==10) key_slow_down=0;
	if(++led_slow_down==10) led_slow_down=0;
	if(++seg_slow_down==30) seg_slow_down=0;

	
	seg_disp(seg_buf,pos);
	if(++pos==8) pos=0;
	
	
	led_disp(ucled);
}


void task_proc()
{
	
	if(cnt_T==100)
	{
		T=rd_temp()/16.0;
		cnt_T=0;
	}
	
	//DA的处理
	
	if(mode_da==1)
	{
		if(T<T_val_sure)
		{
			DA=0;		
		}
		
		else 
		{
			DA=255;
		}
	}
	
	else if(mode_da==2)
	{
		if(T<20)
		{
			DA=51;
		}
		
		else if(T>40)
		{
			DA=204;
		}
		
		else
		{
			DA=(7.65*(T-20)+51);
		}
		
	}
	
	if(cnt_da==160)
	{
		pcf8591_dac(DA);
		cnt_da=0;
	}
	
	
	
	
}



void key_proc()
{
	if(key_slow_down) return;
	key_slow_down=1;
	
	key_value=key_read();
	key_down=key_value&(key_value^key_old);
	key_up=~key_value&(key_value^key_old);
	key_old=key_value;
	
	switch(key_down)
	{
		
		case 4:
			if(++state_flag==3) state_flag=0;
		  if(state_flag==2)
			T_val_sure=T_val;
		break;
		
		case 8:
			if(state_flag==1)
			{
				T_val-=1;
			if(T_val==255)
				T_val=0;
		 }
		break;
		
		case 9:
			if(state_flag==1)
			{
				T_val+=1;
			if(T_val==0)
				T_val=255;
		 }
		break;
		
		case 5:
			 if(mode_da==2)
				  mode_da=1;
			 else if(mode_da==1)
				 mode_da=2;
    break;	

		
		 
		
	}
	
}


void seg_proc()
{
	if(seg_slow_down) return;
	seg_slow_down=1;
	
	switch(state_flag)
	{
		case 0:
			sprintf(seg_string,"C   %5.2f",T);
		break;
		
		case 1:
			sprintf(seg_string,"P     %2d",(unsigned int)T_val);
		break;
		
		case 2:
			sprintf(seg_string,"A    %4.2f",(DA/51.0));
		break;
	}
	
	seg_tran(seg_string,seg_buf);
}


void led_proc()
{
	if(led_slow_down) return;
	led_slow_down=1;
	
	
	if(mode_da==1)
		ucled |=0x01;
	else
		ucled &=(~0x01);
	
	if(state_flag==0)
		ucled |=0x02;
	else
		ucled &=(~0x02);
	
	if(state_flag==1)
	  ucled |=0x04;
	else
		ucled &=(~0x04);
	
	if(state_flag==2)
	  ucled |=0x08;
	else
		ucled &=(~0x08);
	
	
	
	
}
