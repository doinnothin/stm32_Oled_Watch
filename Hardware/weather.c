#include "weather.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Key.h"
#include "esp32c3.h"

extern uint8_t KeyNum;   /* 真正定义在 menu.c */

static char* DayLabel(int i)
{
    static char* d[3] = {"今日", "明日", "后日"};
    if (i < 0 || i > 2) return "";
    return d[i];
}

/* 画天气页：顶部 返回图标 + "天气" + 当前温度；下面三行三天预报 */
void Show_Weather_UI(void)
{
    int i;

    OLED_ShowImage(0, 0, 16, 16, Return);          /* 返回图标 */
    OLED_ShowString(20, 2, "天气", OLED_8X16);      /* 标题 */
    if (ESP32C3_DataValid())
    {
        OLED_Printf(96, 2, OLED_8X16, "%dC", ESP32C3_Temperature());   /* 当前温度 */
    }

    if (ESP32C3_ForecastValid())
    {
        for (i = 0; i < 3; i++)
        {
            int y = 16 + i * 16;
            OLED_ShowString(0, y, DayLabel(i), OLED_8X16);                                  /* 今日/明日/后日 */
            OLED_ShowString(34, y, ESP32C3_ForecastText(i), OLED_8X16);                     /* 天气文字 */
            OLED_Printf(84, y, OLED_8X16, "%d/%d", ESP32C3_ForecastHigh(i), ESP32C3_ForecastLow(i)); /* 高/低温 */
        }
    }
    else
    {
        /* 还没收到预报：退回显示当前天气大字 */
        char *t = ESP32C3_WeatherText();
        if (ESP32C3_DataValid() && t && t[0] != '\0')
        {
            int n = 0;
            char *p;
            for (p = t; *p != '\0'; p++)
            {
                if ((*p & 0xC0) != 0x80) n++;
            }
            {
                int wx = (128 - n * 16) / 2;
                if (wx < 0) wx = 0;
                OLED_ShowString(wx, 24, t, OLED_8X16);
            }
        }
        else
        {
            OLED_ShowString(0, 22, "No data", OLED_8X16);
            /* 诊断：RX=收到的行数  OK=成功解析$OK次数  FC=成功解析$FC次数 */
            OLED_Printf(0, 44, OLED_6X8, "RX=%u OK=%u FC=%u",
                        ESP32C3_RxLines(), ESP32C3_OkCount(), ESP32C3_FcCount());
        }
    }
}

/* 天气页面按键循环：确认键(KEY3) 返回菜单 */
int Weather(void)
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
        ESP32C3_Poll();          /* 该页面内仍持续接收 C3 数据 */
        OLED_Clear();
        Show_Weather_UI();
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Update();
    }
}
