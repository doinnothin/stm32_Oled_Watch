#include "menu_sub.h"
#include "OLED_Data.h"
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
#include "dino.h"
#include "bird.h"
#include "esp32c3.h"

extern uint8_t KeyNum; // 声明外部全局变量（真正定义在 menu.c），此处不能再赋值，否则会重复定义

/*----------------------------------秒表界面-------------------------------------*/

uint8_t hour, min, sec;
// 显示秒表界面的函数
void Show_StopWatch_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_Printf(32, 20, OLED_8X16, "%02d:%02d:%02d", hour, min, sec);
    OLED_ShowString(8, 44, "开始", OLED_8X16);
    OLED_ShowString(48, 44, "停止", OLED_8X16);
    OLED_ShowString(88, 44, "清除", OLED_8X16);
}

uint8_t start_timing_flag; // 1：开始，0：停止

// 显示计时器的函数
void StopWatch_Tick(void)
{
    static uint16_t Count;
    Count++;
    if (Count >= 1000)
    {
        Count = 0;
        if (start_timing_flag == 1)
        {
            sec++;
            if (sec >= 60)
            {
                sec = 0;
                min++;
                if (min >= 60)
                {
                    min = 0;
                    hour++;
                    if (hour > 99)
                        hour = 0;
                }
            }
        }
    }
}

uint8_t stopwatch_flag = 1;
// 控制光标在秒表界面移动的函数
int StopWatch(void)
{
    while (1)
    {
        KeyNum = Key_GetNum();
        uint8_t stopwatch_flag_temp = 0;
        if (KeyNum == 1) // 上一项
        {
            stopwatch_flag--;
            if (stopwatch_flag <= 0)
                stopwatch_flag = 4;
        }
        else if (KeyNum == 2) // 下一项
        {
            stopwatch_flag++;
            if (stopwatch_flag >= 5)
                stopwatch_flag = 1;
        }
        else if (KeyNum == 3) // 确认
        {
            OLED_Clear();
            OLED_Update();
            stopwatch_flag_temp = stopwatch_flag;
        }

        if (stopwatch_flag_temp == 1)
        {
            return 0;
        }

        switch (stopwatch_flag)
        {
        case 1:
            Show_StopWatch_UI();
            OLED_ReverseArea(0, 0, 16, 16);
            OLED_Update();
            break;

        case 2:
            Show_StopWatch_UI();
            start_timing_flag = 1;
            OLED_ReverseArea(8, 44, 32, 16);
            OLED_Update();
            break;

        case 3:
            Show_StopWatch_UI();
            start_timing_flag = 0;
            OLED_ReverseArea(48, 44, 32, 16);
            OLED_Update();
            break;

        case 4:
            Show_StopWatch_UI();
            start_timing_flag = 0;
            hour = min = sec = 0;
            OLED_ReverseArea(88, 44, 32, 16);
            OLED_Update();
            break;
        }
    }
}

/*----------------------------------手电筒界面------------------------------------*/
// 显示手电筒界面的函数
void Show_LED_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(20, 20, "OFF", OLED_12X24);
    OLED_ShowString(72, 20, "ON", OLED_12X24);
}

uint8_t led_flag = 1;
// 控制光标在手电筒界面移动的函数
int LED(void)
{
    while (1)
    {
        KeyNum = Key_GetNum();
        uint8_t led_flag_temp = 0;
        if (KeyNum == 1) // 上一项
        {
            led_flag--;
            if (led_flag <= 0)
                led_flag = 3;
        }
        else if (KeyNum == 2) // 下一项
        {
            led_flag++;
            if (led_flag >= 4)
                led_flag = 1;
        }
        else if (KeyNum == 3) // 确认
        {
            OLED_Clear();
            OLED_Update();
            led_flag_temp = led_flag;
        }

        if (led_flag_temp == 1)
        {
            return 0;
        }

        switch (led_flag)
        {
        case 1:
            Show_LED_UI();
            OLED_ReverseArea(0, 0, 16, 16);
            OLED_Update();
            break;

        case 2:
            Show_LED_UI();
            LED_OFF();
            OLED_ReverseArea(20, 20, 36, 24);
            OLED_Update();
            break;

        case 3:
            Show_LED_UI();
            LED_ON();
            OLED_ReverseArea(72, 20, 24, 24);
            OLED_Update();
            break;
        }
    }
}

