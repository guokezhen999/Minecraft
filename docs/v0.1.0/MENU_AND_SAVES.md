# 菜单与存档规划

v0.1.0 已按本规划落地。摘要见 [OVERALL.md](OVERALL.md)。光图不写入存档，见 [LIGHTING.md](LIGHTING.md)。

---

## 目标

- 启动进 **标题菜单**，不自动建世界
- 游戏中 **ESC → 暂停**（松鼠标、停模拟、世界留在内存）
- 多存档：列表加载 / 删除；随机或手输种子新建
- 设置：FOV、视距、鼠标灵敏度、垂直同步、全屏；写全局配置，不跟某个世界绑死
- 切世界 / 回标题 / 退出前 **flush**，挖过的坑还在

## 不做

中文输入法、云存档、世界预览图、region 大文件、改键、音量。

---

## 界面

```text
Title ──┬── Worlds ──┬── CreateWorld
        │            └── Load / Delete
        ├── Settings
        └── Quit

Playing ── ESC ── Paused ──┬── Resume
                           ├── Worlds / CreateWorld   （切档前 flush）
                           ├── Settings
                           ├── Save & Title
                           └── Quit
```

| 状态 | 鼠标 | 世界 | 模拟 |
|---|---|---|---|
| Title / Worlds / CreateWorld / Settings（未进游戏） | 自由 | 无 | — |
| Playing | 捕获 | 有 | 跑 |
| Paused 及从暂停打开的 Worlds / Settings | 自由 | 有，留内存 | 停 |

- Playing + ESC → Paused，松鼠标
- 子菜单 + ESC → 上一级，**不关窗口**
- 退出只走菜单「退出游戏」
- 失焦：松鼠标并进暂停

### 标题

天空背景 + 面板，不建 `World`。

| 按钮 | 行为 |
|---|---|
| 继续 | 打开 `LastWorld`；没有则进存档列表 |
| 存档 | → Worlds |
| 设置 | → Settings |
| 退出游戏 | flush（若有世界）→ 关窗口 |

### 暂停

背后仍画世界，上面压一层暗色（alpha ~0.55）。隐藏快捷栏 / 准星。

| 按钮 | 行为 |
|---|---|
| 继续 | 捕获鼠标 → Playing |
| 存档 | → Worlds（当前档仍在内存，直到真的 Load / 新建） |
| 设置 | → Settings |
| 保存并回标题 | flush → 销毁 World → Title |
| 退出游戏 | flush → 关窗口 |

### 存档列表

扫描 `saves/worlds/*/world.dat`，按 `LastPlayed` 新到旧。每行：显示名、种子、上次游玩。

| 按钮 | 行为 |
|---|---|
| 进入 | 若正在玩别的档：先 flush 再销毁，再加载 → Playing |
| 删除 | 确认后删除；删的是当前档则回标题 |
| 新建世界 | → CreateWorld |
| 返回 | Title 或 Paused |

空列表只留「新建世界」。损坏的 `world.dat` 标「无法读取」，不能进入。

### 新建世界

| 控件 | 规则 |
|---|---|
| 名称 | ASCII，默认 `New World`；文件夹名 slug 化，重名加 `_2` |
| 种子 | 空 = 随机；只接受整数（可负）；非法则创建按钮无效 |
| 创建 | 建目录 + 写 `world.dat` → 加载 → Playing；记下 `LastWorld` |
| 返回 | 回 Worlds |

实际用的种子写入档里，列表上能看见。

### 设置

全局 `saves/settings.cfg`。

| 项 | 范围 | 何时生效 |
|---|---|---|
| FOV | 60–110，默认 90 | 立刻 |
| 视距 | 4–16 chunk，默认 10 | 回到 Playing 后下一帧 |
| 鼠标灵敏度 | 0.04–0.30，默认 0.10 | 立刻 |
| 垂直同步 | 开 / 关 | 立刻 `glfwSwapInterval` |
| 全屏 | 开 / 关 | 立刻；关时回到 `windowX/Y` |

视距：`World` 持有 `m_renderDistance`，卸载距离 = 视距 + 2；雾 / 花草 LOD 按当前视距算。

---

## 字体

一张 ASCII 点阵图，不引入 FreeType。

```text
resources/ui/font_ascii.png    16×16 格，ASCII 32–126
```

`HudRenderer::drawText` / `drawQuad`。存档名、种子只允许 ASCII。按钮文案用英文。

---

## 存档布局

