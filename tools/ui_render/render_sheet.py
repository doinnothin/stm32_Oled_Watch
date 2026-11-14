# -*- coding: utf-8 -*-
# Compose all UI PNGs into one labeled contact sheet.
from PIL import Image, ImageDraw, ImageFont
import os

PNG = "png"
OUT = "00_overview.png"
items = [
    ("01_home_clock", "首页时钟"), ("02_menu", "滑动菜单"),
    ("03_stopwatch", "秒表"), ("04_flashlight", "手电筒"),
    ("05_mpu6050", "MPU6050 姿态"), ("06_gradienter_menu", "水平仪菜单"),
    ("07_level", "水平仪"), ("08_vertical", "竖直仪"),
    ("09_slope", "斜坡仪"), ("10_emoji", "动态表情"),
    ("11_game_menu", "小游戏"), ("12_weather", "天气·三天预报"),
    ("12b_weather_trend", "温度趋势曲线"),
    ("13_temp_humi", "温湿度 AHT20"), ("14_setting_menu", "设置"),
    ("15_set_date", "设置日期"), ("16_set_time", "设置时间"),
]
BG=(10,13,18); CARD=(16,20,27); TXT=(225,240,255); SUB=(130,150,170); ACC=(90,170,255)
fp = r"C:\Windows\Fonts\msyh.ttc"
try:
    f_title = ImageFont.truetype(fp, 34); f_cap = ImageFont.truetype(fp, 20); f_sub = ImageFont.truetype(fp, 16)
except Exception:
    f_title = f_cap = f_sub = ImageFont.load_default()

cols, rows = 4, 5
cell_w, cell_h, pad = 384, 210, 22
img_w, img_h = 360, 180
top = 92
W = pad + cols*(cell_w+pad)
H = top + rows*(cell_h+pad) + 40
sheet = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(sheet)
d.text((pad+6, 26), "STM32 可编程多功能手表 · 固件 UI", font=f_title, fill=TXT)
d.text((pad+8, 66), "以下画面由固件中真实运行的同一份 C 代码在 PC 上渲染（128×64 显存逐像素导出）", font=f_sub, fill=SUB)
for idx, (name, cap) in enumerate(items):
    r, c = divmod(idx, cols)
    x = pad + c*(cell_w+pad); y = top + r*(cell_h+pad)
    d.rounded_rectangle([x, y, x+cell_w, y+cell_h], radius=10, fill=CARD)
    img = Image.open(os.path.join(PNG, name+".png")).convert("RGB").resize((img_w, img_h), Image.LANCZOS)
    sheet.paste(img, (x + (cell_w-img_w)//2, y+10))
    d.text((x+14, y+img_h+16), cap, font=f_cap, fill=TXT)
    d.text((x+cell_w-58, y+img_h+18), f"{idx+1:02d}", font=f_sub, fill=ACC)
sheet.save(OUT)
print("saved", OUT, sheet.size)