/*----------------------------------MPU6050界面------------------------------------*/

int16_t ax, ay, az, gx, gy, gz; // mpu6050测的三轴加速度计和陀螺仪角速度数据
float roll_g, pitch_g, yaw_g;   // 陀螺仪角速度解算的欧拉角
float roll_a, pitch_a, yaw_a;   // 加速度计加速度解算的欧拉角
float Roll, Pitch, Yaw;         // 互补滤波后的欧拉角
float a = 0.9;                  // 互补滤波系数
float Delta_t = 0.005;          // 采样周期 200hz
double pi = 3.1415927;
float pitch_offset = 0, roll_offset = 0;              // 加速度角零点（校准写入）
float gx_offset = 0, gy_offset = 0, gz_offset = 0;    // 陀螺零偏（校准写入）

void MPU6050_Calculation(void)
{
    Delay_ms(5); // 5ms采样一次
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

    // 陀螺仪：先扣零偏，再按 16.4 LSB/(°/s) 换算成角速度
    roll_g  = Roll  + ((float)gx - gx_offset) / 16.4f * Delta_t;
    pitch_g = Pitch + ((float)gy - gy_offset) / 16.4f * Delta_t;
    yaw_g   = Yaw   + ((float)gz - gz_offset) / 16.4f * Delta_t;

    // 加速度计解算欧拉角，并减去校准零点
    pitch_a = atan2((-1) * ax, az) * 180 / pi - pitch_offset;
    roll_a  = atan2(ay, az)     * 180 / pi - roll_offset;

    // 互补滤波融合
    Roll  = a * roll_g  + (1 - a) * roll_a;
    Pitch = a * pitch_g + (1 - a) * pitch_a;
    Yaw   = yaw_g;      // 无磁力计：仅陀螺积分，只做相对转角，会缓慢漂移
}

/**
  * 函    数：MPU6050 校准（放平静止后长按K2触发）
  * 说    明：连采 50 次求平均，把“当前加速度角”记为 0 点，
  *           同时把“静止时陀螺的读数”记为陀螺零偏，之后解算全部减掉。
  */
void MPU6050_Calibrate(void)
{
    uint8_t i;
    float ps = 0, rs = 0, gxs = 0, gys = 0, gzs = 0;

    for (i = 0; i < 50; i ++)
    {
        MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
        ps  += atan2((-1) * ax, az) * 180 / pi;
        rs  += atan2(ay, az)     * 180 / pi;
        gxs += gx;  gys += gy;  gzs += gz;
        Delay_ms(10);
    }

    pitch_offset = ps / 50;
    roll_offset  = rs / 50;
    gx_offset    = gxs / 50;
    gy_offset    = gys / 50;
    gz_offset    = gzs / 50;

    Roll = Pitch = Yaw = 0;     // 融合角复位，避免校准后跳变
    roll_a = pitch_a = 0;
}

// 显示MPU6050界面的UI
void Show_MPU6050_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_Printf(0, 16, OLED_8X16, "Roll: %.2f", Roll);
    OLED_Printf(0, 32, OLED_8X16, "Pitch:%.2f", Pitch);
    OLED_Printf(0, 48, OLED_8X16, "Yaw:  %.2f", Yaw);
}

