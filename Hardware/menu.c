#include "stm32f10x.h" // Device header
#include "menu.h"
#include "Key.h"
#include "LED.h"
#include "MPU6050.h"
#include "MyRTC.h"
#include "OLED.h"
#include "SetTime.h"
#include "delay.h"
#include <math.h>
#include "mpu6050.h"    
#include "dht11.h"
#include "dino.h"
#include "menu_sub.h"

uint8_t KeyNum = 0; // 定义全局变量，存储按键键码值，范围：0~3，0代表没有按键按下

// 外设初始化
void Peripheral_Init(void)
{
    MyRTC_Init();
    Key_Init();
    LED_Init();
    MPU6050_Init();
    DHT11_Init();
}

/*----------------------------------首页时钟-------------------------------------*/
void Show_ui_Clock(void)
{
    MyRTC_ReadTime();

    OLED_Printf(0, 0, OLED_6X8, "%02d:%02d:%02d", MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]);
    OLED_Printf(0, 16, OLED_12X24, "%02d-%02d-%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
    OLED_ShowString(0, 48, "菜单", OLED_8X16);
    OLED_ShowString(96, 48, "设置", OLED_8X16);
}

int clkflag = 1;

int First_Page_Clock(void)
{
    while (1) // 页面函数内部自己循环，只有按下确认键才返回
    {
        KeyNum = Key_GetNum();
        if (KeyNum == 1) // 上一项
        {
            clkflag--;
            if (clkflag <= 0)
                clkflag = 2;
        }
        else if (KeyNum == 2) // 下一项
        {
            clkflag++;
            if (clkflag >= 3)
                clkflag = 1;
        }
        else if (KeyNum == 3) // 确认键
        {
            OLED_Clear();
            OLED_Update();
            return clkflag; // 1=菜单，2=设置
        }

        switch (clkflag)
        {
        case 1:
            Show_ui_Clock();
            OLED_ReverseArea(0, 48, 32, 16); // 反色显示
            OLED_Update();
            break;
        case 2:
            Show_ui_Clock();
            OLED_ReverseArea(96, 48, 32, 16); // 反色显示
            OLED_Update();
            break;
        case 3:
            break;
        default:
            break;
        }
    }
}

/*----------------------------------设置界面-------------------------------------*/

void Show_SettingPage_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(0, 16, "日期时间设置", OLED_8X16);
}

int set_flag = 1;
int SettingPage(void)
{
    while (1) // 页面函数内部自己循环，只有按"返回"才退出
    {
        KeyNum = Key_GetNum();
        uint8_t setflag_temp = 0;
        if (KeyNum == 1) // 上一项
        {
            set_flag--;
            if (set_flag <= 0)
                set_flag = 2;
        }
        else if (KeyNum == 2) // 下一项
        {
            set_flag++;
            if (set_flag >= 3)
                set_flag = 1;
        }
        else if (KeyNum == 3) // 确认键
        {
            OLED_Clear();
            OLED_Update();
            setflag_temp = set_flag;
        }

        if (setflag_temp == 1)
            return 0;
        else if (setflag_temp == 2)
            SetTime(); // 进入日期时间设置

        switch (set_flag)
        {
        case 1:
            Show_SettingPage_UI();
            OLED_ReverseArea(0, 0, 16, 16); // 反色显示
            OLED_Update();
            break;
        case 2:
            Show_SettingPage_UI();
            OLED_ReverseArea(0, 16, 96, 16); // 反色显示
            OLED_Update();
            break;
        }
    }
}

/*----------------------------------滑动菜单界面-------------------------------------*/

uint8_t pre_selection = 0; // 菜单目前present选择数组索引值，0~6，分别对应七个菜单项
uint8_t tag_selection = 0; // 菜单目标target选择菜单数组多音质，0~6，分别对应七个菜单项
uint8_t x_pre = 48;        // 菜单上一次选择的X坐标
uint8_t Speed = 4;         // 菜单移动速度，范围：1~8，数值越大移动越快
uint8_t move_flag = 0;     // 菜单移动标志位，0=不移动，1=开始移动

