/* Host harness: runs the REAL firmware UI code and dumps each screen's
 * 128x64 OLED framebuffer (8 pages x 128 columns) to out/<name>.bin.
 * A Python step turns these into scaled PNGs.
 *
 * Build/run:  powershell -ExecutionPolicy Bypass -File tools/ui_render/build_and_render.ps1
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "OLED.h"

void Show_ui_Clock(void);
void Show_SettingPage_UI(void);
void Show_SetTime_FirstUI(void);
void Show_SetTime_SecondUI(void);
void Menu_Animation(void);
void Show_StopWatch_UI(void);
void Show_LED_UI(void);
void Show_MPU6050_UI(void);
void Show_GradienterMenu_UI(void);
void Show_Gradienter_UI(void);
void Show_Vertical_UI(void);
void Show_Slope_UI(void);
void Show_Emoji_UI(void);
void Show_Game_UI(void);
void Show_Weather_UI(void);
int  AHT20_Page(void);
void MPU6050_Calculation(void);

extern int MyRTC_Time[];
extern float Roll, Pitch, Yaw;
extern uint8_t pre_selection, tag_selection, x_pre, move_flag;
extern int menu_flag;
extern uint8_t hour, min, sec;

extern int16_t g_mu_ax, g_mu_ay, g_mu_az;
extern int g_wvalid, g_temp, g_fcvalid, g_steps;
extern float g_aht_t, g_aht_h;
extern int g_aht_ret;
extern int g_capture, g_captured, g_force_exit;
extern const char *g_capture_name;

void dump_framebuffer(const char *name)
{
    char path[256];
    snprintf(path, sizeof(path), "out/%s.bin", name);
    FILE *f = fopen(path, "wb");
    if (f) { fwrite(OLED_DisplayBuf, 1, sizeof(OLED_DisplayBuf), f); fclose(f); }
    else { fprintf(stderr, "cannot write %s\n", path); }
}

static void reset_buf(void) { memset(OLED_DisplayBuf, 0, sizeof(OLED_DisplayBuf)); }

static void set_mpu(int ax, int ay, int az)
{
    g_mu_ax = (int16_t)ax; g_mu_ay = (int16_t)ay; g_mu_az = (int16_t)az;
    for (int i = 0; i < 200; i++) MPU6050_Calculation();
}

int main(void)
{
    int t[6] = {2026, 11, 14, 16, 26, 52};
    memcpy(MyRTC_Time, t, sizeof(t));
    g_wvalid = 1; g_temp = 26; g_steps = 1234;
    reset_buf(); Show_ui_Clock(); OLED_ReverseArea(0, 48, 32, 16);
    dump_framebuffer("01_home_clock");

    reset_buf(); pre_selection = 1; tag_selection = 1; x_pre = 48; move_flag = 0;
    Menu_Animation();
    dump_framebuffer("02_menu");

    reset_buf(); hour = 0; min = 12; sec = 34; Show_StopWatch_UI();
    OLED_ReverseArea(8, 44, 32, 16);
    dump_framebuffer("03_stopwatch");

    reset_buf(); Show_LED_UI(); OLED_ReverseArea(72, 20, 24, 24);
    dump_framebuffer("04_flashlight");

    reset_buf(); set_mpu(-560, 1140, 10000); Show_MPU6050_UI();
    OLED_ReverseArea(0, 0, 16, 16);
    dump_framebuffer("05_mpu6050");

    reset_buf(); Show_GradienterMenu_UI(); OLED_ReverseArea(0, 16, 48, 16);
    dump_framebuffer("06_gradienter_menu");

    reset_buf(); set_mpu(-30, 50, 10000); Show_Gradienter_UI();
    dump_framebuffer("07_level");

    reset_buf(); set_mpu(0, 10000, 100); Show_Vertical_UI();
    dump_framebuffer("08_vertical");

    reset_buf(); set_mpu(-500, 4000, 10000); Show_Slope_UI();
    dump_framebuffer("09_slope");

    reset_buf(); g_capture = 0; Show_Emoji_UI();
    dump_framebuffer("10_emoji");

    reset_buf(); Show_Game_UI(); OLED_ReverseArea(0, 16, 80, 16);
    dump_framebuffer("11_game_menu");

    reset_buf(); g_wvalid = 1; g_temp = 24; g_fcvalid = 1; Show_Weather_UI();
    OLED_ReverseArea(0, 0, 16, 16);
    dump_framebuffer("12_weather");

    reset_buf(); g_aht_t = 26.4f; g_aht_h = 58.2f; g_aht_ret = 0;
    g_capture = 1; g_captured = 0; g_capture_name = "13_temp_humi"; g_force_exit = 0;
    AHT20_Page();
    g_capture = 0;

    reset_buf(); Show_SettingPage_UI(); OLED_ReverseArea(0, 0, 16, 16);
    dump_framebuffer("14_setting_menu");

    reset_buf(); Show_SetTime_FirstUI(); OLED_ReverseArea(0, 0, 16, 16);
    dump_framebuffer("15_set_date");

    reset_buf(); Show_SetTime_SecondUI(); OLED_ReverseArea(0, 0, 16, 16);
    dump_framebuffer("16_set_time");

    printf("done\n");
    return 0;
}
