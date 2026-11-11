#include "stm32f10x.h"
#include "bird.h"
#include "OLED.h"
#include "Key.h"
#include "delay.h"
#include "stdlib.h"

/* 依赖 menu_sub.c 里的姿态解算 */
void MPU6050_Calculation(void);
extern float Pitch;          // 互补滤波后的 pitch 角，用于体感控制上下

#define BIRD_X        22      // 小鸟固定横坐标
#define BIRD_W        16
#define BIRD_H        14
#define TER_LEN       256     // 地形循环长度（循环缓冲，避免越界）
#define SCROLL_SPEED  2       // 每帧地形左移像素
#define PITCH_K       1.6f    // pitch(度) -> 纵向像素 的比例
#define WING_PERIOD   6       // 每6帧换一次翅膀

/* 小鸟像素图两帧（16x14，2页×16列 = 32字节）：0=翅膀朝上 1=翅膀朝下 */
static const uint8_t Bird_img[2][32] = {
    {
        0x18,0x3C,0x7E,0xFE,0xFE,0xFE,0xFC,0xF8,0xF8,0x38,0x38,0xF0,0xE0,0xC0,0x80,0x00,
        0x00,0x00,0x00,0x07,0x0F,0x1F,0x3F,0x3F,0x3F,0x3F,0x3F,0x1F,0x0F,0x07,0x03,0x01
    },
    {
        0x00,0x00,0x00,0xC0,0xE0,0xF0,0xF8,0xF8,0xF8,0x38,0x38,0xF0,0xE0,0xC0,0x80,0x00,
        0x18,0x3C,0x3E,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x1F,0x0F,0x07,0x03,0x01
    }
};

/* 洞窟地形：每列记录“上壁高度”和“下壁起始 y”，随机游走生成，滚动循环 */
static uint8_t  ter_top[TER_LEN];
static uint8_t  ter_bot[TER_LEN];
static uint16_t ter_base;
static uint16_t Bird_Score;
static uint16_t score_acc;

/* 重新开始一局：生成一段随机洞窟（上下壁平滑起伏，通道至少 26px） */
void BirdGame_Reset(void)
{
    int i, top, bot, vt, vb;
    static uint16_t seed = 0;
    seed += 1234;
    srand(seed);

    top = 12; bot = 44; vt = 0; vb = 0;
    for (i = 0; i < TER_LEN; i ++)
    {
        /* 用“速度”做随机游走，边界才会平滑起伏，不会一格一格的锯齿 */
        vt += (int)(rand() % 3) - 1; if (vt > 1) vt = 1; else if (vt < -1) vt = -1;
        vb += (int)(rand() % 3) - 1; if (vb > 1) vb = 1; else if (vb < -1) vb = -1;
        top += vt;
        bot += vb;

        if (top < 4)  { top = 4;  vt = 0; }
        if (top > 22) { top = 22; vt = 0; }
        if (bot > 60) { bot = 60; vb = 0; }
        if (bot < 44) { bot = 44; vb = 0; }
        if (bot - top < 26)                         // 保证通道宽度
        {
            bot = top + 26;
            if (bot > 62) { bot = 62; top = 62 - 26; }
        }
        ter_top[i] = top;
        ter_bot[i] = bot;
    }

    ter_base    = 0;
    Bird_Score  = 0;
    score_acc   = 0;
}

/* 直接往显存里画洞窟（上下白墙、中间黑通道），比逐个矩形快 */
static void Cave_Draw(void)
{
    uint8_t x;
    uint16_t idx;
    uint8_t t, b;
    int y;

    for (x = 0; x < 128; x ++)
    {
        idx = ter_base + x;
        if (idx >= TER_LEN) idx -= TER_LEN;
        t = ter_top[idx];
        b = ter_bot[idx];
        for (y = 0; y < t; y ++)  OLED_DisplayBuf[y >> 3][x] |= (uint8_t)(1 << (y & 7));
        for (y = b; y < 64; y ++) OLED_DisplayBuf[y >> 3][x] |= (uint8_t)(1 << (y & 7));
    }
}

/* 碰撞：小鸟碰到上壁/下壁返回 1 */
static uint8_t Cave_Collide(int16_t by)
{
    int16_t bx0 = BIRD_X + 3, bx1 = BIRD_X + 13;
    int16_t by0 = by + 3,      by1 = by + 11;
    int16_t x;
    uint16_t idx;

    if (by0 < 0 || by1 > 63) return 1;

    for (x = bx0; x <= bx1; x ++)
    {
        idx = ter_base + x;
        if (idx >= TER_LEN) idx -= TER_LEN;
        if (by0 < ter_top[idx]) return 1;      // 撞上壁
        if (by1 > ter_bot[idx]) return 1;      // 撞下壁
    }
    return 0;
}

/* 小鸟游戏主循环：K3 按键返回 */
int BirdGame_Animation(void)
{
    int16_t by = 27;       // 小鸟左上角纵坐标（机身中心约在通道中间）
    uint8_t wing = 0, wing_cnt = 0;
    float p;
    static float pf = 0;

    Key_Reset();
    BirdGame_Reset();

    while (1)
    {
        if (Key_Check(KEY_3, KEY_SINGLE))                 // 返回游戏菜单
        {
            OLED_Clear();
            OLED_Update();
            Key_Reset();
            return 0;
        }

        MPU6050_Calculation();
        p = Pitch;
        if (p > 45) p = 45; else if (p < -45) p = -45;
        pf = 0.75f * pf + 0.25f * p;
        by = 27 - (int16_t)(pf * PITCH_K);                // 体感：pitch 越大越往上
        if (by < 0) by = 0; else if (by > 50) by = 50;

        /* 地形滚动 + 计分（按飞行距离） */
        ter_base += SCROLL_SPEED;
        if (ter_base >= TER_LEN) ter_base -= TER_LEN;
        score_acc += SCROLL_SPEED;
        if (score_acc >= 16) { score_acc = 0; Bird_Score ++; }

        if (++wing_cnt >= WING_PERIOD) { wing_cnt = 0; wing ^= 1; }

        OLED_Clear();
        Cave_Draw();
        OLED_ShowImage(BIRD_X, by, BIRD_W, BIRD_H, Bird_img[wing]);
        OLED_ClearArea(92, 0, 36, 9);
        OLED_ShowNum(94, 0, Bird_Score, 5, OLED_6X8);
        OLED_Update();

        if (Cave_Collide(by))
        {
            OLED_Clear();
            OLED_ShowString(20, 20, "Game Over", OLED_8X16);
            OLED_ShowNum(94, 44, Bird_Score, 5, OLED_6X8);
            OLED_Update();
            Delay_ms(800);
            OLED_Clear();
            OLED_Update();
            Key_Reset();
            return 0;
        }
    }
}