// MPU6050界面：长按K2校准，按K3返回
int MPU6050(void)
{
    Key_Reset();
    while (1)
    {
        if (Key_Check(KEY_2, KEY_LONG)) MPU6050_Calibrate();   // 长按K2校准

        if (Key_Check(KEY_3, KEY_SINGLE))                      // K3返回
        {
            OLED_Clear();
            OLED_Update();
            Key_Reset();
            return 0;
        }

        OLED_Clear();
        MPU6050_Calculation();
        Show_MPU6050_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
    }
}

/*----------------------------------水平仪 / 竖直仪 / 斜坡仪-------------------------------------*/

//显示水平仪（气泡进内圈即水平，右侧显示两轴角与判断）
void Show_Gradienter_UI(void)
{
    float tilt, dx, dy;
    MPU6050_Calculation();
    tilt = sqrt(Roll * Roll + Pitch * Pitch);

    OLED_DrawCircle(32, 32, 24, 0);   // 外圈
    OLED_DrawCircle(32, 32, 7, 0);    // 内圈目标区

    dx = Roll * 0.7f;
    dy = Pitch * 0.7f;
    if (dx > 17) dx = 17; else if (dx < -17) dx = -17;
    if (dy > 17) dy = 17; else if (dy < -17) dy = -17;
    OLED_DrawCircle((int16_t)(32 - dx), (int16_t)(32 + dy), 4, 1);

    OLED_Printf(62, 8,  OLED_8X16, "X%+6.1f", Roll);
    OLED_Printf(62, 24, OLED_8X16, "Y%+6.1f", Pitch);
    if (tilt < 1.5f)
    {
        OLED_ShowString(62, 42, "水平", OLED_8X16);
        OLED_ReverseArea(62, 42, 32, 16);   // 反色突出“水平”
    }
    else
    {
        OLED_ShowString(62, 42, "倾斜", OLED_8X16);
    }
}

//显示竖直仪（气泡居中即竖直，右侧显示偏差与判断）
void Show_Vertical_UI(void)
{
    float tilt, dev;
    int16_t y;
    MPU6050_Calculation();
    tilt = sqrt(Pitch * Pitch + Roll * Roll);   //合成倾角：0平放 90竖直
    dev = 90 - tilt;                            //离竖直方向的偏差

    OLED_DrawRectangle(30, 6, 22, 52, 0);       //竖直气泡管
    OLED_DrawLine(32, 32, 50, 32);              //中心基准线
    y = 32 + (int16_t)(dev / 2.5f);
    if (y > 52) y = 52; else if (y < 12) y = 12;
    OLED_DrawCircle(41, y, 4, 1);

    OLED_ShowString(56, 8, "偏差", OLED_8X16);
    OLED_Printf(56, 26, OLED_8X16, "%5.1f", dev < 0 ? -dev : dev);
    if (fabs(dev) < 1.5f)
    {
        OLED_ShowString(56, 42, "垂直", OLED_8X16);
        OLED_ReverseArea(56, 42, 32, 16);
    }
    else
    {
        OLED_ShowString(56, 42, "倾斜", OLED_8X16);
    }
}

//显示斜坡仪（斜坡示意 + 角度 + 坡度百分比）
void Show_Slope_UI(void)
{
    float tilt, draw, g;
    double rad;
    int16_t x0 = 4, y0 = 58, x1, y1;
    MPU6050_Calculation();
    tilt = sqrt(Pitch * Pitch + Roll * Roll);

    draw = tilt; if (draw > 60) draw = 60;
    rad = (double)draw * pi / 180.0;
    x1 = (int16_t)(x0 + 54.0 * cos(rad));
    y1 = (int16_t)(y0 - 54.0 * sin(rad));

    OLED_DrawLine(x0, y0, x0 + 54, y0);   //底边
    OLED_DrawLine(x0, y0, x1, y1);        //斜边
    OLED_DrawLine(x1, y1, x0 + 54, y0);   //竖边

    OLED_ShowString(66, 8,  "角度", OLED_8X16);
    OLED_Printf(66, 26, OLED_8X16, "%5.1f", tilt);
    g = tilt; if (g > 85) g = 85;
    OLED_Printf(66, 46, OLED_6X8,  "%.0f%%", tan((double)g * pi / 180.0) * 100);
}

