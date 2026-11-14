#include "stm32f10x.h"      /* Device header */
#include <stdio.h>
#include <stdarg.h>
#include "Serial.h"

char USART1_RX_BUF[1024] = {0};
uint16_t USART1_RX_LEN = 0;
uint8_t USART1_RX_FINISH = 0;

char USART2_RX_BUF[1024] = {0};
uint16_t USART2_RX_LEN = 0;
uint8_t USART2_RX_FINISH = 0;

/*
 * 串口初始化
 *   USART_Name = 1：USART1，PA9=TX / PA10=RX，9600（调试用，可选）
 *   USART_Name = 2：USART2，PA2=TX / PA3=RX，115200（接 ESP32-C3）
 * 两路都开启 RXNE 接收中断，收到 '\n' 时把 *_RX_FINISH 置 1，表示收到一整行。
 */
void Serial_Init(uint8_t USART_Name)
{
    if (USART_Name == 1)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;      /* 复用推挽输出 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;        /* 上拉输入 */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        USART_InitTypeDef USART_InitStructure;
        USART_InitStructure.USART_BaudRate = 9600;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_Init(USART1, &USART_InitStructure);

        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_Init(&NVIC_InitStructure);

        USART_Cmd(USART1, ENABLE);
    }
    else if (USART_Name == 2)
    {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;      /* PA2 = TX */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;        /* PA3 = RX */
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        USART_InitTypeDef USART_InitStructure;
        USART_InitStructure.USART_BaudRate = 115200;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_Init(USART2, &USART_InitStructure);

        USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
        NVIC_Init(&NVIC_InitStructure);

        USART_Cmd(USART2, ENABLE);
    }
}

void Serial_SendByte(uint8_t USART_Name, uint8_t Byte)
{
    if (USART_Name == 1)
    {
        USART_SendData(USART1, Byte);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    else if (USART_Name == 2)
    {
        USART_SendData(USART2, Byte);
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
}

void Serial_SendArray(uint8_t USART_Name, uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(USART_Name, Array[i]);
    }
}

void Serial_SendString(uint8_t USART_Name, char *String)
{
    uint16_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        Serial_SendByte(USART_Name, String[i]);
    }
}

static uint32_t Serial_Pow(uint32_t x, uint32_t y)
{
    uint32_t Result = 1;
    while (y--) Result *= x;
    return Result;
}

void Serial_SendNumber(uint8_t USART_Name, uint32_t Number, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i++)
    {
        Serial_SendByte(USART_Name, Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void Serial_Printf(uint8_t USART_Name, char *format, ...)
{
    char String[128];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    Serial_SendString(USART_Name, String);
}

/* USART1 接收中断：调试口，一般不用 */
void USART1_IRQHandler(void)
{
    uint8_t temp;
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        temp = (uint8_t)USART_ReceiveData(USART1);
        USART1_RX_BUF[USART1_RX_LEN++] = temp;
        if (USART1_RX_LEN >= 1023) USART1_RX_LEN = 0;
        if (temp == '\n') USART1_RX_FINISH = 1;
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/* USART2 接收中断：接 ESP32-C3 */
void USART2_IRQHandler(void)
{
    uint8_t temp;
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        temp = (uint8_t)USART_ReceiveData(USART2);
        USART2_RX_BUF[USART2_RX_LEN++] = temp;
        if (USART2_RX_LEN >= 1023) USART2_RX_LEN = 0;
        if (temp == '\n') USART2_RX_FINISH = 1;
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
