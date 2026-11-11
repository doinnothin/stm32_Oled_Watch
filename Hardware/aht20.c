#include "aht20.h"
#include "delay.h"

/*==================================================================================
 * 软件 I2C 底层（开漏输出，SCL/SDA 引脚见 aht20.h）
 *================================================================================*/
#define AHT20_SCL_W(v)  GPIO_WriteBit(AHT20_SCL_PORT, AHT20_SCL_PIN, (v) ? Bit_SET : Bit_RESET)
#define AHT20_SDA_W(v)  GPIO_WriteBit(AHT20_SDA_PORT, AHT20_SDA_PIN, (v) ? Bit_SET : Bit_RESET)
#define AHT20_SDA_R()   GPIO_ReadInputDataBit(AHT20_SDA_PORT, AHT20_SDA_PIN)

#define AHT20_HALF_DELAY_US  5      /* 半周期 5us，约 100kHz，杜邦线上比较稳 */

static void AHT20_Delay(void)
{
    Delay_us(AHT20_HALF_DELAY_US);
}

/**
  * 函    数：AHT20 用到的两个引脚初始化为开漏输出并释放总线
  * 说    明：AHT20 模块自带 4.7k~10k 上拉，开漏输出 + 写 1 就是释放总线
  */
static void AHT20_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(AHT20_SCL_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(AHT20_SDA_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = AHT20_SCL_PIN;
    GPIO_Init(AHT20_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = AHT20_SDA_PIN;
    GPIO_Init(AHT20_SDA_PORT, &GPIO_InitStructure);

    AHT20_SDA_W(1);
    AHT20_SCL_W(1);
}

static void AHT20_I2C_Start(void)
{
    AHT20_SDA_W(1);
    AHT20_SCL_W(1);
    AHT20_Delay();
    AHT20_SDA_W(0);          /* SCL 高电平期间拉低 SDA：起始信号 */
    AHT20_Delay();
    AHT20_SCL_W(0);
    AHT20_Delay();
}

static void AHT20_I2C_Stop(void)
{
    AHT20_SDA_W(0);
    AHT20_Delay();
    AHT20_SCL_W(1);
    AHT20_Delay();
    AHT20_SDA_W(1);          /* SCL 高电平期间释放 SDA：停止信号 */
    AHT20_Delay();
}

/* 发送一个字节，返回 0 表示从机应答（ACK），1 表示非应答（NAK） */
static uint8_t AHT20_I2C_SendByte(uint8_t Byte)
{
    uint8_t i, Ack;

    for (i = 0; i < 8; i++)
    {
        AHT20_SDA_W((Byte & (0x80 >> i)) ? 1 : 0);
        AHT20_Delay();
        AHT20_SCL_W(1);
        AHT20_Delay();
        AHT20_SCL_W(0);
    }

    AHT20_SDA_W(1);          /* 释放 SDA，让从机回应答 */
    AHT20_Delay();
    AHT20_SCL_W(1);
    AHT20_Delay();
    Ack = AHT20_SDA_R();
    AHT20_SCL_W(0);
    AHT20_Delay();

    return Ack;
}

/* 接收一个字节；Ack = 1 回 ACK（还要继续读），Ack = 0 回 NAK（最后一个字节） */
static uint8_t AHT20_I2C_ReceiveByte(uint8_t Ack)
{
    uint8_t i, Byte = 0;

    AHT20_SDA_W(1);          /* 主机释放 SDA，交给从机驱动 */
    for (i = 0; i < 8; i++)
    {
        AHT20_SCL_W(1);
        AHT20_Delay();
        if (AHT20_SDA_R())
            Byte |= (0x80 >> i);
        AHT20_SCL_W(0);
        AHT20_Delay();
    }

    AHT20_SDA_W(Ack ? 0 : 1);
    AHT20_Delay();
    AHT20_SCL_W(1);
    AHT20_Delay();
    AHT20_SCL_W(0);
    AHT20_Delay();

    return Byte;
}

/*==================================================================================
 * AHT20 协议层
 *================================================================================*/

/* 发送 3 字节命令；返回 1 成功，0 失败（寻址失败或数据被拒收） */
static uint8_t AHT20_SendCmd(uint8_t Cmd, uint8_t Arg1, uint8_t Arg2)
{
    uint8_t ok = 1;

    AHT20_I2C_Start();
    if (AHT20_I2C_SendByte(AHT20_SLAVE_ADDR) != 0) ok = 0;   /* 写地址 */
    if (ok && AHT20_I2C_SendByte(Cmd) != 0)        ok = 0;
    if (ok && AHT20_I2C_SendByte(Arg1) != 0)       ok = 0;
    if (ok && AHT20_I2C_SendByte(Arg2) != 0)       ok = 0;
    AHT20_I2C_Stop();

    return ok;
}

/* 从 AHT20 读 Len 个字节（无寄存器地址器件，直接读）；返回 1 成功 */
static uint8_t AHT20_ReadBytes(uint8_t *Buf, uint8_t Len)
{
    uint8_t i;

    AHT20_I2C_Start();
    if (AHT20_I2C_SendByte(AHT20_SLAVE_ADDR | 0x01) != 0)   /* 读地址 */
    {
        AHT20_I2C_Stop();
        return 0;
    }

    for (i = 0; i < Len; i++)
    {
        Buf[i] = AHT20_I2C_ReceiveByte(i < (Len - 1));
    }

    AHT20_I2C_Stop();
    return 1;
}

/**
  * 函    数：AHT20初始化
  * 返 回 值：AHT20_OK 成功，其它为错误码
  * 说    明：手册要求上电等 40ms；状态字 bit3=1 表示已校准，为 0 才需要发校准命令
  */
int AHT20_Init(void)
{
    uint8_t status = 0;

    AHT20_I2C_Init();                       /* 配置 SCL/SDA 引脚 */
    Delay_ms(40);                           /* 手册：上电后等待 40ms */

    /* 第1步：直接读 1 字节状态字（AHT20 没有寄存器地址） */
    if (!AHT20_ReadBytes(&status, 1))
    {
        return AHT20_ERR_ADDR;              /* 寻址失败：先查接线和上拉 */
    }

    /* 第2步：bit3 = 0 表示未校准，发 0xBE 0x08 0x00 */
    if ((status & 0x08) == 0)
    {
        if (!AHT20_SendCmd(0xBE, 0x08, 0x00))
        {
            return AHT20_ERR_DATA;
        }

        Delay_ms(10);                       /* 手册：校准约 10ms */

        if (!AHT20_ReadBytes(&status, 1))
        {
            return AHT20_ERR_ADDR;
        }

        if ((status & 0x08) == 0)
        {
            return AHT20_ERR_DATA;          /* 校准没成功：多为供电不足 */
        }
    }

    return AHT20_OK;
}

/**
  * 函    数：AHT20读取温湿度
  * 参    数：Temp 温度，单位 ℃，通过它返回
  * 参    数：Humi 湿度，单位 %RH，通过它返回
  * 返 回 值：AHT20_OK 成功，其它为错误码（失败时不修改 Temp / Humi）
  * 说    明：湿度 = raw_h * 100 / 2^20；温度 = raw_t * 200 / 2^20 - 50
  *           buf[0] 状态字，buf[1..5]：湿度20位 + 温度20位
  */
int AHT20_Read(float *Temp, float *Humi)
{
    uint8_t buf[6];
    uint8_t retry;
    uint32_t raw_h, raw_t;

    /* 第1步：触发测量 0xAC 0x33 0x00 */
    if (!AHT20_SendCmd(0xAC, 0x33, 0x00))
    {
        return AHT20_ERR_DATA;
    }

    Delay_ms(80);                           /* 手册：一次测量 75~80ms */

    /* 第2步：读 6 字节，buf[0] 的 bit7 是忙闲位 */
    for (retry = 0; retry < 3; retry++)
    {
        if (!AHT20_ReadBytes(buf, 6))
        {
            return AHT20_ERR_ADDR;
        }

        if ((buf[0] & 0x80) == 0)
        {
            break;                          /* bit7 = 0，数据有效 */
        }

        Delay_ms(20);                       /* 还在忙，等 20ms 重试 */
    }

    if (retry == 3)
    {
        return AHT20_ERR_BUSY;
    }

    /* 第3步：拼 20 位原始值 */
    raw_h = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    raw_t = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    /* 第4步：按手册公式换算 */
    *Humi = (float)raw_h * 100.0f / 1048576.0f;         /* 2^20 = 1048576 */
    *Temp = (float)raw_t * 200.0f / 1048576.0f - 50.0f;

    return AHT20_OK;
}
