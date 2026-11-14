<div align="center">

# ⌚ STM32 可编程多功能手表 · v2.0

**在 v1 的基础上加了一颗 ESP32-C3：现在它能看到真实的时间和天气。** 🌤️

[![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B)](https://www.st.com/)
![WiFi](https://img.shields.io/badge/WiFi-ESP32--C3%20SuperMini-E7352C)
![Language](https://img.shields.io/badge/Language-C-283593)
![Build](https://img.shields.io/badge/Build-EIDE%20%2F%20Keil%20MDK-00b4d8)
![Version](https://img.shields.io/badge/Version-v2.0-orange)
![License](https://img.shields.io/badge/License-MIT-green)

📽️ [实机演示](#demo) · 🆕 [v2 更新](#changelog) · 🔧 [硬件接线](#hardware) · 🔌 [串口协议](#protocol) · 🚀 [编译烧录](#quickstart)

</div>

## 🧭 项目简介

这是一块基于 **STM32F103C8T6** 的可编程多功能手表。v1 里它已经能走时、滑动菜单、量姿态、玩游戏；到了 **v2**，我们给它配了一颗 **ESP32-C3 SuperMini**：由 C3 负责连 WiFi、取网络时间、拉真实天气，再通过串口把数据喂给 STM32 显示。

于是手表首页有了**真实时间 + 当前天气 + 步数**，还多了一个能看到**今日 / 明日 / 后日 三天预报**的天气页。STM32 侧依然跑在**裸机 + 标准外设库（SPL）**上，C3 侧是 Arduino 固件，两边通过一条 UART 通信，分工清清楚楚。

> 📌 仓库历史里的 **v1 工程**仍保留在 git 提交记录中；当前 `main` 分支为 **v2**。
> 🎬 UI 效果见下方 [实机演示](#demo)（固件同源代码渲染）；真实拍摄的视频 / GIF 可继续放进 [`demo/`](demo/)。

<a id="features"></a>
## ✨ 功能一览

| | 功能 | 说明 |
|---|---|---|
| 🕐 | **首页时钟** | 日期 + 星期 + 大字时间，实时显示天气 / 温度 / 步数 |
| 🌤️ | **真实天气** | 心知天气实况 + **三天预报**；天气页含**列表页 + 温度趋势曲线页**（KEY1/KEY2 滑动切换） |
| 📶 | **网络校时** | ESP32-C3 走 NTP，每隔约 10 分钟自动校准 RTC |
| 🚶 | **计步** | MPU6050 加速度模值峰谷检测，步数显示在首页 |
| 🎞️ | **滑动菜单** | 32×32 图标横向滑动切换 |
| ⏱️ | **秒表** | 开始 / 停止 / 清除，毫秒级 |
| 🔦 | **手电筒** | 一键点亮板载 LED |
| 📐 | **MPU6050 姿态** | 实时 Roll / Pitch / Yaw 欧拉角 |
| 🧭 | **水平仪** | 水平 / 竖直 / 斜坡三种模式 |
| 😀 | **动态表情** | 表情动画循环播放 |
| 🦖 | **小游戏** | 谷歌小恐龙 + 像素小鸟 |
| 🌡️ | **温湿度** | AHT20（I²C，±0.3℃ / ±2%RH，v2 起替换 DHT11） |

<a id="changelog"></a>
## 🆕 v2 更新记录

- 🔗 **新增 ESP32-C3 联网**：WiFi + NTP 校时 + 心知天气（实况 + 三天预报），通过 UART 文本协议 `$OK / $FC / $FD` 发给 STM32。
- 📈 **天气页第二页**：新增**多天温度趋势曲线**（高/低温折线），KEY1/KEY2 左右滑动切换；数据来自 `$FD` 多天预报（心知免费版 3 天，付费最多 15 天，代码自动适配）。
- 🧱 **新增分层驱动**：`Serial.c/h`、`esp32c3.c/h`、`weather.c/h`。
- 🏠 **首页升级**：显示真实时间、当前天气/温度、步数。
- 🌡️ **AHT20 替换 DHT11**：精度更高，移除 DHT11 驱动与页面。
- 🚶 **新增计步**：`pedometer.c/h`，MPU6050 加速度峰谷检测。
- 🔤 **字库补充**：天气/预报所需汉字（晴/雨/雷/霾/今/明/后/步…）。

<a id="demo"></a>
## 📽️ 实机演示

> 🖥️ **下面所有界面，都是把固件里「真实运行的同一份 C 代码」搬到 PC 上逐像素渲染出来的**（读取 128×64 OLED 显存导出，非手绘/PS），
> 因此与硬件屏幕上的画面几乎一模一样。生成方法与脚本见 [`tools/ui_render/`](tools/ui_render/)。
> 真实拍摄的视频 / GIF 之后仍可直接放进 [`demo/`](demo/) 补充。

<p align="center"><img src="demo/ui/00_overview.png" alt="STM32 手表全部 UI 总览（固件同源代码渲染）" width="94%"></p>

| | | | |
|:--:|:--:|:--:|:--:|
| <img src="demo/ui/01_home_clock.png" width="240"><br>**首页时钟**<br>时间 · 天气 · 步数 | <img src="demo/ui/02_menu.png" width="240"><br>**滑动菜单**<br>图标横向轮播 | <img src="demo/ui/03_stopwatch.png" width="240"><br>**秒表**<br>开始 / 停止 / 清除 | <img src="demo/ui/04_flashlight.png" width="240"><br>**手电筒**<br>OFF / ON |
| <img src="demo/ui/05_mpu6050.png" width="240"><br>**MPU6050 姿态**<br>Roll / Pitch / Yaw | <img src="demo/ui/06_gradienter_menu.png" width="240"><br>**水平仪菜单**<br>水平 / 竖直 / 斜坡 | <img src="demo/ui/07_level.png" width="240"><br>**水平仪**<br>气泡居中即水平 | <img src="demo/ui/08_vertical.png" width="240"><br>**竖直仪**<br>气泡居中即竖直 |
| <img src="demo/ui/09_slope.png" width="240"><br>**斜坡仪**<br>角度 + 坡度百分比 | <img src="demo/ui/10_emoji.png" width="240"><br>**动态表情**<br>眨眼动画 | <img src="demo/ui/11_game_menu.png" width="240"><br>**小游戏**<br>小恐龙 / 像素小鸟 | <img src="demo/ui/12_weather.png" width="240"><br>**天气·列表**<br>三天预报（C3 数据） |
| <img src="demo/ui/12b_weather_trend.png" width="240"><br>**温度趋势**<br>多天高/低温曲线（按键翻页） |
| <img src="demo/ui/13_temp_humi.png" width="240"><br>**温湿度**<br>AHT20 | <img src="demo/ui/14_setting_menu.png" width="240"><br>**设置**<br>进入日期时间设置 | <img src="demo/ui/15_set_date.png" width="240"><br>**设置日期**<br>年 / 月 / 日 | <img src="demo/ui/16_set_time.png" width="240"><br>**设置时间**<br>时 / 分 / 秒 |

> 点开任意图片看大图；完整清单与最新渲染见 [`demo/ui/`](demo/ui/)。

<a id="arch"></a>
## 🏗️ 系统架构

```text
        ┌───────────────────────┐        WiFi         ┌────────────────┐
        │  ESP32-C3 SuperMini   │──────────────────▶  │ 路由器 / 互联网 │
        │  Arduino 固件         │  NTP / 心知天气      └────────────────┘
        └──────────┬────────────┘
                   │ UART 115200（GPIO4→PA3，GND 共地）
                   ▼
        ┌───────────────────────┐
        │  STM32F103C8T6 手表   │  OLED 显示 / RTC 走时 / 按键 / 传感器
        └───────────────────────┘
```

- **ESP32-C3**：连 WiFi → NTP 取时 → 请求心知天气（实况 `now.json` + 三天 `daily.json`）→ 解析 JSON → 串口输出几行文本。
- **STM32**：串口接收解析 → 写 RTC → OLED 显示 → 按键与各传感器页面。

<a id="hardware"></a>
## 🔧 硬件与接线

### STM32F103C8T6 引脚

| 外设 | 引脚 |
|---|---|
| OLED（软件 I²C，SSD1306） | SCL=`PB8`，SDA=`PB9` |
| MPU6050（软件 I²C） | SCL=`PB10`，SDA=`PB11` |
| 按键 ×3 | `PB12` / `PB13` / `PB15` |
| 照明 LED | `PA0` |
| AHT20（软件 I²C） | SCL=`PA11`，SDA=`PA12` |
| **ESP32-C3 串口 USART2** | **`PA2`=TX，`PA3`=RX** |

### ESP32-C3 ↔ STM32 接线

| ESP32-C3 | 方向 | STM32 | 说明 |
|---|---|---|---|
| **GPIO4 (TX)** | → | **PA3 (USART2_RX)** | C3 把时间/天气发给 STM32（**必接**） |
| GPIO? (RX) | ← | PA2 (USART2_TX) | 可选：STM32 发 `R` 让 C3 立即刷新 |
| GND | — | GND | **必须共地** |
| 电源 | — | — | 建议给 C3 独立 3.3V/USB 供电（C3 峰值电流大） |

> 🔎 C3 的 RX 引脚以 `esp32c3_firmware/ESP32C3_WeatherClock.ino` 里的 `STM32_RX_PIN` 定义为准（教程图中为 GPIO5）。最少只需接 **GPIO4→PA3、GND、电源** 三根即可显示。
> ⚡ 注意：手表上的 ME6211C33 带不动 C3 的 WiFi 峰值电流，**不要用表内稳压给 C3 供电**，否则会死机/复位。

![接线图](docs/接线图_ESP32C3_STM32.png)

<a id="protocol"></a>
## 🔌 串口协议

一行一帧，`\r\n` 结尾，波特率 **115200**：

| 报文 | 例子 | 含义 |
|---|---|---|
| `$OK,<utc秒>,<天气码>,<温度>` | `$OK,1794577140,13,20` | 时间 + 当前天气 |
| `$FC,<今c>,<今高>,<今低>,<明c>,<明高>,<明低>,<后c>,<后高>,<后低>` | `$FC,9,27,18,0,28,18,13,28,19` | 三天预报（列表页） |
| `$FD,<n>,<c0>,<h0>,<l0>,<c1>,<h1>,<l1>,...` | `$FD,3,10,26,19,0,28,19,4,26,21` | n 天预报（温度趋势曲线页，n≤15） |
| `$ERR,0,0,0` | — | 取数失败 |

- `utc` 为 UTC 秒，STM32 写入 RTC，显示时 **+8 小时**（北京时间）。
- 天气用**数字代码**（心知天气 code）传输，STM32 查表转中文，避免串口传中文的编码问题。
- STM32 上电发送 `'R'` 可请求 C3 立即刷新（需接 PA2→C3 RX）。

<a id="structure"></a>
## 📁 目录结构

```text
.
├─ Hardware/            STM32 外设驱动与页面
│  ├─ OLED.c/h          SSD1306 显示驱动（含字库 OLED_Data.c）
│  ├─ MyI2C.c/h         软件 I²C（MPU6050 用）
│  ├─ MPU6050.c/h       六轴姿态
│  ├─ aht20.c/h         AHT20 温湿度（软件 I²C，PA11/PA12）
│  ├─ Serial.c/h        USART1/2 驱动（中断接收）
│  ├─ esp32c3.c/h       ESP32-C3 串口协议解析（$OK/$FC/$FD）
│  ├─ weather.c/h       天气页（列表 + 温度趋势曲线，按键翻页）
│  ├─ pedometer.c/h     计步（MPU6050 加速度峰谷检测）
│  ├─ menu.c/h          首页时钟 + 滑动菜单
│  ├─ menu_sub.c/h      秒表 / 手电 / 姿态 / 水平仪 / 表情 / 游戏 / AHT20
│  └─ dino.c / bird.c   两款小游戏
├─ System/              Delay / MyRTC / Timer
├─ User/                main.c、中断与外设初始化
├─ Start/ Library/      CMSIS 启动文件 + 标准外设库（SPL）
├─ .eide/               EIDE 工程配置（推荐构建方式）
├─ esp32c3_firmware/    ESP32-C3 Arduino 固件（ESP32C3_WeatherClock.ino）
├─ docs/                接线图 + 从 0 复刻教程
├─ tools/ui_render/     在 PC 上编译固件 UI 代码并导出界面 PNG 的脚本
└─ demo/                实机演示素材（demo/ui/ 为固件渲染的界面图，可再放实拍视频）
```

<a id="quickstart"></a>
## 🚀 编译与烧录

### STM32（推荐 VS Code + EIDE）

1. 安装 VS Code + **EIDE** 插件、ARM 编译器（AC5 或 AC6）。
2. 打开仓库根目录，EIDE 会自动读取 `.eide/eide.yml`。
3. 若提示缺芯片包，安装 **Keil.STM32F1xx_DFP 2.4.0**。
4. Build → 用 **ST-Link**（SWD）烧录。

> 也可以用 Keil 新建工程，把 `Start / Library / System / Hardware / User` 里的源文件加入即可（本仓库不附带 v2 的 `.uvprojx`）。

### ESP32-C3（Arduino IDE）

1. 开发板管理器安装 **esp32** 包，建议核心版本 **2.0.17**（新版在 C3 上有连接兼容问题）。
2. 开发板选 **ESP32C3 Dev Module**，`USB CDC On Boot = Enabled`。
3. 打开 `esp32c3_firmware/ESP32C3_WeatherClock.ino`，修改顶部配置：
   ```cpp
   WIFI_SSID / WIFI_PASSWORD   // 你的 2.4G WiFi
   CITY                        // 城市拼音，如 guangzhou
   SENIVERSE_KEY               // 心知天气私钥（自己免费申请）
   ```
4. 上传，串口监视器 115200，应看到 `发送 -> utc=... code=... temp=...` 与 `预报 -> ...`。

<a id="faq"></a>
## ❓ 常见问题

- **C3 连不上 WiFi / `reason=2 AUTH_EXPIRE`**：把 esp32 核心降到 **2.0.17**，保持 `WiFi.setTxPower(WIFI_POWER_8_5dBm)`；热点用 **WPA2-PSK / 2.4GHz**，别用 WPA3。
- **STM32 显示 No data（C3 已发出）**：C3 先上电、STM32 后上电会错过首帧。接上 `PA2→C3 RX` 让 STM32 上电自动请求刷新，或按一下 C3 复位。固件已内置开机 3 分钟内每 20 秒重发。
- **OLED 某个汉字显示 "?"**：该字不在字库 `OLED_Data.c` 的 `OLED_CF16x16` 中，需补字模。
- **AHT20 报 `Code:-1`**：寻址失败，检查 SCL/SDA 是否接反、VCC 是否 3.3V、上拉是否正常。

<a id="author"></a>
## 👤 致谢与许可

- OLED 驱动与字库格式来自 **江协科技** 教程（jiangxiekeji.com）。
- 天气数据来自 **心知天气 Seniverse**。
- 本项目在 v1 工程基础上二次开发，许可证 **MIT**（详见 [LICENSE](LICENSE)）。
- 作者 / 维护：[@doinnothin](https://github.com/doinnothin) · 仓库：[stm32_Oled_Watch](https://github.com/doinnothin/stm32_Oled_Watch)

<div align="center"><sub>⌚ + 📶 = 一块会看天气的表。玩得开心。</sub></div>
