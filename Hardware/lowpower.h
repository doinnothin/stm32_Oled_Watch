#ifndef __LOWPOWER_H
#define __LOWPOWER_H

#include "stm32f10x.h"

/* 极简低功耗：
 *   - 首页 15s 无操作 → OLED 息屏 + STM32 进 STOP
 *   - 按键(EXTI) 或 RTC 闹钟(~10s) 唤醒
 *   - 按键唤醒则亮屏并重置计时；RTC 唤醒则保持息屏继续睡
 */
void    LowPower_Init(void);
void    LowPower_Isr1ms(void);   /* 在 TIM2 的 1ms 中断里调用 */
void    LowPower_Poll(void);     /* 首页循环里反复调用 */
uint8_t LowPower_IsOff(void);    /* 1 = 当前处于息屏/睡眠状态 */

#endif
