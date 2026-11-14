#include "weather.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Key.h"
#include "esp32c3.h"
#include "delay.h"
#include <string.h>

extern uint8_t KeyNum;   /* 真正定义在 menu.c */

/* 两个页面的画面快照：用于左右滑动动画（8页 x 128列 = 1KB/页） */
static uint8_t s_snap[2][8][128];

static char* DayLabel(int i)
{
    static char* d[3] = {"今日", "明日", "后日"};
    if (i < 0 || i > 2) return "";
    return d[i];
}

/* ================ 页面0：天气列表（今日/明日/后日） ================ */
void Show_Weather_UI(void)
{
    int i;

    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(20, 2, "天气", OLED_8X16);
    if (ESP32C3_DataValid())
    {
        OLED_Printf(96, 2, OLED_8X16, "%dC", ESP32C3_Temperature());
    }

    if (ESP32C3_ForecastValid())
    {
        for (i = 0; i < 3; i++)
        {
            int y = 16 + i * 16;
            OLED_ShowString(0, y, DayLabel(i), OLED_8X16);
            OLED_ShowString(34, y, ESP32C3_ForecastText(i), OLED_8X16);
            OLED_Printf(84, y, OLED_8X16, "%d/%d", ESP32C3_ForecastHigh(i), ESP32C3_ForecastLow(i));
        }
    }
    else
    {
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
            OLED_Printf(0, 44, OLED_6X8, "RX=%u OK=%u FC=%u",
                        ESP32C3_RxLines(), ESP32C3_OkCount(), ESP32C3_FcCount());
        }
    }
}

/* ================ 页面1：温度趋势曲线（多天预报 高/低温） ================ */
void Show_Trend_UI(void)
{
    int n = ESP32C3_TrendCount();
    int i, tmin = 999, tmax = -999;
    const int y0 = 20, y1 = 54, x0 = 8, x1 = 120;

    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ShowString(20, 2, "温度", OLED_8X16);     /* 标题 */

    if (n <= 0)
    {
        OLED_ShowString(24, 28, "No data", OLED_8X16);
        return;
    }

    for (i = 0; i < n; i++)
    {
        int h = ESP32C3_TrendHigh(i);
        int l = ESP32C3_TrendLow(i);
        if (h > tmax) tmax = h;
        if (l < tmin) tmin = l;
    }
    if (tmax <= tmin) tmax = tmin + 1;

    OLED_DrawLine(x0, y1, x1, y1);                 /* 底部横轴 */

    {
        int prevX = -1, prevH = 0, prevL = 0;
        for (i = 0; i < n; i++)
        {
            int x = (n == 1) ? 64 : x0 + i * (x1 - x0) / (n - 1);
            int yh = y1 - (ESP32C3_TrendHigh(i) - tmin) * (y1 - y0) / (tmax - tmin);
            int yl = y1 - (ESP32C3_TrendLow(i) - tmin) * (y1 - y0) / (tmax - tmin);
            if (prevX >= 0)
            {
                OLED_DrawLine(prevX, prevH, x, yh);   /* 高温折线 */
                OLED_DrawLine(prevX, prevL, x, yl);   /* 低温折线 */
            }
            OLED_DrawCircle(x, yh, 1, 1);             /* 高/低温点 */
            OLED_DrawCircle(x, yl, 1, 1);
            prevX = x; prevH = yh; prevL = yl;
        }
    }

    /* 右上角显示温度范围，左下角显示天数 */
    OLED_Printf(88, 13, OLED_6X8, "%d~%d", tmin, tmax);
    OLED_Printf(4, 56, OLED_6X8, "%dd", n);
}

/* ================ 把某页渲染进快照（不显示） ================ */
static void RenderSnapshots(void)
{
    OLED_Clear();
    Show_Weather_UI();
    OLED_ReverseArea(0, 0, 16, 16);
    memcpy(s_snap[0], OLED_DisplayBuf, sizeof(s_snap[0]));

    OLED_Clear();
    Show_Trend_UI();
    OLED_ReverseArea(0, 0, 16, 16);
    memcpy(s_snap[1], OLED_DisplayBuf, sizeof(s_snap[1]));
}

static void ShowSnapshot(int p)
{
    memcpy(OLED_DisplayBuf, s_snap[p], sizeof(s_snap[0]));
    OLED_Update();
}

/* 滑动动画：dir>0 新页从右侧进入（左移）；dir<0 从左侧进入（右移） */
static void SlideTo(int from, int to, int dir)
{
    int o, x, pg;

    for (o = 0; o <= 128; o += 8)
    {
        for (x = 0; x < 128; x++)
        {
            int sx = (dir > 0) ? (x + o) : (x - o);
            for (pg = 0; pg < 8; pg++)
            {
                if (dir > 0)
                    OLED_DisplayBuf[pg][x] = (sx < 128) ? s_snap[from][pg][sx] : s_snap[to][pg][sx - 128];
                else
                    OLED_DisplayBuf[pg][x] = (sx >= 0) ? s_snap[from][pg][sx] : s_snap[to][pg][128 + sx];
            }
        }
        OLED_Update();
        Delay_ms(8);
    }
}

/* ================ 天气页主循环：KEY2 下一页 / KEY1 上一页 / KEY3 返回 ================ */
int Weather(void)
{
    int page = 0;
    uint16_t lastOk = 0xFFFF, lastFc = 0xFFFF;
    uint8_t lastTr = 0xFF;

    RenderSnapshots();
    ShowSnapshot(0);
    lastOk = ESP32C3_OkCount();
    lastFc = ESP32C3_FcCount();
    lastTr = ESP32C3_TrendCount();

    while (1)
    {
        KeyNum = Key_GetNum();

        if (KeyNum == 3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        else if (KeyNum == 2 && page == 0)      /* 下一页：左滑 */
        {
            SlideTo(0, 1, 1);
            page = 1;
        }
        else if (KeyNum == 1 && page == 1)      /* 上一页：右滑 */
        {
            SlideTo(1, 0, -1);
            page = 0;
        }

        ESP32C3_Poll();

        /* 数据有更新就重渲染快照并刷新当前页 */
        if (ESP32C3_OkCount() != lastOk || ESP32C3_FcCount() != lastFc || ESP32C3_TrendCount() != lastTr)
        {
            lastOk = ESP32C3_OkCount();
            lastFc = ESP32C3_FcCount();
            lastTr = ESP32C3_TrendCount();
            RenderSnapshots();
            ShowSnapshot(page);
        }

        Delay_ms(20);
    }
}
