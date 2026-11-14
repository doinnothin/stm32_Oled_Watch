#ifndef __ESP32C3_H
#define __ESP32C3_H

#include "stm32f10x.h"

/* 初始化与 ESP32-C3 的串口（USART2，PA2=TX / PA3=RX，115200） */
void ESP32C3_Init(void);

/* 在首页循环里反复调用：收到一整行就解析、写 RTC、更新天气 */
void ESP32C3_Poll(void);

/* 查询解析结果 */
uint8_t ESP32C3_DataValid(void);      /* 1=已收到有效数据 */
char*   ESP32C3_WeatherText(void);    /* UTF-8 中文，如 "晴"、"多云" */
int     ESP32C3_Temperature(void);    /* 温度，单位 ℃ */
int     ESP32C3_WeatherCode(void);    /* 心知天气代码 0~38 */

/* 三天预报（0=今日, 1=明日, 2=后日） */
uint8_t ESP32C3_ForecastValid(void);
char*   ESP32C3_ForecastText(int day);   /* UTF-8 中文天气 */
int     ESP32C3_ForecastHigh(int day);
int     ESP32C3_ForecastLow(int day);
int     ESP32C3_ForecastCode(int day);

/* 诊断计数 */
uint16_t ESP32C3_RxLines(void);
uint16_t ESP32C3_OkCount(void);
uint16_t ESP32C3_FcCount(void);

#endif
