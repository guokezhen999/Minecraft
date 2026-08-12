# Minecraft 克隆 — 开发规划

> 当前进度：**垂直 Section 体素世界**（高度 128、水面/群系重调、按 Section 脏标记重建）。  
> 目标：从「风景浏览器」推进到「可玩的体素沙盒原型」。

---

## 现状摘要

### 已完成

- SFML 窗口、OpenGL 3.3、相机（WASD + 鼠标/方向键）
- 着色器、纹理图集、方块数据（草/土/石/水/树/花草等）
- Chunk 列（8 × 16³ Section）+ 面剔除 Mesh（solid / water / flora）
- World：按相机加载视野 Chunk、距离裁剪渲染、按 Section 视锥剔除
- 种子噪声地形：海洋 / 草原 / 沙漠 / 山地 + 树木与装饰（适配 WORLD_HEIGHT）
- **阶段 0 修债** + **渲染优化**：视锥剔除、异步生成/建网、透明排序、flora LOD、批量绘制
- **阶段 1 方块交互**：`getBlock` / `setBlock`、Raycast、左键挖 / 右键放、选中框
- **阶段 2 玩家物理**：AABB 碰撞、重力、跳跃、潜行、飞行开关、相机绑眼睛高度
- **阶段 3 世界结构**：垂直 Section、水面/群系重调、Section 脏标记、异步生成（沿用）

### 主要缺口

- 无背包 / HUD / 存档 / 真实光照
- 尚未做真正的 mesh 合并（多 Section 合成单 VBO）；当前是按 solid/transparent 分批减少状态切换

### 阶段定位

| 阶段 | 内容 | 状态 |
|------|------|------|
| 1 | 画单个立方体 / 纹理 | 完成 |
| 2 | Chunk mesh + 面剔除 | 完成 |
| 3 | 无限世界 + 程序化地形 | 完成 |
| 4 | 破坏 / 放置方块 | 完成 |
| 5 | 碰撞 / 重力 / 第一人称玩家 | 完成 |
| 6 | 光照、生物、UI、存档… | **当前** |

---

## 阶段 0：先修债 ✅

把现有世界修顺，再加玩法。

1. **统一世界高度** ✅  
   `WATER_LEVEL = 8`，地形高度按单层 `CHUNK_SIZE = 16` 重新缩放；出生点抬到水面上方。
2. **Chunk 卸载** ✅  
   超出 `UNLOAD_DISTANCE` 的 Chunk 从内存移除。
3. **跨 Chunk 邻接面剔除** ✅  
   `Chunk::updateMesh(World&)` 经 `World::getBlock` 查询邻居；新 Chunk 生成后标记自身与四邻 remesh。
4. **生成性能** ✅  
   每帧最多生成 `MAX_CHUNKS_GENERATED_PER_FRAME` 个、构建 `MAX_MESHES_BUILT_PER_FRAME` 个 mesh；按距玩家远近入队。

常量见 `World/WorldConstants.h`。

---

## 阶段 1：方块交互 ✅

目标：能挖、能放，世界真正可修改。

1. **世界坐标读写 API** ✅  
   `World::getBlock` / `setBlock(worldX, Y, Z)`，内部换算到 Chunk；改块后同步 remesh 本 Chunk 与边界邻居。
2. **射线检测（Raycast）** ✅  
   `Physics/RayCast`：Amanatides & Woo 体素遍历；命中固体/树叶/花草，穿过空气与水；记录 `blockPos`（挖）与 `previousPos`（放）。
3. **破坏 / 放置** ✅  
   左键挖、右键放（默认石块；`1–4` 切换石/土/草/沙）；`setBlock` 后立即重建 mesh。
4. **选中框** ✅  
   `OutlineRenderer` 线框绘制准星目标方块。

---

## 阶段 2：玩家物理 ✅

目标：从飞行相机变成站在地上的人。

1. **完善 `AABB`** ✅  
   `Physics/AABB.h`：min/max、与方块相交检测。
2. **方块碰撞** ✅  
   `Player` 逐轴移动并解析；`World::isCollidable`（未加载 Chunk 视为实心，避免掉虚空）。
3. **重力 + 跳跃** ✅  
   落地、空格跳跃、Shift 潜行；`V` 切换飞行（Space/`R` 上升，Shift/`F` 下降）。
4. **相机绑定玩家** ✅  
   眼睛高度跟随站立 / 潜行；鼠标锁定仍用于视角。

---

## 阶段 3：世界结构升级 ✅

目标：更接近 Minecraft 的世界尺度。

1. **垂直 Chunk Section** ✅  
   一列 `CHUNK_SECTIONS`（8）个 16³ section，`WORLD_HEIGHT = 128`。
2. **水面 / 生物群系参数重调** ✅  
   `WATER_LEVEL = 62`；海洋 / 草原 / 沙漠 / 山地振幅按新高度重设。
3. **Chunk 脏标记** ✅  
   每 Section 独立 dirty；改块只 rebuild 本 Section 与垂直/水平邻接 Section。
4. **异步生成** ✅  
   工作线程生成 / 建网，主线程限流上传 GPU（阶段 0 已有，垂直后沿用）。

---

## 阶段 4：光照与渲染观感

目标：看起来不再平涂。

1. **环境光遮蔽 / 加强顶点光照**（已有 cardinal light，可扩展）
2. **天空盒 / 雾** — 远景过渡，遮盖加载边界
3. **透明排序** — 水、树叶正确混合（先不透明后透明）
4. **（进阶）阳光 + 方块光 BFS** — Minecraft 式光照

---

## 阶段 5：物品与 UI

目标：有基础工具感。

1. **快捷栏（Hotbar）** — 1–9 选方块类型
2. **准星 + 简单 HUD** — SFML 2D 叠在 OpenGL 上即可
3. **背包** — 可后置；先做热键直接拿方块

---

## 阶段 6：内容与系统（按兴趣选做）

| 方向 | 内容 |
|------|------|
| 地形 | 洞穴、矿石、更多生物群系 |
| 玩法 | 生命值、饥饿、昼夜 |
| 实体 | 生物 AI |
| 持久化 | 世界存档 / 加载 |
| 音效 | 走路、挖方块 |

---

## 推荐落地顺序

最短路径到「好玩」：

```
修高度 / 卸载
    → World get/setBlock
    → Raycast 挖放
    → AABB 碰撞重力
    → 垂直 Section
    → Hotbar
    → 雾 / 光照
    → 洞穴矿石 / 存档
```

### 近期可执行清单（约两周）

1. 修水面与高度逻辑 ✅  
2. Chunk 卸载 ✅  
3. `getBlock` / `setBlock` ✅  
4. Raycast + 挖放 + mesh 更新 ✅  
5. AABB + 重力 ✅  
6. 垂直 Section + 高度重调 ✅  

---

## 相关代码入口

| 模块 | 路径 |
|------|------|
| 游戏主循环 | `Game.cpp` / `Main.cpp` |
| 玩家物理 | `Physics/Player.cpp` |
| 世界 / Chunk 管理 | `World/World.cpp` |
| 地形生成 | `World/TerrainGenerator.cpp` |
| Chunk 列 / Section | `World/Chunk/Chunk.*` / `ChunkSection.*` |
| Chunk Mesh | `World/Chunk/ChunkMesh.*` |
| 射线检测 | `Physics/RayCast.cpp` |
| 选中框 | `Renderer/OutlineRenderer.cpp` |
| 渲染调度 | `Renderer/RenderMaster.cpp` |
| 碰撞盒 | `Physics/AABB.h` |
| 世界常量 | `World/WorldConstants.h` |
