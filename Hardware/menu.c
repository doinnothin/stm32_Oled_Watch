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
#include "aht20.h"
#include "pedometer.h"
#include "lowpower.h"
#include "dino.h"
#include "menu_sub.h"
#include "esp32c3.h"
#include "weather.h"

uint8_t KeyNum = 0; // 定义全局变量，存储按键键码值，范围：0~3，0代表没有按键按下

// 外设初始化
void Peripheral_Init(void)
{
    MyRTC_Init();
    Key_Init();
    LED_Init();
    MPU6050_Init();
    AHT20_Init();   /* 温湿度改用 AHT20（PA11/PA12），DHT11 停用 */
    Pedometer_Init();   /* 计步模块初始化 */
}

/*----------------------------------首页时钟-------------------------------------*/

/* 由年月日算星期（0=周日），Sakamoto 公式 */
static const char* Week_ASCII(int y, int m, int d)
{
    static const char* names[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int w = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
    if (w < 0) w += 7;
    return names[w];
}

void Show_ui_Clock(void)
{
    ESP32C3_Poll();      /* 处理 ESP32-C3 发来的时间/天气（收到就写 RTC） */
    Pedometer_Update();  /* 每 20ms 采一次 MPU6050 做计步 */
    MyRTC_ReadTime();

    /* 第 1 行：日期 + 星期（6x8 小字） */
    OLED_Printf(0, 0, OLED_6X8, "%04d-%02d-%02d %s",
                MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2],
                Week_ASCII(MyRTC_Time[0], MyRTC_Time[1], MyRTC_Time[2]));

    /* 第 2 行：时间（12x24 大字） */
    OLED_Printf(10, 8, OLED_12X24, "%02d:%02d:%02d",
                MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);

    /* 第 3 行：天气 + 温度 + 步数 */
    OLED_ClearArea(0, 32, 128, 16);
    if (ESP32C3_DataValid())
    {
        char *wt = ESP32C3_WeatherText();
        char *pp;
        int wn = 0, tx;
        for (pp = wt; *pp != '\0'; pp++)
        {
            if ((*pp & 0xC0) != 0x80) wn++;   /* 统计 UTF-8 字符数，中文占 16px */
        }
        OLED_ShowString(0, 32, wt, OLED_8X16);
        tx = wn * 16 + 2;
        OLED_Printf(tx, 32, OLED_8X16, "%dC", ESP32C3_Temperature());
    }
    else
    {
        OLED_ShowString(0, 32, "天气 --", OLED_8X16);
    }
    /* 右侧：步数（"步" + 4 位数字） */
    {
        unsigned int st = (unsigned int)Pedometer_GetSteps();
        if (st > 9999) st = 9999;
        OLED_ShowString(80, 32, "步", OLED_8X16);
        OLED_Printf(96, 32, OLED_8X16, "%4u", st);
    }

    /* 第 4 行：菜单 / 设置（保持原有导航与反色高亮） */
    OLED_ShowString(0, 48, "菜单", OLED_8X16);
    OLED_ShowString(96, 48, "设置", OLED_8X16);
}

int clkflag = 1;

int First_Page_Clock(void)
{
    while (1) // 页面函数内部自己循环，只有按下确认键才返回
    {
        LowPower_Poll();                 /* 首页无操作自动息屏 + STOP 休眠 */
        if (LowPower_IsOff()) continue;  /* 息屏状态：不刷新显示 */

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
        ESP32C3_Poll();   /* 各页面都要解析串口：虚拟按键/天气更新/回ACK */
        if (LowPower_Tick()) continue;
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
        ESP32C3_Poll();   /* 各页面都要解析串口：虚拟按键/天气更新/回ACK */
        if (LowPower_Tick()) continue;
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
        else if(Key_Check(KEY_3, KEY_LONG)) 
        {
            OLED_Clear();
            return 0;
        }
        if (menuflag_temp) Key_Reset();   // 进入子页面前清掉按键标志

        if (menuflag_temp == 1) return 0;                                              // 返回主界面
        else if (menuflag_temp == 2) { MenuToFunction(); StopWatch(); }                // 跳转到秒表界面
        else if (menuflag_temp == 3) { MenuToFunction(); LED(); }                      // 跳转到手电筒界面
        else if (menuflag_temp == 4) { MenuToFunction(); MPU6050(); }                  // 跳转到MPU6050界面
        else if (menuflag_temp == 5) { MenuToFunction(); Game(); }                                               // 预留
        else if (menuflag_temp == 6) { MenuToFunction(); Emoji(); }                    // 跳转到动态表情包界面
        else if (menuflag_temp == 7) { MenuToFunction(); Gradienter(); }               // 跳转到水平仪界面
        else if (menuflag_temp == 8) { MenuToFunction(); Weather(); }                   // 跳转到天气界面（ESP32-C3 数据）
        else if (menuflag_temp == 9) { MenuToFunction(); AHT20_Page();}              // 温湿度（AHT20）
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
