#include "dht11.h"
#include "delay.h"

/* 每段等待的最大轮数：本工程 Delay_us(1) 一轮约 1.5us，
 * 300 轮 ≈ 450us，和例程里 DelayUs(1)（约 3us）的 100 轮基本一致；
 * 兼容那些应答脉冲偏长的 DHT11 兼容片，没接传感器时也能及时放弃 */
#define DHT11_RETRY_MAX     300

/**
  * 函    数：DHT11切换引脚方向
  * 参    数：mode DHT11_MODE_OUT 输出推挽，DHT11_MODE_IN 输入上拉
  * 返 回 值：无
  * 说    明：输出用于主机主动拉低/释放总线；输入上拉时模块自带 4.7k/10k
  *           上拉则内部上拉无副作用，裸传感器没有外部上拉时也能读到高电平
  */
static void DHT11_Mode(uint8_t mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin   = DHT11_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	if (mode)
	{
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	}
	else
	{
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	}

	GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * 函    数：DHT11发送起始信号
  * 参    数：无
  * 返 回 值：无
  * 说    明：拉低 20ms（Tbe 要求 18~30ms），再拉高 13us（Tgo 要求 10~20us）
  */
static void DHT11_Rst(void)
{
	DHT11_Mode(DHT11_MODE_OUT);

	DHT11_Low;
	Delay_ms(20);          /* Tbe：18~30ms，取 20ms */

	DHT11_High;
	Delay_us(13);        /* Tgo：10~20us，取典型 13us */
}

/**
  * 函    数：DHT11等待从机应答
  * 参    数：无
  * 返 回 值：0 从机有响应，1 没有响应
  * 说    明：先等线被拉低（约 83us），再等它释放（约 87us）；
  *           每段等待都有限次重试，没接传感器时不会卡死
  */
static uint8_t DHT11_Check(void)
{
	uint8_t retry = 0;

	DHT11_Mode(DHT11_MODE_IN);     /* 先放手，再等 */

	while (DHT11_DATA_IN() && retry < DHT11_RETRY_MAX)
	{
		retry++;
		Delay_us(1);
	}
	if (retry >= DHT11_RETRY_MAX) return 1;

	retry = 0;
	while (!DHT11_DATA_IN() && retry < DHT11_RETRY_MAX)
	{
		retry++;
		Delay_us(1);
	}
	if (retry >= DHT11_RETRY_MAX) return 1;

	return 0;
}

/**
  * 函    数：DHT11读取一位数据
  * 参    数：无
  * 返 回 值：读到的位，0 或 1
  * 说    明：等到真实上升沿，再数 40us 看线还在不在高位；
  *           仍在高电平说明这一位是 1，已经掉低说明是 0
  */
static uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0;

	/* 等上一位的 54us 低电平结束 */
	while (DHT11_DATA_IN() && retry < DHT11_RETRY_MAX)
	{
		retry++;
		Delay_us(1);
	}

	/* 等这一位的 54us 低电平结束，此时是上升沿 */
	retry = 0;
	while (!DHT11_DATA_IN() && retry < DHT11_RETRY_MAX)
	{
		retry++;
		Delay_us(1);
	}

	Delay_us(40);                   /* 从上升沿起算 40us */

	if (DHT11_DATA_IN()) return 1; /* 还是高 -> 位 1 */
	return 0;                      /* 已经掉低 -> 位 0 */
}

/**
  * 函    数：DHT11读取一个字节
  * 参    数：无
  * 返 回 值：读到的字节
  * 说    明：DHT11 是高位在前，每读一位先把结果左移再拼进去
  */
static uint8_t DHT11_Read_Byte(void)
{
	uint8_t i;
	uint8_t dat = 0;

	for (i = 0; i < 8; i++)
	{
		dat <<= 1;
		dat |= DHT11_Read_Bit();
	}

	return dat;
}

/**
  * 函    数：DHT11初始化
  * 参    数：无
  * 返 回 值：DHT11_OK 传感器在场，DHT11_NO_RESPONSE 没有应答
  * 说    明：配置 PB14 为输出并释放总线，再做一次握手自检；
  *           上电后 DHT11 需要约 1s 才输出有效数据，建议调用前先 Delay(1000)
  */
uint8_t DHT11_Init(void)
{
	RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);

	DHT11_Mode(DHT11_MODE_OUT);
	DHT11_High;

	/* 上电后 DHT11 需要约 1s 才输出有效数据，这里做一次握手自检 */
	DHT11_Rst();

	return DHT11_Check();
}

/**
  * 函    数：DHT11读取温湿度
  * 参    数：temp 温度，单位 ℃，范围 0~50，整数
  * 参    数：humi 湿度，单位 %RH，范围 5~95，整数
  * 返 回 值：DHT11_OK 成功，DHT11_NO_RESPONSE 无应答，DHT11_CHECKSUM_ERR 校验失败
  * 说    明：buf[0] 湿度整数、buf[1] 湿度小数（恒 0）、buf[2] 温度整数、
  *           buf[3] 温度小数（恒 0）、buf[4] 为前四个字节之和的低 8 位；
  *           返回非 DHT11_OK 时 *temp / *humi 不会被改动
  */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
	uint8_t buf[5] = {0, 0, 0, 0, 0};
	uint8_t i;

	DHT11_Rst();

	if (DHT11_Check() != 0)
	{
		return DHT11_NO_RESPONSE;
	}

	for (i = 0; i < 5; i++)
	{
		buf[i] = DHT11_Read_Byte();
	}

	/* buf[0] 湿度整数, buf[1] 湿度小数(恒 0),
	 * buf[2] 温度整数, buf[3] 温度小数(恒 0), buf[4] 校验和 */
	if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4])
	{
		return DHT11_CHECKSUM_ERR;      /* 原版这里直接 return 0，是它最大的坑 */
	}

	*humi = buf[0];
	*temp = buf[2];

	return DHT11_OK;
}