//三个测量页：长按K2校准，按K3返回子菜单
int Gradienter_Level(void)
{
    Key_Reset();
    while (1)
    {
        if (Key_Check(KEY_2, KEY_LONG)) MPU6050_Calibrate();
        if (Key_Check(KEY_3, KEY_SINGLE)) { OLED_Clear(); OLED_Update(); Key_Reset(); return 0; }
        OLED_Clear();
        Show_Gradienter_UI();
        OLED_Update();
    }
}

int Gradienter_Vertical(void)
{
    Key_Reset();
    while (1)
    {
        if (Key_Check(KEY_2, KEY_LONG)) MPU6050_Calibrate();
        if (Key_Check(KEY_3, KEY_SINGLE)) { OLED_Clear(); OLED_Update(); Key_Reset(); return 0; }
        OLED_Clear();
        Show_Vertical_UI();
        OLED_Update();
    }
}

int Gradienter_Slope(void)
{
    Key_Reset();
    while (1)
    {
        if (Key_Check(KEY_2, KEY_LONG)) MPU6050_Calibrate();
        if (Key_Check(KEY_3, KEY_SINGLE)) { OLED_Clear(); OLED_Update(); Key_Reset(); return 0; }
        OLED_Clear();
        Show_Slope_UI();
        OLED_Update();
    }
}

//显示水平仪子菜单
void Show_GradienterMenu_UI(void)
{
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(0, 16, "水平仪", OLED_8X16);
    OLED_ShowString(0, 32, "竖直仪", OLED_8X16);
    OLED_ShowString(0, 48, "斜坡仪", OLED_8X16);
}

uint8_t grad_flag = 1;
//水平仪子菜单：竖直排列，用按键选择、确认进入
int Gradienter(void)
{
    while (1)
    {
        KeyNum = Key_GetNum();
        uint8_t grad_flag_temp = 0;
        if (KeyNum == 1)      //上一项
        {
            grad_flag--;
            if (grad_flag <= 0) grad_flag = 4;
        }
        else if (KeyNum == 2) //下一项
        {
            grad_flag++;
            if (grad_flag >= 5) grad_flag = 1;
        }
        else if (KeyNum == 3) //确认
        {
            OLED_Clear();
            OLED_Update();
            grad_flag_temp = grad_flag;
        }

        if (grad_flag_temp == 1) return 0;              //返回主菜单
        else if (grad_flag_temp == 2) { Gradienter_Level(); }
        else if (grad_flag_temp == 3) { Gradienter_Vertical(); }
        else if (grad_flag_temp == 4) { Gradienter_Slope(); }

        switch (grad_flag)
        {
        case 1:
            Show_GradienterMenu_UI();
            OLED_ReverseArea(0, 0, 16, 16);
            OLED_Update();
            break;
        case 2:
            Show_GradienterMenu_UI();
            OLED_ReverseArea(0, 16, 48, 16);
            OLED_Update();
            break;
        case 3:
            Show_GradienterMenu_UI();
            OLED_ReverseArea(0, 32, 48, 16);
            OLED_Update();
            break;
        case 4:
            Show_GradienterMenu_UI();
            OLED_ReverseArea(0, 48, 48, 16);
            OLED_Update();
            break;
        }
    }
}

/*----------------------------------动态表情包-------------------------------------*/

//显示动态表情包
void Show_Emoji_UI(void)
{
	/*闭眼*/
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,6-i,1);//左眼
		OLED_DrawEllipse(88,32,6,6-i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}
	
	/*睁眼*/
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,4+i,1);//左眼
		OLED_DrawEllipse(88,32,6,4+i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}
	
	Delay_ms(500);
	
}

//用按键控制退出动态表情包界面的函数
int Emoji(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		
		Show_Emoji_UI();
		OLED_Update();
	}
}


