# 🖥️ 固件 UI 的 PC 端渲染

这里的脚本把**固件里真实运行的同一份 UI 代码**搬到 PC 上跑，把 128×64 OLED 显存逐像素导出成 PNG，
用来在没有实物时预览 / 出演示图。硬件读写（GPIO/I2C、传感器、RTC、网络）全部用桩函数替换为
确定值；**所有绘图逻辑、字库、布局都来自固件原文件**（`Hardware/OLED.c`、`OLED_Data.c`、`menu.c`、
`menu_sub.c`、`weather.c`、`SetTime.c`），与屏幕上看到的几乎一模一样。

## 文件

| 文件 | 说明 |
|---|---|
| `shim/stm32f10x.h` | 极简桩头，让固件源码在 PC 上编译（GPIO 等为空操作） |
| `stubs.c` | 传感器 / RTC / 天气 / 按键等外设桩，返回可控的演示数据 |
| `harness.c` | 依次调用各页面 UI 函数，导出 `out/<页面>.bin` 显存 |
| `render_ui.py` | 把 `.bin` 解码成放大 6 倍的 OLED 风格 PNG |
| `render_sheet.py` | 拼成总览图 `00_overview.png` |
| `build_and_render.ps1` | 一键编译 + 运行 + 出图 |

## 运行

在**仓库根目录**执行（需要 gcc 与 Python + Pillow）：

```powershell
powershell -ExecutionPolicy Bypass -File tools/ui_render/build_and_render.ps1
```

生成的图片在 `tools/ui_render/png/`，总览在 `tools/ui_render/00_overview.png`。
仓库 `demo/ui/` 里放的就是用这套流程导出的成品。
