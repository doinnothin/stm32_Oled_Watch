/* Minimal host shim for stm32f10x.h so the real OLED/UI sources compile on PC.
 * Only what the UI + OLED drawing code touches is provided; all hardware
 * side effects are no-ops (the framebuffer is real, the panel is not). */
#ifndef __SHIM_STM32F10X_H
#define __SHIM_STM32F10X_H
#include <stdint.h>
#include <stddef.h>

typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { Bit_RESET = 0, Bit_SET = 1 } BitAction;

typedef struct { int _dummy; } GPIO_TypeDef;
#define GPIOA ((GPIO_TypeDef *)0)
#define GPIOB ((GPIO_TypeDef *)0)
#define GPIOC ((GPIO_TypeDef *)0)

#define GPIO_Pin_0  ((uint16_t)0x0001)
#define GPIO_Pin_1  ((uint16_t)0x0002)
#define GPIO_Pin_2  ((uint16_t)0x0004)
#define GPIO_Pin_3  ((uint16_t)0x0008)
#define GPIO_Pin_4  ((uint16_t)0x0010)
#define GPIO_Pin_5  ((uint16_t)0x0020)
#define GPIO_Pin_6  ((uint16_t)0x0040)
#define GPIO_Pin_7  ((uint16_t)0x0080)
#define GPIO_Pin_8  ((uint16_t)0x0100)
#define GPIO_Pin_9  ((uint16_t)0x0200)
#define GPIO_Pin_10 ((uint16_t)0x0400)
#define GPIO_Pin_11 ((uint16_t)0x0800)
#define GPIO_Pin_12 ((uint16_t)0x1000)
#define GPIO_Pin_13 ((uint16_t)0x2000)
#define GPIO_Pin_14 ((uint16_t)0x4000)
#define GPIO_Pin_15 ((uint16_t)0x8000)

typedef struct {
    uint16_t GPIO_Pin;
    int GPIO_Speed;
    int GPIO_Mode;
} GPIO_InitTypeDef;

#define GPIO_Speed_50MHz  3
#define GPIO_Mode_Out_OD  0x10
#define GPIO_Mode_Out_PP  0x00
#define GPIO_Mode_IPU     0x08
#define GPIO_Mode_IN_FLOATING 0x04

#define RCC_APB2Periph_GPIOA ((uint32_t)0x00000004)
#define RCC_APB2Periph_GPIOB ((uint32_t)0x00000008)
#define RCC_APB2Periph_GPIOC ((uint32_t)0x00000010)

static inline void GPIO_WriteBit(GPIO_TypeDef *p, uint16_t pin, BitAction v) { (void)p; (void)pin; (void)v; }
static inline void GPIO_SetBits(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; }
static inline void GPIO_ResetBits(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; }
static inline uint8_t GPIO_ReadInputDataBit(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return 0; }
static inline uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return 0; }
static inline void GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *g) { (void)p; (void)g; }
static inline void RCC_APB2PeriphClockCmd(uint32_t periph, FunctionalState e) { (void)periph; (void)e; }
static inline void RCC_APB1PeriphClockCmd(uint32_t periph, FunctionalState e) { (void)periph; (void)e; }

#endif
