# 菜单、存档与设置

> 阶段 6 从「固定目录静默读档」升级为：ESC 菜单、多存档切换、随机 / 指定种子建档、游戏与渲染设置。  
> 块数据落盘仍是本阶段的底座；没有它，菜单里的「加载」没有意义。

---

## 现状与缺口

| 现有 | 问题 |
|---|---|
| ESC：第一次松鼠标，第二次关窗口 | 没有暂停，误触第二次就退出 |
| 松鼠标后任意点击重新捕获 | 无法点 UI |
| `Game::Init` 写死种子 `114514`，立刻建 `World` | 不能选档、不能指定种子 |
| `Config` 有 FOV / 视距 / 全屏，但视距走 `WorldConstants::RENDER_DISTANCE` | 设置改了也不生效 |
| HUD 只有色块 + 图集图标，**没有字体** | 菜单、种子输入、存档名都画不出来 |
| 路线图原计划「第一版固定目录、不做选世界 UI」 | 被本方案取代 |

---

## 目标

- 启动先进入 **标题菜单**，不自动建世界。
- 游戏中 **ESC → 暂停菜单**（松鼠标、停模拟、世界还在内存里）。
- **存档**：列出已有世界；加载；新建（随机种子或手输种子）；删除。
- **设置**：FOV、视距、鼠标灵敏度、垂直同步；写到全局配置，不跟某个世界绑死。
- 切世界 / 回标题 / 退出前 **flush** 当前档，保证挖的坑还在。

不做：中文输入法、云存档、世界预览图、region 大文件、设置里改键位。

---

## 界面状态

```text
Title ──┬── Worlds ──┬── CreateWorld
        │            └── (Load / Delete)
        ├── Settings
        └── Quit

Playing ── ESC ── Paused ──┬── Resume → Playing
                           ├── Worlds / CreateWorld   （切档前先 flush）
                           ├── Settings
                           ├── Save & Title → Title
                           └── Quit Game
```

| 状态 | 鼠标 | 世界 | 模拟 |
|---|---|---|---|
| `Title` / `Worlds` / `CreateWorld` / `Settings`（未进游戏） | 自由 | 无 | — |
| `Playing` | 捕获 | 有 | 跑 |
| `Paused` 及从暂停打开的 Worlds / Settings | 自由 | 有，留在内存 | **停**（不 `Player::update` / 不 `World::Update`） |

`Main.cpp` 里现在的 ESC 逻辑整段换掉：

- `Playing` + ESC → `Paused`，松鼠标。
- 子菜单 + ESC → 回到上一级（暂停或标题），**不要关窗口**。
- 退出只走菜单里的「退出游戏」。
- 失焦仍可松鼠标并进暂停（已有 `focusCallback` 可接到 `Paused`）。

---

## 各屏做什么

### 标题

半透明面板叠在天空上（用现有 `SkyRenderer` + 一台看向地平线的相机，不建 `World`）。

| 按钮 | 行为 |
|---|---|
| 继续 | 打开 `settings` 里记下的 `LastWorld`；没有则进存档列表 |
| 存档 | → Worlds |
| 设置 | → Settings |
| 退出游戏 | flush（若有世界）→ 关窗口 |

### 暂停（游戏中 ESC）

背后继续画世界最后一帧，上面压一层暗色（`hud` 着色器全屏 quad，alpha ~0.55）。快捷栏 / 准星隐藏。

| 按钮 | 行为 |
|---|---|
| 继续 | 捕获鼠标 → `Playing` |
| 存档 | → Worlds（当前档仍在内存，直到真的 Load / 新建） |
| 设置 | → Settings |
| 保存并回标题 | flush → 销毁 `World`（join worker）→ `Title` |
| 退出游戏 | flush → 关窗口 |

### 存档列表

扫描 `saves/worlds/*/world.dat`，按 `LastPlayed` 新到旧。

每行：显示名、种子、上次游玩时间。选中后：

| 按钮 | 行为 |
|---|---|
| 进入 | 若正在玩别的档：先 flush 再销毁，再加载；成功 → `Playing` |
| 删除 | 确认条「删除后无法恢复」；删的是当前正在玩的档则回标题 |
| 新建世界 | → CreateWorld |
| 返回 | 回到 Title 或 Paused |

列表空时只留「新建世界」。损坏的 `world.dat` 显示为灰色「无法读取」，不能进入。

### 新建世界

| 控件 | 规则 |
|---|---|
| 名称 | ASCII，默认 `New World`；文件夹名由名称 slug 化，重名加 `_2` |
| 种子 | 空 = 随机（按钮「随机」填入一个 `int32` 并显示出来） |
| 种子（手输） | 只接受整数，可负号；非法时创建按钮无效 |
| 创建 | 建目录 + 写 `world.dat` → 加载 → `Playing`；`LastWorld` 指向它 |
| 返回 | 回 Worlds |

