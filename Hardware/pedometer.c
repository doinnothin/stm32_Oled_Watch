#include "pedometer.h"
#include "MPU6050.h"
#include <math.h>

/*
 * 计步原理（加速度模值 + 动态峰谷检测）：
 *  1) 每 20ms（50Hz）读一次三轴加速度 (ax,ay,az)，求模值 a = sqrt(ax^2+ay^2+az^2)。
 *     模值与"传感器朝向"无关，所以手腕怎么转都不影响，比直接用 Roll/Pitch 姿态角稳。
 *  2) 用一阶低通(慢跟随)估算重力基线 g，动态分量 dyn = a - g。
 *     走路时手臂/身体上下摆动，会让 dyn 出现"正峰—回落"的周期波形。
 *  3) 状态机：dyn 上穿高阈值 -> 记"在峰上"；再下穿低阈值 -> 计 1 步。
 *     用"最小步间隔"去抖，避免一个波峰被计成两三步。
 *
 * 注：MPU6050_Init() 里配置的是 ±16g 量程，1g ≈ 2048 LSB，
 *     下面阈值单位就是原始 LSB（可据实佩戴效果微调）。
 */

#define PED_SAMPLE_MS    20       /* 采样周期 20ms = 50Hz */
#define PED_TH_HIGH      300.0f   /* 上穿阈值 ≈ +0.15g */
#define PED_TH_LOW       120.0f   /* 下穿阈值 ≈ +0.06g */
#define PED_MIN_STEP_MS  280      /* 最小步间隔≈280ms（约3.5步/秒）去抖 */
#define PED_PEAK_MAX_MS  2000     /* 波峰持续太久则复位，防卡死 */
#define PED_GRAV_ALPHA   0.02f    /* 重力基线慢跟随系数（约1秒时间常数@50Hz） */

static volatile uint32_t s_ms = 0;          /* 1ms 计数（ISR 累加） */
static uint32_t s_last_sample_ms = 0;
static uint32_t s_last_step_ms = 0;
static uint32_t s_peak_ms = 0;
static uint32_t s_steps = 0;

static float   s_grav = 0.0f;               /* 重力基线 */
static uint8_t s_grav_inited = 0;
static uint8_t s_state = 0;                 /* 0=等上升沿 1=在峰上 */

void Pedometer_Init(void)
{
    s_ms = 0;
    s_last_sample_ms = 0;
    s_last_step_ms = 0;
    s_peak_ms = 0;
    s_steps = 0;
    s_grav = 0.0f;
    s_grav_inited = 0;
    s_state = 0;
}

void Pedometer_Isr1ms(void)
{
    s_ms++;
}

uint32_t Pedometer_GetSteps(void)
{
    return s_steps;
}

void Pedometer_Reset(void)
{
    s_steps = 0;
    s_state = 0;
    s_grav_inited = 0;
}

void Pedometer_Update(void)
{
    int16_t ax, ay, az, gx, gy, gz;
    float a, dyn;

    /* 未到 20ms 就不采样（s_ms 由 1ms 中断累加） */
    if ((uint32_t)(s_ms - s_last_sample_ms) < PED_SAMPLE_MS) return;
    s_last_sample_ms = s_ms;

    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
    a = (float)sqrt((double)ax * ax + (double)ay * ay + (double)az * az);

    if (!s_grav_inited)
    {
        s_grav = a;                 /* 第一次采样当作重力初值 */
        s_grav_inited = 1;
        return;
    }

    s_grav += (a - s_grav) * PED_GRAV_ALPHA;     /* 慢速跟随重力 */
    dyn = a - s_grav;                            /* 动态加速度（去掉重力） */

    switch (s_state)
    {
    case 0:   /* 等待上升沿 */
        if (dyn > PED_TH_HIGH)
        {
            s_state = 1;
            s_peak_ms = s_ms;
        }
        break;

    case 1:   /* 在波峰上，等待回落到低阈值 */
        if (dyn < PED_TH_LOW)
        {
            if ((uint32_t)(s_ms - s_last_step_ms) >= PED_MIN_STEP_MS)
            {
                s_steps++;
                s_last_step_ms = s_ms;
            }
            s_state = 0;
        }
        else if ((uint32_t)(s_ms - s_peak_ms) > PED_PEAK_MAX_MS)
        {
            s_state = 0;        /* 一直在高位（晃动/骑行），丢弃 */
        }
        break;

    default:
        s_state = 0;
        break;
    }
}
