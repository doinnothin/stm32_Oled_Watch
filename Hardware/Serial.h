#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

void Serial_Init(uint8_t USART_Name);
void Serial_SendByte(uint8_t USART_Name, uint8_t Byte);
void Serial_SendArray(uint8_t USART_Name, uint8_t *Array, uint16_t Length);
void Serial_SendString(uint8_t USART_Name, char *String);
void Serial_SendNumber(uint8_t USART_Name, uint32_t Number, uint16_t Length);
void Serial_Printf(uint8_t USART_Name, char *format, ...);

/* USART1 接收缓存（本工程主要用于调试，可不用） */
extern char USART1_RX_BUF[1024];
extern uint16_t USART1_RX_LEN;
extern uint8_t USART1_RX_FINISH;

/* USART2 接收缓存（接 ESP32-C3，一行一帧，收到 '\n' 置完成标志） */
extern char USART2_RX_BUF[1024];
extern uint16_t USART2_RX_LEN;
extern uint8_t USART2_RX_FINISH;

#endif
