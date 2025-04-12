/* 额外按键处理 */
idata unsigned char KeyN_Last;
idata unsigned char KeyN_Slow_Down;

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
        case N:
            if(KeyN_Last && KeyN_Slow_Down<30){
                //执行函数

                //重置
                KeyN_Last=0;
                KeyN_Slow_Down=0;
            }
            else {  //单击时操作
                KeyN_Last=1;
                KeyN_Slow_Down=0;
            }
    }
}

/* 定时器中断服务函数 */
void Timer1Server() interrupt 3
{  

    //增加双击超时
    if(KeyN_Last&&++KeyN_Slow_Down>=30){
        KeyN_Last=0;
        KeyN_Slow_Down=0;
    }
}