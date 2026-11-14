#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "menu.h"
#include "Key.h"
#include "LED.h"
#include "Timer.h"
#include "SetTime.h"
#include "dino.h"
#include "bird.h"
#include "menu_sub.h"
#include "esp32c3.h"
#include "pedometer.h"
#include "lowpower.h"

/**
  * 坐标轴定义：
  * 左上角为(0, 0)点
  * 横向向右为X轴，取值范围：0~127
  * 纵向向下为Y轴，取值范围：0~63
  * 
  *       0             X轴           127 
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  *  Y轴 |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  * 
  */

int main(void)
{
	/*OLED初始化*/
	OLED_Init();
	OLED_Clear();
	Peripheral_Init(); //外设初始化
	Timer_Init(); //定时器初始化
	ESP32C3_Init(); //初始化与 ESP32-C3 的串口（USART2），并请求一次数据
	LowPower_Init(); //低功耗：按键/RTC 唤醒配置

	int clkflag1;

	while (1)
	{
      clkflag1 = First_Page_Clock();		//首页内部自己循环，按确认键才返回
      if (clkflag1 == 1) {Menu_Page();}				//菜单：1-2章还没有菜单页，后续章节补上
      else if (clkflag1 == 2) {SettingPage();}	//设置页内部自己循环，按返回才退出
	}
}

//定时器中断函数，1ms中断
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key_Tick();
		StopWatch_Tick();
		Pedometer_Isr1ms();   /* 计步 1ms 计时 */
		LowPower_Isr1ms();    /* 低功耗 1ms 计时 */
    Dino_Tick();
		Bird_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
