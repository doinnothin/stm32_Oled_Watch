#include "stm32f10x.h"
#include "dino.h"
#include "OLED.h"
#include "key.h"
#include "stdlib.h"
#include "math.h"
#include "Delay.h"

uint16_t Score = 0;
void Show_Score(void)
{
    OLED_ShowNum(98, 0, Score, 5, OLED_6X8);
}

uint16_t Ground_index;
void Show_Ground(void)
{
    for (uint8_t i = 0; i < 128; i++)
    {
        OLED_DisplayBuf[7][i] = Ground[(Ground_index + i) % 256];
    }
}

uint8_t barrier_flag;
uint8_t barrier_index;
struct Object_Position barrier;
void Show_Barrier(void)
{
    OLED_ShowImage(127 - barrier_index, 44, 16, 18, Barrier[barrier_flag]);

    barrier.minX = 127 - barrier_index;
    barrier.maxX = 143 - barrier_index;
    barrier.minY = 44;
    barrier.maxY = 62;
}



uint8_t cloud_index;
void Show_Cloud(void)
{
    OLED_ShowImage(127-cloud_index, 9, 16, 8, Cloud);
}

uint8_t dino_jump_flag = 0; //0：奔跑 1：跳跃
uint16_t jump_t;
uint8_t Jump_Pos;
extern double pi;
struct Object_Position dino;
void Show_Dino(void)
{
    if (Key_Check(KEY_1, KEY_DOWN)) dino_jump_flag = 1;   //按下沿立即起跳
    Jump_Pos = 28 * sin((float)(pi * jump_t / 1000));

    if(dino_jump_flag==0)
	{
        if (cloud_index % 2 == 0)   OLED_ShowImage(0, 44, 16, 18, Dino[0]);
        else                        OLED_ShowImage(0, 44, 16, 18, Dino[1]);
    }
    else
    {
        OLED_ShowImage(0, 44 - Jump_Pos, 16, 18, Dino[2]);
    }

    dino.minX=0;
	dino.maxX=16;
	dino.minY=44-Jump_Pos;
	dino.maxY=62-Jump_Pos;   
}

int isColliding(struct Object_Position *a, struct Object_Position *b)
{
    if ((a->maxX > b->minX) && (a->minX < b->maxX) && (a->maxY > b->minY) && (a->minY < b->maxY))
    {
        OLED_Clear();
        OLED_ShowString(28, 24, "Game Over", OLED_8X16);
        OLED_Update();
        Delay_s(1);
        OLED_Clear();
        OLED_Update();
        return 1;
    }
    return 0;
}

int DinoGame_Animation(void)
{
    Key_Reset();
    while (1)
    {
        int return_flag;
        OLED_Clear();
        Show_Score();
        Show_Ground();
        Show_Barrier();
        Show_Cloud();
        Show_Dino();
        OLED_Update();
        return_flag = isColliding(&dino, &barrier);
        if (return_flag == 1)
        {
            return 0;
        }
    }
}


void Dino_Tick(void)
{
    static uint8_t score_count, ground_count, cloud_count;
    score_count++;
    ground_count++;
    cloud_count++;
    
    if (score_count >= 100)
    {
        score_count = 0;
        Score++;
    }
    if (ground_count >= 20)
    {
        ground_count = 0;
        Ground_index++;
        barrier_index++;
        if (Ground_index >= 256)  Ground_index = 0;
        if (barrier_index >= 144) //128+16
        {
            barrier_index = 0;
            barrier_flag = rand() % 3;
        }
    }

    if (cloud_count >= 50)
    {
        cloud_count = 0;
        cloud_index++;
        if (cloud_index >= 200)  cloud_index = 0;
    }

    if (dino_jump_flag == 1) //跳跃计时
    {
        jump_t++;
        if (jump_t >= 1000)
        {
            jump_t = 0;
            dino_jump_flag = 0;
        }
    }
}

void DinoGame_Pos_Init(void)
{
    Score = Ground_index = barrier_index = cloud_index = Jump_Pos = 0;
}
