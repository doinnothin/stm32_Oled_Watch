<div align="center">

# ⌚ STM32 可编程多功能手表 · v1.0

**一块自己画板、自己写固件、自己戴着走的小手表。** 🛠️

[![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B)](https://www.st.com/)
![Language](https://img.shields.io/badge/Language-C-283593)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK%20uVision-00b4d8)
![Version](https://img.shields.io/badge/Version-v1.0-orange)
![License](https://img.shields.io/badge/License-MIT-green)

📽️ [实机演示](#demo) · 🔧 [硬件平台](#hardware) · 🚀 [快速开始](#quickstart) · 🎮 [操作说明](#control)

</div>

## 🧭 项目简介

这是一块基于 **STM32F103C8T6** 的可编程多功能手表。它不只是一块电子表：RTC 走时、滑动菜单、传感器、小游戏，全都塞进一块小小的 128×64 OLED 里。整套固件跑在**裸机 + 标准外设库（SPL）**上，外设驱动、页面逻辑、动画都是自己一行行敲出来的。

v1.0 是第一个**完整可用**的版本——从首页时钟一路滑到小游戏，功能一个不缺。拿它当智能穿戴、掌上开发板，或者干脆当个 DIY 玩具，都行。😄

> 📌 本仓库只放 **v1 固件工程**；配套的原理图 / PCB、外壳 3D 模型与教程资料不在这里。

<a id="features"></a>
## ✨ 功能一览

| | 功能 | 说明 |
|---|---|---|
| 🕐 | **首页电子时钟** | 大字号时间 + 日期，RTC 独立走时，掉电不丢 |
| 🗓️ | **日期时间设置** | 手动校准年 / 月 / 日 / 时 / 分 / 秒 |
| 🎞️ | **滑动式功能菜单** | 32×32 图标横向滑动，跟着手指一格格丝滑切换 |
| ⏱️ | **秒表** | 开始 / 停止 / 清除，毫秒级计时 |
| 🔦 | **手电筒** | 一键点亮板载 LED 补光 |
| 📐 | **MPU6050 姿态** | 实时解算 Roll / Pitch / Yaw 欧拉角 |
| 🧭 | **水平仪** | 水平 / 竖直 / 斜坡三种模式，带角度与百分比 |
| 😀 | **动态表情包** | 表情动画循环播放，给手表一点"性格" |
| 🦖 | **小游戏** | 谷歌小恐龙 + 像素小鸟，按键操作躲障碍 |
| 🌡️ | **温湿度** | DHT11 实时读取温度与湿度 |

<a id="demo"></a>
## 📽️ 实机演示

> 演示素材（视频 / GIF / 截图）会放在 [`demo/`](demo/) 目录，后续补充。

| 演示内容 | 入口 |
|---|---|
| 首页时钟 & 滑动菜单 | `demo/` _待补充_ |
| 秒表 / 手电筒 / 姿态 / 水平仪 | `demo/` _待补充_ |
| 小游戏 & 动态表情 | `demo/` _待补充_ |

<!--
  把 demo.mp4 / demo.gif 或截图放进 demo/ 文件夹后，
  用下面这种写法替换上面的占位表格即可：
  ![演示](demo/demo.gif)
  <video src="demo/demo.mp4" controls width="480"></video>
-->

<a id="hardware"></a>
## 🔧 硬件平台

| 项目 | 规格 |
|---|---|
| 主控 | **STM32F103C8T6**（Cortex-M3，72 MHz，64 KB Flash / 20 KB SRAM） |
| 显示 | 128×64 OLED（SSD1306，软件 I²C，`PB8`=SCL / `PB9`=SDA，地址 `0x78`） |
| 姿态传感器 | MPU6050 六轴（软件 I²C，`PB10`=SCL / `PB11`=SDA） |
| 温湿度 | DHT11 单总线（`PB14`） |
| 按键 | 3 个独立按键（`PB12` / `PB13` / `PB15`） |
| 照明 LED | `PA0` |
| 走时 | 片内 RTC（外部 32.768 kHz 晶振） |
| 供电 / 充电 | 锂电池 + TP4056 充电管理 + ME6211C33 线性稳压（板级） |
| 下载调试 | SWD 接口（ST-Link） |

> 💡 换引脚 / 换外设时，只需改对应驱动头文件里的宏定义，主逻辑不用动。

<a id="structure"></a>
## 📁 目录结构

```text
├── User/                # 入口与中断：main.c、外设初始化、1 ms 定时器节拍
├── Hardware/            # 外设驱动 + 页面逻辑（OLED、按键、菜单、秒表、游戏…）
│   ├── OLED.c/.h        #   SSD1306 显示驱动（含字库/图形绘制 API）
│   ├── OLED_Data.c/.h   #   汉字字库、图标与图片素材
│   ├── menu.c/.h        #   首页时钟、设置页、滑动菜单调度
│   ├── menu_sub.c/.h    #   秒表、手电筒、姿态、水平仪、表情、游戏、温湿度
│   ├── Key.c/.h         #   三键非阻塞扫描（单击/双击/长按/连发）
│   ├── SetTime.c/.h     #   日期时间设置
│   ├── MPU6050.c/.h     #   六轴姿态解算 + 软件 I²C（MyI2C）
│   ├── dino.c / bird.c  #   谷歌小恐龙 / 像素小鸟
│   └── dht11.c / aht20.c#   温湿度传感器驱动
├── System/              # Delay、RTC、Timer 时基
├── Library/             # STM32F10x 标准外设库（SPL）
├── Start/               # 启动文件、CMSIS 内核、system_stm32f10x
├── demo/                # 实机演示素材（视频 / GIF / 截图）—— 待补充
├── Project.uvprojx      # Keil MDK 工程文件（双击打开即可）
└── README.md
```

<a id="quickstart"></a>
## 🚀 快速开始

**环境**：Keil MDK 5 + `Keil.STM32F1xx_DFP` 设备包（在 Keil Pack Installer 里装一次即可）。

1. 打开 `Project.uvprojx`；
2. `F7` 编译，`F8` 通过 ST-Link（SWD）下载到板子；
3. 上电，看首页时钟亮起来就成功了。🎉

> 🧩 用 VS Code 的话，直接打开 `Project.code-workspace` 浏览/编辑源码即可；编译下载仍走 Keil（本仓库不附带约 87 MB 的设备 pack，保持轻盈）。

<a id="control"></a>
## 🎮 操作说明

三个按键分工很简单：

| 按键 | 引脚 | 作用 |
|---|---|---|
| **K1** | `PB12` | 上一条 / 左移（滑动菜单里翻上一项） |
| **K2** | `PB13` | 下一条 / 右移（滑动菜单里翻下一项） |
| **K3** | `PB15` | 确认 / 进入当前功能；子页面里通常用于操作 |

> 首页按 **K3** 进入滑动菜单；子页面按 **K1** 返回。长按 K1 / K2 可以连续翻页。

<a id="notes"></a>
## ⚠️ 说明与已知限制

- 源码里的中文注释为 **GBK 编码**（Keil 习惯）。在 GitHub 网页上个别注释可能显示为乱码，属正常现象，**不影响编译**。
- DHT11 两次读取间隔需 **≥ 1 秒**，否则会读到旧数据。
- 本工程基于标准外设库（SPL），非 HAL / CubeMX。
- 这是个人 DIY 项目，欢迎提 Issue 交流，但不保证对任意硬件改动即插即用。

<a id="author"></a>
## 👤 作者与许可

- 作者 / 维护：[@doinnothin](https://github.com/doinnothin)
- 仓库：[doinnothin/stm32_Oled_Watch](https://github.com/doinnothin/stm32_Oled_Watch)
- 许可证：**MIT**（详见 [LICENSE](LICENSE)）

<div align="center"><sub>⌚ 把想法焊进一块表里 —— 玩得开心。</sub></div>