void Menu_Animation(void)
{
    OLED_Clear(); // OLED清屏
    OLED_ShowImage(42, 10, 44, 44, Frame);
    if (pre_selection < tag_selection) // 向左移动
    {
        x_pre -= Speed;
        if (x_pre <= 0) // 移动到位
        {
            x_pre = 48;
            pre_selection++;
            move_flag = 0; // 停止移动
        }
    }
    else if (pre_selection > tag_selection) // 向右移动
    {
        x_pre += Speed;
        if (x_pre >= 96) // 移动到位
        {
            x_pre = 48;
            pre_selection--;
            move_flag = 0; // 停止移动
        }
    }

    if (pre_selection >= 1) // 左边图标
    {
        OLED_ShowImage(x_pre - 48, 16, 32, 32, Menu_Graph[pre_selection - 1]);
    }
    if (pre_selection >= 2) // 最左边图标
    {
        OLED_ShowImage(x_pre - 96, 16, 32, 32, Menu_Graph[pre_selection - 2]);
    }
    OLED_ShowImage(x_pre, 16, 32, 32, Menu_Graph[pre_selection]);          // 中间图标
    OLED_ShowImage(x_pre + 48, 16, 32, 32, Menu_Graph[pre_selection + 1]); // 右边图标
    OLED_ShowImage(x_pre + 96, 16, 32, 32, Menu_Graph[pre_selection + 2]); // 最右边图标

    OLED_Update();
}

void Set_Selection(uint8_t move_flag, uint8_t Pre_Selection, uint8_t Target_Selection)
{
    if (move_flag)
    {
        pre_selection = Pre_Selection;
        tag_selection = Target_Selection;
    }
    Menu_Animation();
}

void MenuToFunction(void)
{
    for (uint8_t i = 0; i <= 6; i++) // 每个循环向下移8格，六个循环过后完全移出屏幕
    {
        OLED_Clear();

        if (pre_selection >= 1) // 左边图标
        {
            OLED_ShowImage(x_pre - 48, 16 + 8 * i, 32, 32, Menu_Graph[pre_selection - 1]);
        }
        OLED_ShowImage(x_pre, 16 + 8 * i, 32, 32, Menu_Graph[pre_selection]);          // 中间图标
        OLED_ShowImage(x_pre + 48, 16 + 8 * i, 32, 32, Menu_Graph[pre_selection + 1]); // 右边图标

        OLED_Update();
    }
}

int menu_flag = 1; // 菜单标志位，1~3，分别对应三个菜单项
int Menu_Page(void)
{
    move_flag = 1;
    Key_Reset();
    uint8_t Dirflag = 2; // 移动标志位 置1 移动到上一项 右移 置2 移动到下一项 左移
    while (1)            // 页面函数内部自己循环，只有按"返回"才退出
    {
        uint8_t menuflag_temp = 0;
        /* K1/K2：单击走一格；长按1s走一格，之后每1s走一格（KEY_LONG首格，KEY_REPEAT后续） */
        if (Key_Check(KEY_1, KEY_SINGLE) | Key_Check(KEY_1, KEY_LONG) | Key_Check(KEY_1, KEY_REPEAT)) // 上一项
        {
            Dirflag = 1;
            move_flag = 1;
            menu_flag--;
            if (menu_flag <= 0)
                menu_flag = 9;
        }
        else if (Key_Check(KEY_2, KEY_SINGLE) | Key_Check(KEY_2, KEY_LONG) | Key_Check(KEY_2, KEY_REPEAT)) // 下一项
        {
            Dirflag = 2;
            move_flag = 1;
            menu_flag++;
            if (menu_flag >= 10)
                menu_flag = 1;
        }
        else if (Key_Check(KEY_3, KEY_SINGLE)) // 确认键
        {
            OLED_Clear();
            OLED_Update();
            menuflag_temp = menu_flag;
        }
        if (menuflag_temp) Key_Reset();   // 进入子页面前清掉按键标志

        if (menuflag_temp == 1) return 0;                                              // 返回主界面
        else if (menuflag_temp == 2) { MenuToFunction(); StopWatch(); }                // 跳转到秒表界面
        else if (menuflag_temp == 3) { MenuToFunction(); LED(); }                      // 跳转到手电筒界面
        else if (menuflag_temp == 4) { MenuToFunction(); MPU6050(); }                  // 跳转到MPU6050界面
        else if (menuflag_temp == 5) { MenuToFunction(); Game(); }                                               // 预留
        else if (menuflag_temp == 6) { MenuToFunction(); Emoji(); }                    // 跳转到动态表情包界面
        else if (menuflag_temp == 7) { MenuToFunction(); Gradienter(); }               // 跳转到水平仪界面
        else if (menuflag_temp == 8) {  }                   
        else if (menuflag_temp == 9) { MenuToFunction(); DHT11();}                    
        if (menuflag_temp) Key_Reset();   // 从子页面返回后清掉按键标志

        if (menu_flag == 1)
        {
            if (Dirflag == 1)
                Set_Selection(move_flag, 1, 0);
            else if (Dirflag == 2)
                Set_Selection(move_flag, 0, 0);
        }
        // 索引 = 编号 - 1 p=m-1
        else
        {
            if (Dirflag == 1)
                Set_Selection(move_flag, menu_flag, menu_flag - 1); // 右移
            else if (Dirflag == 2)
                Set_Selection(move_flag, menu_flag - 2, menu_flag - 1); // 左移
        }
    }
}
