#include "lowpower.h"
#include "OLED.h"
#include "Key.h"
#include "delay.h"

/* ===== 可调参数 ===== */
#define LP_IDLE_MS     15000u   /* 首页无操作 15s 后息屏休眠 */
#define LP_RTC_WAKE_S  10u      /* STOP 期间 RTC 每 10s 醒一次（只醒一下就继续睡） */

static volatile uint32_t s_ms = 0;
static uint32_t s_last_active = 0;
static uint8_t  s_off = 0;
static uint8_t  s_enabled = 1;

void LowPower_Isr1ms(void) { s_ms++; }

uint8_t LowPower_IsOff(void) { return s_off; }

uint8_t LowPower_Tick(void) { LowPower_Poll(); return s_off; }

void LowPower_SetEnabled(uint8_t en)
{
    s_enabled = en;
    if (!en) { s_off = 0; s_last_active = s_ms; }
}

/* 按键 PB12/13/15 按下为低 */
static uint8_t KeyPressed(void)
{
    return (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0) ||
           (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0) ||
           (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 0);
}

void LowPower_Init(void)
{
    EXTI_InitTypeDef e;
    NVIC_InitTypeDef n;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* 按键引脚 -> EXTI 下降沿唤醒 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource15);

    e.EXTI_Line = EXTI_Line12 | EXTI_Line13 | EXTI_Line15;
    e.EXTI_Mode = EXTI_Mode_Interrupt;
    e.EXTI_Trigger = EXTI_Trigger_Falling;
    e.EXTI_LineCmd = ENABLE;
    EXTI_Init(&e);
    EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13 | EXTI_Line15);

    n.NVIC_IRQChannel = EXTI15_10_IRQn;
    n.NVIC_IRQChannelCmd = ENABLE;
    n.NVIC_IRQChannelPreemptionPriority = 3;
    n.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&n);

    /* RTC 闹钟 -> EXTI Line17 唤醒 */
    e.EXTI_Line = EXTI_Line17;
    e.EXTI_Mode = EXTI_Mode_Interrupt;
    e.EXTI_Trigger = EXTI_Trigger_Rising;
    e.EXTI_LineCmd = ENABLE;
    EXTI_Init(&e);
    EXTI_ClearITPendingBit(EXTI_Line17);

    n.NVIC_IRQChannel = RTCAlarm_IRQn;
    n.NVIC_IRQChannelCmd = ENABLE;
    n.NVIC_IRQChannelPreemptionPriority = 3;
    n.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&n);

    s_ms = 0;
    s_last_active = 0;
    s_off = 0;
}

/* 进入 STOP，直到 按键 或 RTC 闹钟 唤醒 */
static void EnterSleep(void)
{
    Key_Reset();                             /* 睡前清空按键事件，避免残留 */
    OLED_WriteCommand(0xAE);                 /* 关显示 */

    RTC_SetAlarm(RTC_GetCounter() + LP_RTC_WAKE_S);
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();

    EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13 | EXTI_Line15 | EXTI_Line17);
    PWR_ClearFlag(PWR_FLAG_WU);

    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);

    /* 唤醒后：STOP 会停掉时钟，需恢复 HSE/PLL */
    SystemInit();
    PWR_ClearFlag(PWR_FLAG_WU);

    RTC_ITConfig(RTC_IT_ALR, DISABLE);
    RTC_WaitForLastTask();
    EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13 | EXTI_Line15 | EXTI_Line17);
}

void LowPower_Poll(void)
{
    if (!s_enabled) { s_off = 0; s_last_active = s_ms; return; }

    if (KeyPressed())
    {
        s_off = 0;
        s_last_active = s_ms;
    }

    if (!s_off)
    {
        if ((uint32_t)(s_ms - s_last_active) >= LP_IDLE_MS)
            s_off = 1;                        /* 到时 → 进入息屏休眠 */
        else
            return;
    }

    /* 息屏休眠（按键或 RTC 会把它唤醒） */
    EnterSleep();

    if (KeyPressed())
    {
        uint32_t t0 = s_ms;
        s_off = 0;                            /* 按键唤醒：亮屏 */
        s_last_active = s_ms;
        OLED_WriteCommand(0xAF);              /* 开显示（SSD1306 GDDRAM 内容仍在，直接恢复） */

        /* 吞掉这次"唤醒按键"：等松手（最多 800ms）后清掉按键事件，
           这样当前页面不会把唤醒的这一次按键再当成一次操作 */
        while (KeyPressed() && (uint32_t)(s_ms - t0) < 800u) { }
        Key_Reset();
        s_last_active = s_ms;
    }
    /* 否则是 RTC 唤醒：保持 s_off=1，下一页继续睡 */
}

/* EXTI15_10：按键唤醒，清标志即可 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line12) || EXTI_GetITStatus(EXTI_Line13) || EXTI_GetITStatus(EXTI_Line15))
        EXTI_ClearITPendingBit(EXTI_Line12 | EXTI_Line13 | EXTI_Line15);
}

/* RTC 闹钟中断：只清标志 */
void RTCAlarm_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line17))
        EXTI_ClearITPendingBit(EXTI_Line17);
    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_WaitForLastTask();
}
