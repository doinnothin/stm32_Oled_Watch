#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

#define KEY_COUNT				3

#define KEY_1					0
#define KEY_2					1
#define KEY_3					2

#define KEY_HOLD				0x01
#define KEY_DOWN				0x02
#define KEY_UP					0x04
#define KEY_SINGLE				0x08
#define KEY_DOUBLE				0x10
#define KEY_LONG				0x20
#define KEY_REPEAT				0x40

void Key_Init(void);
uint8_t Key_GetNum(void);
uint8_t Key_Check(uint8_t n, uint8_t Flag);
void Key_Reset(void);
void Key_Tick(void);
void Key_Inject(uint8_t n, uint8_t Flag);   /* 注入一个按键事件（手机网页虚拟按键用） */

#endif