随机种子：`std::random_device` 失败则用时间戳。创建后 **把实际用的种子写进档里**，列表上能看见，方便复现地形。

### 设置

从标题或暂停都能进，改的是 **全局** `saves/settings.cfg`，不是某个世界。

| 项 | 范围 | 何时生效 |
|---|---|---|
| 视野 FOV | 60–110，默认 90 | 立刻（`Camera::Zoom` + 重建投影） |
| 视距 | 4–16 chunk，默认 10 | 立刻（见下文，须把常量改成运行时） |
| 鼠标灵敏度 | 0.04–0.30，默认 0.10 | 立刻 |
| 垂直同步 | 开 / 关 | 立刻 `glfwSwapInterval` |
| 全屏 | 开 / 关 | 立刻 `glfwSetWindowMonitor`；关时回到 `windowX/Y` |

**第一版不做**：音量（没音频库）、渲染距离以外的质量档、改键。

暂停里改视距：`World::Update` 被停住，所以要在 Apply 时仍允许一次 `enqueueMissingChunks` / `unloadDistantChunks`，或者回到 `Playing` 后下一帧再生效——选 **下一帧生效** 更简单，设置里写一句「继续游戏后生效」即可。FOV / 灵敏度 / vsync 可以立即生效。

---

## 没有字体怎么办

菜单、数字、种子、存档名都要字。阶段 12 的「坐标 / FPS 字体」太晚。

**本阶段加一张 ASCII 点阵图**，不引入 FreeType。

```text
resources/ui/font_ascii.png    16×16 格，覆盖 ASCII 32–126
```

`HudRenderer`（可由现在的 `HotbarRenderer` 抽公共 quad + 着色器）增加 `drawText(x, y, str, scale, color)`。

约束：

- 存档名、种子 **只允许 ASCII**（无 IME）。
- 按钮文案第一版用英文（`Resume` / `Worlds` / `Settings`…），避免再做中文图集。
- 以后要中文标签，再加一张预烘焙短语图，不换输入方案。

---

## 存档布局

```text
saves/
  settings.cfg                 # 全局设置 + LastWorld
  worlds/
    <folder>/
      world.dat                # 头：名字、种子、玩家、快捷栏
      c.<cx>.<cz>              # 仅「被改过」的整列
```

不加 region。一列 `16×128×16×(id+meta)` = 64 KB，改过的列不会太多。

### `settings.cfg`（行文本，风格同 `.block`）

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

