#include "stm32f10x.h"                  // Device header
#include "Key.h"

#define KEY_PRESSED				1
#define KEY_UNPRESSED			0

#define KEY_TIME_DOUBLE			200		//双击判定窗口(ms)
#define KEY_TIME_LONG			1000	//长按判定时间(ms)
#define KEY_TIME_REPEAT			500	//长按后重复间隔(ms)

uint8_t Key_Flag[KEY_COUNT];

/**
  * 函    数：按键初始化（PB12/PB13/PB15 上拉输入，按下为低电平）
  */
void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_12 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * 函    数：读取单个按键的当前状态（1按下/0松开）
  */
static uint8_t Key_Read(uint8_t n)
{
	if (n == KEY_1)
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0) return KEY_PRESSED;
	}
	else if (n == KEY_2)
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0) return KEY_PRESSED;
	}
	else
	{
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 0) return KEY_PRESSED;
	}
	return KEY_UNPRESSED;
}

/**
  * 函    数：检查按键事件标志，读到后自动清除（KEY_HOLD 除外）
  * 参    数：n 按键编号 KEY_1~KEY_3；Flag KEY_SINGLE/KEY_DOUBLE/KEY_LONG/KEY_REPEAT/KEY_HOLD...
  */
uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
	if (Key_Flag[n] & Flag)
	{
		if (Flag != KEY_HOLD)
		{
			Key_Flag[n] &= ~Flag;
		}
		return 1;
	}
	return 0;
}

/**
  * 函    数：清空所有按键标志（切换页面时调用，避免旧事件带入新页面）
  */
void Key_Reset(void)
{
	uint8_t i;
	for (i = 0; i < KEY_COUNT; i ++)
	{
		Key_Flag[i] = 0;
	}
}

/**
  * 函    数：获取单击键码（兼容旧写法，返回 1~3，0 表示无）
  * 说    明：单击在松开后 KEY_TIME_DOUBLE 窗口内确认，可直接接到原来的 Key_GetNum() 位置
  */
uint8_t Key_GetNum(void)
{
	uint8_t i;
	for (i = 0; i < KEY_COUNT; i ++)
	{
		if (Key_Flag[i] & KEY_SINGLE)
		{
			Key_Flag[i] &= ~KEY_SINGLE;
			return i + 1;
		}
	}
	return 0;
}

/**
  * 函    数：按键扫描，需在 1ms 定时中断里调用
  * 说    明：产生 HOLD/DOWN/UP/SINGLE/DOUBLE/LONG/REPEAT 事件
  */
void Key_Tick(void)
{
	static uint8_t Count, i;
	static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
	static uint8_t S[KEY_COUNT];
	static uint16_t Time[KEY_COUNT];

	for (i = 0; i < KEY_COUNT; i ++)
	{
		if (Time[i] > 0)
		{
			Time[i] --;
		}
	}

	Count ++;
	if (Count >= 20)
	{
		Count = 0;

		for (i = 0; i < KEY_COUNT; i ++)
		{
			PrevState[i] = CurrState[i];
			CurrState[i] = Key_Read(i);

			if (CurrState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KEY_HOLD;
			}
			else
			{
				Key_Flag[i] &= ~KEY_HOLD;
			}

			if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
			{
				Key_Flag[i] |= KEY_DOWN;
			}

			if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KEY_UP;
			}

			if (S[i] == 0)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Time[i] = KEY_TIME_LONG;
					S[i] = 1;
				}
			}
			else if (S[i] == 1)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					Time[i] = KEY_TIME_DOUBLE;
					S[i] = 2;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_Flag[i] |= KEY_LONG;
					S[i] = 4;
				}
			}
			else if (S[i] == 2)
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Key_Flag[i] |= KEY_DOUBLE;
					S[i] = 3;
				}
				else if (Time[i] == 0)
				{
					Key_Flag[i] |= KEY_SINGLE;
					S[i] = 0;
				}
			}
			else if (S[i] == 3)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;
				}
			}
			else if (S[i] == 4)
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;
				}
				else if (Time[i] == 0)
				{
					Time[i] = KEY_TIME_REPEAT;
					Key_Flag[i] |= KEY_REPEAT;
					S[i] = 4;
				}
			}
		}
	}
}
