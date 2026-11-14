/* Hardware/peripheral stubs so the real UI translation units link on PC.
 * Every function returns deterministic, controllable "sensor / RTC / net" data
 * so we can render representative screens of the actual firmware UI code. */
#include <stdint.h>
#include <stddef.h>

/* ---------------------------------------------------------------- RTC */
int MyRTC_Time[6] = {2026, 11, 14, 16, 26, 52};
void MyRTC_Init(void) {}
void MyRTC_SetTime(void) {}
void MyRTC_ReadTime(void) {}

/* ---------------------------------------------------------------- Delay */
int g_capture = 0;          /* 1: snapshot framebuffer on next Delay_ms */
int g_captured = 0;
const char *g_capture_name = 0;
int g_force_exit = 0;       /* make Key_GetNum() return 3 once */
void dump_framebuffer(const char *name);

void Delay_us(uint32_t us) { (void)us; }
void Delay_s(uint32_t s) { (void)s; }
void Delay_ms(uint32_t ms)
{
    (void)ms;
    if (g_capture && !g_captured) {
        dump_framebuffer(g_capture_name);
        g_captured = 1;
        g_force_exit = 1;
    }
}

/* ---------------------------------------------------------------- Keys */
uint8_t Key_GetNum(void) { if (g_force_exit) { g_force_exit = 0; return 3; } return 0; }
uint8_t Key_Check(uint8_t n, uint8_t Flag) { (void)n; (void)Flag; return 0; }
void Key_Reset(void) {}
void Key_Tick(void) {}
void Key_Init(void) {}

/* ---------------------------------------------------------------- LED */
void LED_Init(void) {}
void LED_ON(void) {}
void LED_OFF(void) {}
void LEDs_Turn(void) {}

/* ---------------------------------------------------------------- MPU6050 */
int16_t g_mu_ax = -560, g_mu_ay = 1140, g_mu_az = 10000;
int16_t g_mu_gx = 0, g_mu_gy = 0, g_mu_gz = 0;
void MPU6050_Init(void) {}
uint8_t MPU6050_GetID(void) { return 0x68; }
void MPU6050_WriteReg(uint8_t r, uint8_t d) { (void)r; (void)d; }
uint8_t MPU6050_ReadReg(uint8_t r) { (void)r; return 0; }
void MPU6050_GetData(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz)
{
    *ax = g_mu_ax; *ay = g_mu_ay; *az = g_mu_az;
    *gx = g_mu_gx; *gy = g_mu_gy; *gz = g_mu_gz;
}

/* ---------------------------------------------------------------- AHT20 */
float g_aht_t = 26.4f, g_aht_h = 58.2f;
int g_aht_ret = 0;   /* 0 == AHT20_OK */
int AHT20_Init(void) { return 0; }
int AHT20_Read(float *Temp, float *Humi) { *Temp = g_aht_t; *Humi = g_aht_h; return g_aht_ret; }

/* ---------------------------------------------------------------- ESP32-C3 (weather/time feed) */
static char wtext[] = "晴";
static char ftext0[] = "晴", ftext1[] = "多云", ftext2[] = "小雨";
static char *ftext[3] = {ftext0, ftext1, ftext2};
int g_wvalid = 1, g_fcvalid = 1, g_temp = 24;
int g_fc_hi[3] = {27, 28, 26}, g_fc_lo[3] = {18, 19, 17};

void ESP32C3_Init(void) {}
void ESP32C3_Poll(void) {}
uint8_t ESP32C3_DataValid(void) { return (uint8_t)g_wvalid; }
char *ESP32C3_WeatherText(void) { return wtext; }
int ESP32C3_Temperature(void) { return g_temp; }
int ESP32C3_WeatherCode(void) { return 0; }
uint8_t ESP32C3_ForecastValid(void) { return (uint8_t)g_fcvalid; }
char *ESP32C3_ForecastText(int day) { return (day >= 0 && day < 3) ? ftext[day] : (char *)""; }
int ESP32C3_ForecastHigh(int day) { return (day >= 0 && day < 3) ? g_fc_hi[day] : 0; }
int ESP32C3_ForecastLow(int day) { return (day >= 0 && day < 3) ? g_fc_lo[day] : 0; }
int ESP32C3_ForecastCode(int day) { (void)day; return 0; }
uint16_t ESP32C3_RxLines(void) { return 12; }
uint16_t ESP32C3_OkCount(void) { return 6; }
uint16_t ESP32C3_FcCount(void) { return 6; }

/* 多天趋势（用于温度趋势页渲染） */
static int g_tr_hi[5] = {27, 28, 26, 25, 24};
static int g_tr_lo[5] = {18, 19, 17, 16, 15};
uint8_t ESP32C3_TrendCount(void) { return 3; }
char *ESP32C3_TrendText(int day) { return (day >= 0 && day < 3) ? ftext[day] : (char *)""; }
int ESP32C3_TrendHigh(int day) { return (day >= 0 && day < 5) ? g_tr_hi[day] : 0; }
int ESP32C3_TrendLow(int day) { return (day >= 0 && day < 5) ? g_tr_lo[day] : 0; }
int ESP32C3_TrendCode(int day) { (void)day; return 0; }

/* ---------------------------------------------------------------- Pedometer */
uint32_t g_steps = 1234;
void Pedometer_Init(void) {}
void Pedometer_Isr1ms(void) {}
void Pedometer_Update(void) {}
uint32_t Pedometer_GetSteps(void) { return g_steps; }
void Pedometer_Reset(void) {}

/* ---------------------------------------------------------------- Games (stubbed) */
int DinoGame_Animation(void) { return 0; }
void Dino_Tick(void) {}
void DinoGame_Pos_Init(void) {}
int isColliding(void *a, void *b) { (void)a; (void)b; return 0; }
void BirdGame_Reset(void) {}
int BirdGame_Animation(void) { return 0; }
void Bird_Tick(void) {}
