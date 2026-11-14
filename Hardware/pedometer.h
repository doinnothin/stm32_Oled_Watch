#ifndef __PEDOMETER_H
#define __PEDOMETER_H

#include "stm32f10x.h"

/* 初始化（清零步数/状态） */
void Pedometer_Init(void);

/* 在 1ms 定时中断里调用：只做计数，不读 I2C（ISR 要短） */
void Pedometer_Isr1ms(void);

/* 在主循环里反复调用：内部每 20ms 采一次 MPU6050 并做步态检测 */
void Pedometer_Update(void);

uint32_t Pedometer_GetSteps(void);
void     Pedometer_Reset(void);

#endif
