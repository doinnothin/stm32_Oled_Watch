/**
  ******************************************************************************
  * @file    dht11.h
  * @brief   DHT11 温湿度传感器驱动（单总线，DATA -> PB14）
  *
  * 引脚由下面的宏指定，换引脚只改这里即可。
  * 使用前先 Delay(1000) 让传感器上电稳定，再调用 DHT11_Init()；
  * 每次读取间隔必须 >= 1s（建议 2s），否则会读到旧数据。
  ******************************************************************************
  */
#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/* ---------------- 引脚定义：DATA -> PB14 ---------------- */
#define DHT11_GPIO_PORT     GPIOB
#define DHT11_GPIO_PIN      GPIO_Pin_14
#define DHT11_GPIO_CLK      RCC_APB2Periph_GPIOB

#define DHT11_MODE_OUT      1
#define DHT11_MODE_IN       0

#define DHT11_DATA_IN()     GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)
#define DHT11_Low           GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)
#define DHT11_High          GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

/* ---------------- 返回值 ---------------- */
#define DHT11_OK            0    /* 读到了，校验通过 */
#define DHT11_NO_RESPONSE   1    /* 起始信号后没有任何应答（没接/线断/上电不足） */
#define DHT11_CHECKSUM_ERR  2    /* 收到了 5 个字节，但校验和不对 */

/* 初始化：配置 PB14 并做一次握手自检
 * 返回 DHT11_OK 表示传感器在场 */
uint8_t DHT11_Init(void);

/* 读一次：temp / humi 都是整数（DHT11 只有整数分辨率）
 * humi 单位 %RH，范围 5~95；temp 单位 ℃，范围 0~50
 * 返回 DHT11_OK / DHT11_NO_RESPONSE / DHT11_CHECKSUM_ERR
 * 注意：返回非 0 时 *temp / *humi 不会被改动，调用方应保留上次的值 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

#endif
