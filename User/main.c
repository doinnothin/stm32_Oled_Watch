#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "menu.h"
#include "Key.h"
#include "LED.h"
#include "Timer.h"
#include "SetTime.h"
#include "dino.h"
#include "menu_sub.h"

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
		DHT11_Tick();
    Dino_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
