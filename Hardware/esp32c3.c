#include "esp32c3.h"
#include "Serial.h"
#include <stdio.h>
#include <string.h>

static uint8_t s_valid = 0;
static int     s_code  = -1;
static int     s_temp  = 0;
static char    s_text[16] = "--";

/* 三天预报（今日/明日/后日） */
static uint8_t s_fc_valid = 0;
static int     s_fc_code[3] = {-1, -1, -1};
static int     s_fc_hi[3]   = {0, 0, 0};
static int     s_fc_lo[3]   = {0, 0, 0};
static char    s_fc_text[3][16] = {"", "", ""};

/* 诊断计数：收到的行数、成功解析 $OK / $FC 的次数 */
static uint16_t s_rx_lines = 0;
static uint16_t s_ok_cnt   = 0;
static uint16_t s_fc_cnt   = 0;

/* 心知天气 code（0~38）→ 中文，和开源天气时钟保持一致 */
static const char* CodeToText(int code)
{
    switch (code)
    {
        case 0:  case 1:  case 2:  case 3:  return "晴";
        case 4:  case 5:  case 6:  case 7:  case 8: return "多云";
        case 9:  return "阴";
        case 10: return "阵雨";
        case 11: case 12: return "雷阵雨";
        case 13: return "小雨";
        case 14: return "中雨";
        case 15: return "大雨";
        case 16: return "暴雨";
        case 17: case 18: return "大暴雨";
        case 19: return "冻雨";
        case 20: return "雨夹雪";
        case 21: return "阵雪";
        case 22: return "小雪";
        case 23: return "中雪";
        case 24: return "大雪";
        case 25: return "暴雪";
        case 30: return "雾";
        case 31: return "霾";
        case 32: return "风";
        case 33: return "大风";
        case 37: return "冷";
        case 38: return "热";
        default: return "未知";
    }
}

void ESP32C3_Init(void)
{
    Serial_Init(2);                 /* USART2：PA2=TX / PA3=RX，115200 */

    USART2_RX_LEN = 0;
    USART2_RX_FINISH = 0;
    memset(USART2_RX_BUF, 0, sizeof(USART2_RX_BUF));

    /* 主动告诉 ESP32-C3：上电了，请立刻发一次数据（ESP 端收到 'R' 会刷新） */
    Serial_SendString(2, "R\r\n");
}

void ESP32C3_Poll(void)
{
    if (!USART2_RX_FINISH) return;
    USART2_RX_FINISH = 0;
    s_rx_lines++;

    /* 保证字符串以 '\0' 结尾 */
    if (USART2_RX_LEN > 1023) USART2_RX_LEN = 1023;
    USART2_RX_BUF[USART2_RX_LEN] = '\0';

    char *p = strstr(USART2_RX_BUF, "$OK,");
    if (p != NULL)
    {
        unsigned long utc = 0;
        int code = -1, temp = 0;
        if (sscanf(p + 4, "%lu,%d,%d", &utc, &code, &temp) == 3 && utc > 1600000000UL)
        {
            s_ok_cnt++;
            /* RTC 里存 UTC 秒；MyRTC_ReadTime 显示时会 +8 小时 */
            RTC_SetCounter((uint32_t)utc);
            RTC_WaitForLastTask();

            /* code<0 表示这次没有天气，保留上一次的天气显示，只更新时间 */
            if (code >= 0)
            {
                s_code = code;
                s_temp = temp;
                s_valid = 1;
                strncpy(s_text, CodeToText(code), sizeof(s_text) - 1);
                s_text[sizeof(s_text) - 1] = '\0';
            }
        }
    }

    /* 三天预报行：$FC,c0,h0,l0,c1,h1,l1,c2,h2,l2 */
    {
        char *q = strstr(USART2_RX_BUF, "$FC,");
        if (q != NULL)
        {
            int c0, h0, l0, c1, h1, l1, c2, h2, l2, i;
            if (sscanf(q + 4, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                       &c0, &h0, &l0, &c1, &h1, &l1, &c2, &h2, &l2) == 9)
            {
                s_fc_cnt++;
                s_fc_code[0] = c0; s_fc_hi[0] = h0; s_fc_lo[0] = l0;
                s_fc_code[1] = c1; s_fc_hi[1] = h1; s_fc_lo[1] = l1;
                s_fc_code[2] = c2; s_fc_hi[2] = h2; s_fc_lo[2] = l2;
                for (i = 0; i < 3; i++)
                {
                    strncpy(s_fc_text[i], CodeToText(s_fc_code[i]), sizeof(s_fc_text[i]) - 1);
                    s_fc_text[i][sizeof(s_fc_text[i]) - 1] = '\0';
                }
                s_fc_valid = (c0 >= 0 || c1 >= 0 || c2 >= 0) ? 1 : 0;
            }
        }
        else if (p == NULL && strstr(USART2_RX_BUF, "$ERR") != NULL)
        {
            s_valid = 0;                /* 这次没取到，保留上一次显示 */
        }
    }

    USART2_RX_LEN = 0;
    memset(USART2_RX_BUF, 0, 128);
}

uint8_t ESP32C3_DataValid(void)   { return s_valid; }
char*   ESP32C3_WeatherText(void){ return s_text; }
int     ESP32C3_Temperature(void){ return s_temp; }
int     ESP32C3_WeatherCode(void) { return s_code; }

uint8_t ESP32C3_ForecastValid(void) { return s_fc_valid; }
char*   ESP32C3_ForecastText(int d) { if (d < 0 || d > 2) return ""; return s_fc_text[d]; }
int     ESP32C3_ForecastHigh(int d) { if (d < 0 || d > 2) return 0; return s_fc_hi[d]; }
int     ESP32C3_ForecastLow(int d)  { if (d < 0 || d > 2) return 0; return s_fc_lo[d]; }
int     ESP32C3_ForecastCode(int d) { if (d < 0 || d > 2) return -1; return s_fc_code[d]; }

uint16_t ESP32C3_RxLines(void) { return s_rx_lines; }
uint16_t ESP32C3_OkCount(void) { return s_ok_cnt; }
uint16_t ESP32C3_FcCount(void) { return s_fc_cnt; }