/*----------------------------------温湿度（AHT20 替换 DHT11）-------------------------------------*/
/* 说明：已用 AHT20 全面替代 DHT11（DHT11 驱动已从工程移除）。
 *       AHT20 走软件 I2C，引脚见 aht20.h：SCL=PA11 / SDA=PA12。
 */
static float aht_temp = 0.0f, aht_humi = 0.0f;
static int   aht_status = -99;      /* 最近一次 AHT20_Read 的返回码 */

/* 显示温湿度（保留 1 位小数，不用 %f，避免浮点 printf 开销） */
void Show_AHT20_UI(void)
{
    int t10, h10, ti, td, hi, hd;

    OLED_ShowImage(0, 0, 16, 16, Return);

    if (aht_status == AHT20_OK)
    {
        t10 = (int)(aht_temp * 10.0f);
        h10 = (int)(aht_humi * 10.0f);
        ti = t10 / 10; td = t10 % 10; if (td < 0) td = -td;
        hi = h10 / 10; hd = h10 % 10; if (hd < 0) hd = -hd;

        OLED_ShowString(0, 16, "Temp:", OLED_8X16);
        if (t10 < 0)
            OLED_Printf(40, 16, OLED_8X16, "-%d.%d C", -ti, td);
        else
            OLED_Printf(40, 16, OLED_8X16, "%d.%d C", ti, td);

        OLED_ShowString(0, 32, "Humi:", OLED_8X16);
        OLED_Printf(40, 32, OLED_8X16, "%d.%d %%", hi, hd);
    }
    else
    {
        OLED_ShowString(0, 16, "AHT20 Error", OLED_8X16);
        OLED_Printf(0, 32, OLED_8X16, "Code:%d", aht_status);
        OLED_ShowString(0, 48, "Check PA11/PA12", OLED_6X8);
    }
}

/* 温湿度页面：进入后每 ~0.6s 读一次 AHT20；K3 返回 */
int AHT20_Page(void)
{
    while (1)
    {
        KeyNum = Key_GetNum();
        if (KeyNum == 3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }

        aht_status = AHT20_Read(&aht_temp, &aht_humi);   /* 约 80ms */

        OLED_Clear();
        Show_AHT20_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();

        Delay_ms(500);   /* 约每 0.6s 刷新一次 */
    }
}

/*----------------------------------游戏-------------------------------------*/

void Show_Game_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"谷歌小恐龙",OLED_8X16);
	OLED_ShowString(0,32,"像素小鸟",OLED_8X16);
}

uint8_t game_flag=1;
//控制光标在游戏选择界面移动的函数
int Game(void)
{
	while(1)
	{
		/* 在游戏菜单里长按K2校准一次，进入游戏就不用每局都校准了 */
		if (Key_Check(KEY_2, KEY_LONG))
		{
			OLED_Clear();
			OLED_ShowString(12,24,"Calibrating",OLED_8X16);
			OLED_Update();
			MPU6050_Calibrate();
			Key_Reset();
		}

		KeyNum=Key_GetNum();
		uint8_t game_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			game_flag--;
			if(game_flag<=0)game_flag=3;
		}
		else if(KeyNum==2)//下一项
		{
			game_flag++;
			if(game_flag>=4)game_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			game_flag_temp=game_flag;
		}
		
		if(game_flag_temp==1)   {return 0;}
		else if(game_flag_temp==2)  {DinoGame_Pos_Init(); DinoGame_Animation();}
		else if(game_flag_temp==3)  {BirdGame_Animation();}
		
		switch(game_flag)
		{
			case 1:
				Show_Game_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_Game_UI();
				OLED_ReverseArea(0,16,80,16);
				OLED_Update();
				break;

			case 3:
				Show_Game_UI();
				OLED_ReverseArea(0,32,64,16);
				OLED_Update();
				break;
		}
	}
}
