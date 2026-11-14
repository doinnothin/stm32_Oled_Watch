#include "esp32c3.h"
#include "Serial.h"
#include <stdio.h>
#include <stdlib.h>
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
static uint16_t s_crc_ok   = 0;
static uint16_t s_crc_err  = 0;

/* 多天预报（趋势曲线用），最多 15 天（心知付费上限） */
#define TREND_MAX 15
static uint8_t s_tr_count = 0;
static int     s_tr_code[TREND_MAX];
static int     s_tr_hi[TREND_MAX];
static int     s_tr_lo[TREND_MAX];
static char    s_tr_text[TREND_MAX][16];

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

/* CRC-16/CCITT-FALSE：poly=0x1021, init=0xFFFF，不反转、不异或输出（与 ESP32 端一致） */
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    uint8_t b;
    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

static uint8_t hexVal(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

/* 处理一整行（不含 \r\n）：先校验 CRC，再按 "$XX," 前缀分发 */
static void ProcessLine(char *line, uint16_t len)
{
    char *star;
    uint16_t crcpos, rx, calc;
    uint8_t i;

    if (len < 8) return;

    star = (char *)memchr(line, '*', len);
    if (star == NULL) { s_crc_err++; return; }
    crcpos = (uint16_t)(star - line);
    if ((uint32_t)crcpos + 5 > len) { s_crc_err++; return; }   /* '*' + 4 位十六进制 */

    rx = 0;
    for (i = 0; i < 4; i++)
    {
        uint8_t hv = hexVal(line[crcpos + 1 + i]);
        if (hv == 0xFF) { s_crc_err++; return; }
        rx = (uint16_t)((rx << 4) | hv);
    }
    calc = crc16_ccitt((const uint8_t *)line, crcpos);
    if (rx != calc) { s_crc_err++; return; }
    s_crc_ok++;

    line[crcpos] = '\0';                       /* 截掉 "*HHHH"，后面只看内容 */

    if (strncmp(line, "$OK,", 4) == 0)
    {
        unsigned long utc = 0;
        int code = -1, temp = 0;
        if (sscanf(line + 4, "%lu,%d,%d", &utc, &code, &temp) == 3 && utc > 1600000000UL)
        {
            s_ok_cnt++;
            RTC_SetCounter((uint32_t)utc);
            RTC_WaitForLastTask();
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
    else if (strncmp(line, "$FC,", 4) == 0)
    {
        int c0, h0, l0, c1, h1, l1, c2, h2, l2, k;
        if (sscanf(line + 4, "%d,%d,%d,%d,%d,%d,%d,%d,%d",
                   &c0, &h0, &l0, &c1, &h1, &l1, &c2, &h2, &l2) == 9)
        {
            s_fc_cnt++;
            s_fc_code[0] = c0; s_fc_hi[0] = h0; s_fc_lo[0] = l0;
            s_fc_code[1] = c1; s_fc_hi[1] = h1; s_fc_lo[1] = l1;
            s_fc_code[2] = c2; s_fc_hi[2] = h2; s_fc_lo[2] = l2;
            for (k = 0; k < 3; k++)
            {
                strncpy(s_fc_text[k], CodeToText(s_fc_code[k]), sizeof(s_fc_text[k]) - 1);
                s_fc_text[k][sizeof(s_fc_text[k]) - 1] = '\0';
            }
            s_fc_valid = (c0 >= 0 || c1 >= 0 || c2 >= 0) ? 1 : 0;
        }
    }
    else if (strncmp(line, "$FD,", 4) == 0)
    {
        char *tok = strtok(line + 4, ",");
        if (tok != NULL)
        {
            int n = atoi(tok), k = 0;
            while (k < TREND_MAX)
            {
                char *tc = strtok(NULL, ","); if (tc == NULL) break;
                char *th = strtok(NULL, ","); if (th == NULL) break;
                char *tl = strtok(NULL, ","); if (tl == NULL) break;
                s_tr_code[k] = atoi(tc);
                s_tr_hi[k]   = atoi(th);
                s_tr_lo[k]   = atoi(tl);
                strncpy(s_tr_text[k], CodeToText(s_tr_code[k]), sizeof(s_tr_text[k]) - 1);
                s_tr_text[k][sizeof(s_tr_text[k]) - 1] = '\0';
                k++;
            }
            s_tr_count = (uint8_t)k;
            (void)n;
        }
    }
    else if (strncmp(line, "$ERR", 4) == 0)
    {
        s_valid = 0;                            /* 这次没取到，保留上次显示 */
    }
}

void ESP32C3_Poll(void)
{
    char *s, *nl;

    if (!USART2_RX_FINISH) return;
    USART2_RX_FINISH = 0;

    if (USART2_RX_LEN > 1023) USART2_RX_LEN = 1023;
    USART2_RX_BUF[USART2_RX_LEN] = '\0';

    /* 一帧可能包含多行（$OK\r\n$FC\r\n$FD\r\n）：逐行拆分、逐行校验解析 */
    s = USART2_RX_BUF;
    while (*s != '\0')
    {
        uint16_t ll;
        nl = s;
        while (*nl != '\0' && *nl != '\n') nl++;
        ll = (uint16_t)(nl - s);
        while (ll > 0 && s[ll - 1] == '\r') ll--;
        if (ll > 0)
        {
            s_rx_lines++;
            ProcessLine(s, ll);
        }
        if (*nl == '\n') nl++;
        s = nl;
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

/* 多天预报（趋势）取值：0=今天 */
uint8_t ESP32C3_TrendCount(void) { return s_tr_count; }
char*   ESP32C3_TrendText(int d) { if (d < 0 || d >= (int)s_tr_count) return ""; return s_tr_text[d]; }
int     ESP32C3_TrendHigh(int d) { if (d < 0 || d >= (int)s_tr_count) return 0; return s_tr_hi[d]; }
int     ESP32C3_TrendLow(int d)  { if (d < 0 || d >= (int)s_tr_count) return 0; return s_tr_lo[d]; }
int     ESP32C3_TrendCode(int d) { if (d < 0 || d >= (int)s_tr_count) return -1; return s_tr_code[d]; }
