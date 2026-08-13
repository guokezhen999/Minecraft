# Minecraft (OpenGL)

个人学习用的体素沙盒：区块流式加载、水体、光照、昼夜循环，以及可存档的标题/暂停菜单。

当前版本：**v0.1.0**（预发布）。下一版规划：[docs/v0.2.0/OVERALL.md](docs/v0.2.0/OVERALL.md)（地形 [TERRAIN.md](docs/v0.2.0/TERRAIN.md)、地平线雾 [HORIZON_FOG.md](docs/v0.2.0/HORIZON_FOG.md)）。

本版说明：[docs/v0.1.0/OVERALL.md](docs/v0.1.0/OVERALL.md)（光照 [LIGHTING.md](docs/v0.1.0/LIGHTING.md)、菜单与存档 [MENU_AND_SAVES.md](docs/v0.1.0/MENU_AND_SAVES.md)）。

## 功能

- 16³ 区块柱、异步生成/网格、视锥裁剪
- 可流动的水、火把与天空光照、昼夜
- 草方块、树木、沙漠与海洋气候
- 世界存档（`saves/worlds/`）和设置（视距、FOV、Vsync 等）

## 操作

| 按键 | 作用 |
|------|------|
| WASD | 移动 |
| 空格 | 跳跃 / 飞行上升 |
| Shift | 潜行 / 飞行下降 |
| V | 切换飞行 |
| 鼠标左/右键 | 挖方块 / 放方块 |
| 1–9 / 滚轮 | 快捷栏 |
| Esc | 暂停 / 返回菜单 |
| T | 按住加快昼夜 |

## 从源码编译（macOS）

```bash
brew install glfw cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/Minecraft
```

运行目录要能读到 `resources/`（CMake 会在构建后复制到 `build/resources`）。

## 发行包

GitHub Release 里的 `Minecraft-v0.1.0-macos-arm64.zip` 解压后直接运行 `Minecraft`。若系统提示无法打开，在 Finder 里右键 → 打开。

存档写在运行目录下的 `saves/`，不会进 git。