```text
saves/
  settings.cfg
  worlds/
    <folder>/
      world.dat
      c.<cx>.<cz>          # 仅被改过的整列
```

不加 region。一列约 64 KB。

### `settings.cfg`

```text
Version 1
Fov 90
RenderDistance 10
MouseSensitivity 0.10
Vsync 1
Fullscreen 0
WindowWidth 1280
WindowHeight 720
LastWorld my_world
```

缺文件或缺字段用 `Config` 默认值。写失败不崩游戏。

### `world.dat`

```text
Version 1
Name New World
Seed 114514
Player 0.5 64.0 0.5
Look -90.0 0.0
Flying 0
Hotbar 3 2 1 6 4 5 8 7 9
Selected 0
Created 1690000000
LastPlayed 1690000100
GameTime 6000
```

- `Player`：脚底坐标
- `Look`：yaw / pitch
- `Hotbar`：9 个 `BlockId`
- **不存光图、不存 Chunk 本身**
- `Version` 不够新则拒绝加载，列表标损坏

以后加背包：往后加行，`Version` +1。改地形生成：升 Version，或丢掉旧列文件只保留玩家头。

### 列文件 `c.<cx>.<cz>`

```text
u32 magic = 'MCCH'
u32 version = 1
i32 cx, cz
u8  id[CHUNK_VOLUME * CHUNK_SECTIONS]
u8  meta[CHUNK_VOLUME * CHUNK_SECTIONS]
```

整列都写（含空气）。文件在就 `setBlockRaw` 灌进去，**不要再 `fillChunk`**；文件不在就程序化生成。装饰必须由种子决定，否则只存脏列会丢树。

### 脏标记

`Section::m_dirty` 只表示要重建 mesh。`Chunk::m_modified` 表示要写盘：玩家 / 流水 `setBlock` 置位；`setBlockRaw` / `fillChunk` 不置位。

- 超出卸载距离：`modified` 则写列文件，然后 `erase`
- 切档 / 回标题 / 退出：所有已加载且 `modified` 的列 + `world.dat` 写完再销毁 World

---

## 运行时

切档在主线程：

```text
flushWorld()
m_World.reset()
m_World = make_unique<World>(seed, saveDir)
读 world.dat → Player / Camera / Hotbar
m_World->Update(cameraPos)
```

`World(seed, savePath)`：`fillChunk` 前先看磁盘有没有该列。`Game::Init` 只初始化渲染器和标题相机。

```text
if Playing: 玩法输入 + 更新玩家 / 世界
else:       菜单输入，不更新世界
有 World 就画世界
不是 Playing 就叠菜单
```

挖放、滚轮、快捷栏数字键只在 Playing 且鼠标已捕获时生效。

控件用立即模式（`UI/Menu.*`），绘制走 `HudRenderer`，与快捷栏共用。

---

## 步骤

0. **点阵字 + 暂停壳**：ESC 切换 Playing / Paused；暂停停模拟、松鼠标。ESC 不再关窗口，暂停时人不会掉下去
1. **块落盘**：`m_modified`、列文件、`world.dat`、退出 flush。挖坑 → 走远卸载 → 走回还在；关游戏再开人也在
2. **多存档**：Title / Worlds / CreateWorld；切档 flush。两个种子地形不同；切走再切回来改过的块还在
3. **设置页**：`settings.cfg`；FOV / 灵敏度 / vsync 立刻生效，视距回游戏后生效

---

## 验收

1. 启动是标题，不是直接进世界
2. 能新建随机种子、指定种子，地形可复现
3. 能加载、删除；删当前档则回标题
4. ESC 暂停；继续后鼠标捕获、模拟恢复
5. 挖坑、切到另一档再切回来，坑还在；关游戏再开同样
6. 设置里改 FOV / 灵敏度 / 视距，重启后还在

---

## 改动面

| 模块 | 改什么 |
|---|---|
| `World/WorldSave.*` | `world.dat` + 列文件、扫档 |
| `World` | `savePath`；优先读列；`flush`；运行时视距 |
| `Chunk` | `m_modified`，与 mesh dirty 分开 |
| `Game` | 状态机；Init 不建世界；切档 |
| `Main.cpp` | ESC / 点击 / 字符按状态分流；vsync / 全屏 |
| `Config.h` | 对齐 `settings.cfg`；灵敏度 |
| `UI/Menu.*`、`Renderer/HudRenderer.*` | 立即模式控件 + 点阵字 |
| `HotbarRenderer` | 复用 HudRenderer |
| `resources/ui/font_ascii.png` | ASCII 字图 |
