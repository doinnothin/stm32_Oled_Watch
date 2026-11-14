# 📽️ 实机演示素材

> 🖼️ `ui/` 子目录里是用**固件同一份 C 代码**在 PC 上渲染出的全部界面图（见 [`ui/README.md`](ui/README.md)）。

把演示用的 **视频 / GIF / 截图** 放到这个文件夹，主 README 的「实机演示」一节会引用它们。

建议命名（便于以后替换）：

| 文件 | 内容 |
|---|---|
| `demo.mp4` | 整体上手演示（首页时钟 → 天气 → 滑动菜单 → 各功能） |
| `demo.gif` | 一段短循环，放在 README 顶部最好看 |
| `clock.png` | 首页时钟截图（时间 / 天气 / 步数） |
| `weather.png` | 天气页三天预报截图 |
| `menu.png` | 滑动菜单截图 |
| `game.gif` | 小恐龙 / 像素小鸟游戏动图 |

放好后，在根目录 `README.md` 的 `📽️ 实机演示` 一节里，把占位表格换成对应的 Markdown 引用即可，例如：

```markdown
![天气页](demo/weather.png)

<video src="demo/demo.mp4" controls width="480"></video>
```

> 大文件建议先压缩（视频转 H.264 MP4、图转 PNG/JPG），必要时再用 Git LFS。