缺文件或缺字段 → 用 `Config` 默认值。写失败不崩游戏。

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
```

- `Player`：脚底坐标（与现在 `Player::setPosition` 一致）。
- `Look`：yaw / pitch。
- `Hotbar`：9 个 `BlockId` 整型。以后阶段 8 背包往后面加行，靠 `Version` 兼容。
- **不存光图**（见 `LIGHTING.md`）。不存 Chunk 本身。

`Version` 不够新就拒绝加载并在列表里标损坏，不要猜。

### 列文件 `c.<cx>.<cz>`

二进制：

```text
u32 magic = 'MCCH'
u32 version = 1
i32 cx, cz
u8  id[CHUNK_VOLUME * CHUNK_SECTIONS]
u8  meta[CHUNK_VOLUME * CHUNK_SECTIONS]
```

整列都写（含空气）。加载：文件在就整列灌进 `setBlockRaw`，**不要再跑 `fillChunk`**；文件不在就程序化生成。装饰必须由种子决定（现有生成器已满足），否则「只存脏列」会丢树。

### 脏标记

现在 `Section::m_dirty` 只表示「要重建 mesh」，**不能**当「要写盘」。

`Chunk` 加 `m_modified`：该列任意 `setBlock`（玩家 / 流水）置位。`setBlockRaw` / `fillChunk` **不**置位。

- 超出 `UNLOAD_DISTANCE`：`modified` 则写 `c.cx.cz`，然后 `erase`。
- 切档 / 回标题 / 退出：所有已加载且 `modified` 的列 + `world.dat` 都写完，再销毁 `World`。

---

## 和运行时怎么接

现在 `World(int seed)` 立刻开 worker；析构 `join`。切档必须在主线程：

```text
flushWorld()                  // 脏列 + world.dat + LastPlayed
m_World.reset()               // join worker、放 GPU mesh
m_World = make_unique<World>(seed, saveDir)
读 world.dat → Player / Camera / Hotbar
m_World->Update(cameraPos)    // 开始流式加载
```

`World` 构造改为 `(seed, savePath)`：`fillChunk` 前先看磁盘有没有该列。

`Game::Init` **不再** `make_unique<World>(114514)`，只初始化渲染器和标题相机。

主循环按状态分流：

```text
poll input
if Playing: ProcessGameplayInput; Update player + world
else:       ProcessMenuInput; 不更新世界
render world if World 存在
render menu overlay if 不是 Playing
```

挖放、滚轮、快捷栏数字键只在 `Playing` 且鼠标已捕获时生效。

---

## 视距必须改成运行时

`Config::renderDistance` 已有，但世界用的是编译期 `RENDER_DISTANCE`。设置要生效，需要：

- `World` 持有 `m_renderDistance`，`UNLOAD = render + 2`
- 雾、花草 LOD 按当前视距算（与现在公式相同：起雾 0.55×、结束 0.92×）
- `Camera` 远平面现在是 1000，视距 16×16=256，够用；以后若加大再跟视距挂钩

FOV 已经走 `Camera::Zoom`，设置里改 Zoom 并 `updateMatrices` 即可。灵敏度改 `Camera::MouseSensitivity`。

---

## 控件与绘制

不要上完整 UI 库。在 `UI/` 做一套立即模式，每帧用鼠标位置 + 点击边沿生成：

```text
MenuContext   分辨率、鼠标、点击、字符输入、ESC
Button / Slider / Toggle / TextField / Label / Panel
```

绘制全部走现有 `hud.vert` / `hud.frag`（色块 + 可选纹理）。`HotbarRenderer` 的 ortho quad 抽到 `HudRenderer`，快捷栏、菜单、暂停暗幕共用。

输入：`Main.cpp` 把 `glfwSetCharCallback` 接到文本框（ASCII）。菜单里方向键可选，第一版鼠标够用。

---

## 实现切片

### 刀 0 — 点阵字 + 暂停壳

- ASCII 字体、`HudRenderer::drawText` / `drawQuad`
- 状态机：`Playing` / `Paused`；ESC 切换；暂停停模拟、松鼠标
- 暂停菜单先只有「继续 / 退出游戏」（退出仍不存盘）
- 验收：ESC 不再关窗口；暂停时人不会掉下去

### 刀 1 — 块落盘（无 UI 也能测）

- `Chunk::m_modified`、列文件、`world.dat`、退出 flush
- 临时仍可用单一默认档 `saves/worlds/default`
- 验收：挖坑 → 走远卸载 → 走回坑在；关游戏再开人也在

### 刀 2 — 多存档菜单

- Worlds / CreateWorld；随机种子与手输种子；加载 / 删除
- 切档 flush + 重建 `World`
- 启动进 Title，不自动建世界
- 验收：两个不同种子的档地形不同；切过去再切回来改过的块还在

### 刀 3 — 设置页

- 读写真 `settings.cfg`
- FOV / 灵敏度 / vsync 立即生效；视距运行时化，继续游戏后生效
- 验收：改 FOV 暂停里就能看出投影变化；视距拉低后远处 Chunk 卸载

---

## 完成标准

1. 启动是标题，不是直接进 `114514` 世界。
2. 能新建：随机种子、指定种子（例如 `114514`）各一个，地形可复现。
3. 能从列表加载、删除；正在玩的档被删则回标题。
4. 游戏中 ESC 暂停；继续后鼠标捕获、模拟恢复。
5. 挖坑、切到另一个档、再切回来，坑还在；关游戏再开同样。
6. 设置里改 FOV / 灵敏度 / 视距，重启后配置还在。

---

## 主要改动面

| 模块 | 改什么 |
|---|---|
| 新建 `docs` 对应实现：`World/WorldSave.*` | `world.dat` + 列文件读写、扫档 |
| `World` | 带 `savePath`；加载时优先读列；`m_modified`；`flush`；运行时视距 |
| `Chunk` | `m_modified`，与 mesh dirty 分开 |
| `Game` | 状态机；Init 不建世界；切档 |
| `Main.cpp` | ESC / 点击 / 字符回调按状态分流；vsync / 全屏 |
| `Config.h` | 与 `settings.cfg` 对齐；灵敏度 |
| 新建 `UI/Menu.*`、`Renderer/HudRenderer.*` | 立即模式控件 + 点阵字 |
| `HotbarRenderer` | 复用 HudRenderer |
| `resources/ui/font_ascii.png` | ASCII 字图 |

---

## 和总路线的关系

| 阶段 | 关系 |
|---|---|
| **6 持久化** | 本文件就是阶段 6 的 UI + 存档方案；先刀 0–1 再刀 2–3 |
| **7 洞穴** | 生成变了不能读旧列文件当「未修改」。冻格式后再改生成，或升 `world.dat` Version 并当作全图失效（只信头文件里的玩家，列文件丢掉重生成） |
| **8 背包** | `world.dat` 加行；`Version` +1 |
| **9 光照** | 不存光图；`world.dat` 以后可加 `GameTime` |
| **12** | 「选世界 UI」不再后置；「字体 HUD」仍可后置（坐标 / FPS），菜单字体本阶段就要有 |
