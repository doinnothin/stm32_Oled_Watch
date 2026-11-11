/**
  ******************************************************************************
  * @file    aht20.h
  * @brief   AHT20 温湿度传感器驱动（软件 I2C，自带底层，不依赖 si2c.c）
  *
  * 默认引脚和 11-1-AHT20_Oled 例程一致：SCL -> PA11，SDA -> PA12。
  * 如果模块接在别的引脚，只改下面四个宏即可（SCL/SDA 可以在不同端口）。
  * 注意：AHT20 和 MPU6050 的地址不同（0x38 / 0x68），可以挂在同一条 I2C 上，
  *      此时把 SCL/SDA 改成 GPIOB / GPIO_Pin_10 / GPIO_Pin_11，
  *      并把 AHT20_SCL_CLK、AHT20_SDA_CLK 改成 RCC_APB2Periph_GPIOB。
  ******************************************************************************
  */
#ifndef __AHT20_H
#define __AHT20_H

#include "stm32f10x.h"

/* ---------------- 软件 I2C 引脚定义 ---------------- */
#define AHT20_SCL_PORT      GPIOA
#define AHT20_SCL_PIN       GPIO_Pin_11
#define AHT20_SCL_CLK       RCC_APB2Periph_GPIOA

#define AHT20_SDA_PORT      GPIOA
#define AHT20_SDA_PIN       GPIO_Pin_12
#define AHT20_SDA_CLK       RCC_APB2Periph_GPIOA

/* ---------------- 器件参数 ---------------- */
/* 左对齐地址：0x38 << 1 = 0x70（不要写成 0x38） */
#define AHT20_SLAVE_ADDR    0x70

/* ---------------- 返回值 ---------------- */
#define AHT20_OK            0    /* 成功 */
#define AHT20_ERR_ADDR      -1   /* 寻址失败：没接线 / 没上拉 / 地址错 */
#define AHT20_ERR_DATA      -2   /* 命令被拒收，或校准位一直不置位 */
#define AHT20_ERR_BUSY      -3   /* 一直处于忙状态（供电不足或兼容片较慢） */

/* 初始化：上电等 40ms，读状态字，未校准就发 0xBE 0x08 0x00
 * 返回 AHT20_OK 表示传感器在位且已校准 */
int AHT20_Init(void);

/* 读一次温湿度：Temp 单位 ℃，Humi 单位 %RH，都通过指针返回
 * 一次测量约 80ms；失败时不会回写 Temp / Humi */
int AHT20_Read(float *Temp, float *Humi);

#endif
