# 真光照方案

> 从 `docs/ROADMAP.md` 拆出的阶段 9，加上对现有 cardinal / AO 的分析和落地设计。  
> 依赖：**阶段 6 持久化**（存档不存光图）、**阶段 7 洞穴**（暗处才有意义）。可与阶段 8 并行；**阶段 11 实体之前**建议做完。

---

## 现状：现在的光是假的

光照在 **CPU 建网时烘焙进顶点**，不是 Phong / 方向灯 / 阴影。GPU 只做 `贴图 × 顶点亮度`，再叠距离雾。

### 方向光（Cardinal Light）

`ChunkSection.cpp` 里按面朝向写死倍率：

| 面 | 常量 | 倍率 |
|---|---|---|
| 顶 `+Y` | `LIGHT_TOP` | 1.00 |
| 东西 `±X` | `LIGHT_X` | 0.80 |
| 南北 `±Z` | `LIGHT_Z` | 0.65 |
| 底 `-Y` | `LIGHT_BOT` | 0.50 |

水和花草也用同一套；十字花草整片 `LIGHT_TOP`，不做 AO。

### 四角 AO

只给实心立方体面。每个顶点看外侧角上 3 个不透明邻居（`side1` / `side2` / `corner`），得到 0–3 档，再查表乘方向光：

```text
kTable = {0.55, 0.72, 0.86, 1.0}
shade  = kTable[ao] × cardinal
```

`occludesAO` 只认不透明非空气；空气和水不挡。跨 Section / Chunk 走 `getAdjacentBlock` → `world.getBlockLocked`。

### GPU

`ChunkMesh` 把 4 个顶点 float 放到 `location = 2`。`chunk.frag`：

```text
lit = albedo.rgb × CardinalLight
```

然后按相机距离混雾（空气 / 水下两套）。天空是全屏视线渐变，和方块亮度 **互不耦合**。

快捷栏图标走 `hud` / `cube` 着色器，没有这套光。

### 缺口

| 有 | 没有 |
|---|---|
| 六面固定方向阴影 | 阳光从天空向下传播 |
| 顶点 AO | 方块光（火把 0–15） |
| 距离雾 / 水下雾 | 昼夜、阴影 |

挖洞全亮，放火把也不亮。`ChunkBlock::meta` 已被水的 0–7 占用，光不能塞进去。

阶段 7 曾考虑「按头顶实心层数压暗 cardinal」作权宜之计。**不要做**：会改 mesh 公式，真光还得拆掉。洞穴生成之后直接上本文的刀 1。

---

## 目标与不做

**要达到的观感**

- 白天：地表亮、树荫略暗、水下比空气暗一档
- 矿洞：接近黑，只有洞口附近有阳光漏进来
- 火把：周围一圈亮，拆掉立刻变暗
- 傍晚：天空 / 雾变色，户外跟着变暗，**火把亮度不变**
- 阶段 4 的 **方向光 × 四角 AO 全部保留**，乘在光值上

**明确不做**

- 阴影贴图、法线 Phong、体积光、延迟渲染（总路线后置项）
- 把光写进 `ChunkBlock::meta`
- 昼夜每帧重建全图 mesh
- 第一版不持久化光图（读档后重算，避免阶段 6 存档格式再改一次）
- 第一版不做壁挂火把 / 十字 mesh（整立方体即可）

完成标准：白天地表亮、矿洞暗；放下火把周围亮起、拆掉变暗；傍晚天空变色、户外变暗、整图不卡。

---

## 核心公式

每个顶点：

```text
shade     = AO × cardinal          // 现有逻辑，原样留下
sky01     = brightness(sky 0–15)
block01   = brightness(block 0–15)
light     = max(sky01 × dayFactor, block01)
color.rgb = albedo × shade × light
```

`dayFactor` 是 **shader uniform**（白天 1、午夜 ~0.05）。太阳落山不用 remesh。

`brightness` 不要线性，用平方曲线，并留一点底，避免 0 级死黑不好调：

```text
brightness(t) = mix(0.04, 1.0, t * t)    // t = level / 15
```

AO 仍只对实心立方体面；水 / 花草用该格（或脚下一格）的光值 × cardinal，可以继续不做 AO。

**不要**把 `max(sky, block)` 烤进一个 float：夜晚会把火把和月光糊在一起，只能整图重建。

---

## 数据布局

每个 `ChunkSection` 两张 16³ 图，**不要碰 `ChunkBlock`**：

```text
uint8_t skyLight[4096];    // 0–15
uint8_t blockLight[4096];  // 0–15
```

一列 8 个 Section ≈ 64 KB。nibble 打包能省一半，第一版用整字节。

对外接口（跨 Section / Chunk 和 `getBlockLocked` 同一套边界）：

```text
World::getSkyLight / getBlockLight / setSkyLight / setBlockLight
```

未加载列：阳光当 0、方块光当 0（边界会暂时偏暗，邻居加载后再传播一次）。

`BlockData` 加两个字段，`.block` 文件加两行。`isOpaque` 继续给面剔除 / AO 用，**不要拿它当滤光**——树叶已经是 `Opaque 0`，但应该挡一格阳光。

| 字段 | 空气 / 花草 | 水 | 树叶 | 石头等固体 | 火把 |
|---|---|---|---|---|---|
| `lightOpacity` | 0 | 2 | 1 | 15 | 0 |
| `lightEmission` | 0 | 0 | 0 | 0 | 14 |

固体 `opacity = 15` 表示光进不去。

存档（阶段 6）：只存 `id+meta`。光图读档后重算。

---

## 光规则

### 阳光（sky）

1. 列从上往下扫：世界顶 `y = 127` 之上视为 15。
2. `opacity == 0` 的格子：继承上方的 15（开阔天空一直 15）。
3. 碰到 `opacity > 0`：`sky = max(0, above - opacity)`，再往下不再是「直射 15」，改走传播。
4. 水每格 opacity 2，湖底会明显变暗。

### 传播（sky 和 block 各做一遍）

对 6 个邻居：

```text
next = max(0, current - max(1, neighborOpacity))
```

发光方块：`blockLight = emission`，再向外传。固体内部光为 0（火把自己除外）。

### 建网取值

- 画 **某面** 时，采的是 **面外侧那一格** 的光（空气 / 水里的光），不是实心块内部。实心块内部是 0，采自身格会让所有石头面变黑。
- 顶点再对 AO 用过的那 3 个侧格 + 面外格做平均（smooth lighting），跨 Section / Chunk 必须走邻接查询，否则接缝。
- 空的全空气 Section 可能跳过 mesh，但 **光图仍要算**——阳光 / 火把要穿过它传到下一层。

---

## 和现有管线怎么接

现在是：`fillChunk`（worker）→ 进地图 → `buildDirtyMeshes`（worker 只读）→ 主线程上传；`setBlock` 在主线程同步 remesh。

光必须插在 **mesh 之前**，并且 **写光时不能和读光建网抢**。

```text
生成列
  fillChunk + 装饰
  LightEngine::computeColumn(chunk)     // 竖向天空柱 + 列内传播
  插入 m_chunks
  若四邻已在：propagateFromNeighbors + 邻列 markDirty remesh

玩家 / 流水 setBlock
  改方块
  LightEngine::updateAfterEdit(...)
  受影响 Section markDirty → remesh（现有路径）

建网
  shade = AO × cardinal                 // 不变
  每个顶点再采样外侧 4 格的 sky / block
  VBO：shade + sky + block
```

邻居加载后要 **回刷已有列的边界光**，否则先生成的 Chunk 永远吃不到邻列火把 / 阳光。这和现在 `markNeighborsDirty` 是同一类事件，只是多一步 relight。

`buildMesh` 继续只读光图。写光放在 `fillChunk` 末尾（chunk 还没进地图）和主线程 `setBlock` / 邻居整合处。

---

## 改块怎么更新光

标准增减光 BFS 容易写错（漏光、黑斑、跨列死循环）。玩家每秒改不了几格，一列只有 `16×128×16 ≈ 3 万` 格。

**第一版：局部盒子重算**

1. 以改动点为中心，取曼哈顿半径 15 + 1 的盒子（再加上下各 1 个 Section）。
2. 盒子里的 sky / block 清零后重算（天空柱只重算该盒子覆盖的那些 XZ 列）。
3. 盒子贴着 Chunk 边则把邻列对应 Section 标 dirty。

火把半径 14，15 格盒子刚好盖住。数据布局按 0–15 两张图来，以后要换成增量传播也不改着色器和顶点格式。

流水 `setBlockDeferred` 会连续改很多水格：不要每格全量重算。跟流体一样 **攒脏列，一帧结束 relight 一次**。

---

## 着色器与昼夜

`chunk.vert` 现在只有 `aCardinalLight`。改成：

```text
location 2: float aShade      // AO × cardinal
location 3: float aSky        // 0–15，已在 CPU 对 4 格平均
location 4: float aBlock
```

片元：

```text
float skyB   = brightness(aSky / 15.0);
float blockB = brightness(aBlock / 15.0);
float light  = max(skyB * dayFactor, blockB);
vec3 lit = color.rgb * aShade * light;
// 雾逻辑保持不变
```

`ChunkRenderer` 每帧设 `dayFactor`、`fogColor`；`SkyRenderer` 的 top / horizon 随时间插值。

世界时间：`World` 或 `Game` 里一个 `uint64_t tick`，周期例如 24000。第一版可先做一个键加速时间，方便验收。不必做太阳 / 月亮网格。

水下：继续用现有水下雾；光值会让深处更暗，和雾叠在一起即可，不要再做一套水下光。

阶段 11 刷怪只需要 `getSkyLight` / `getBlockLight` 低于阈值，接口已经够。

---

## 火把

按现有加方块流程，第一版 **整立方体**：

1. `BlockId::Torch`
2. `resources/blocks/torch.block`：`Opaque 0`、`Collidable 0`、`LightEmission 14`、`LightOpacity 0`
3. 图集一格；快捷栏加一格
4. 放 / 挖走现有 `setBlock`，光引擎自动亮 / 灭

以后要十字 mesh 或贴墙，只改 mesh，不改光。合成（木棍 + 煤）是阶段 12，不挡验收。

---

## 实现切片

每刀都能跑起来验收。不要在阶段 7 先做权宜压暗。

### 刀 0 — 数据与接口（看不见差别）

- Section 两张光图，默认 sky=15、block=0（世界暂时仍全亮）
- `BlockData` 读写 `LightOpacity` / `LightEmission`
- 现有方块补默认值：固体 15/0，空气花草 0/0，水 2/0，树叶 1/0
- 建网仍只用 cardinal × AO

### 刀 1 — 天空柱 + 建网采样（洞穴变黑，无火把）

- 生成后、`setBlock` 后跑天空柱 + sky 传播
- 顶点采外侧光：`shade × brightness(sky)`
- 验收：白天地面亮；挖坑 / 进矿洞是黑的；湖底比岸上暗；Chunk 接缝无明显亮带

### 刀 2 — 方块光 + 火把

- block 传播 + emission
- shader：`max(sky * dayFactor, block)`（此时 `dayFactor = 1`）
- 验收：放下火把亮一圈；挖掉变暗；隔墙不透；水和树叶衰减正确

### 刀 3 — 昼夜

- 世界 tick + `dayFactor` 曲线（黄昏短、夜晚略长更好看）
- 天空 / 雾颜色随时间
- 验收：户外变暗、洞里火把仍亮、天空变晚霞再变黑；**整图不卡**（证明没 remesh）

---

## 和总路线的关系

| 阶段 | 对光照的含义 |
|---|---|
| **6 持久化** | 存档只存 `id+meta`。光图读档后重算。现在就把格式冻住。 |
| **7 洞穴** | 没有真光的话矿洞是亮的。洞穴生成之后接刀 1。 |
| **8 物品** | 火把要能进快捷栏 / 掉落，但光引擎不依赖背包。创造模式先放方块也能验收刀 2。 |
| **10 / 11** | 游泳、暗处刷怪只读光值，不要反过来改光结构。 |

---

## 会碰到的坑

1. **采样外侧，不是块内** — 实心块内部是 0。
2. **接缝** — 顶点平均的 4 格必须跨列读，和 AO 同一套。邻居还没加载时先当 0，加载后再 relight + remesh。
3. **worker 只读** — 写光不要放进 `buildMesh`。
4. **空 Section** — 仍要填光图。
5. **叶子 vs 不透明** — `isOpaque` 管面剔除，`lightOpacity` 管光，两套字段分开。
6. **两个 attribute 分开存 sky / block** — 昼夜才能只改 uniform。

---

## 主要改动面

| 模块 | 改什么 |
|---|---|
| `ChunkSection` | `skyLight[]` / `blockLight[]` |
| 新建 `World/LightEngine.*` | 天空柱、传播、改块局部重算 |
| `World.cpp` | 生成后算光；`setBlock` / 流体 / 邻居加载后更新光 |
| `BlockData` + `.block` | opacity / emission |
| `ChunkSection.cpp` 建网 | 顶点 shade + sky + block |
| `ChunkMesh` / `chunk.vert` / `chunk.frag` | 两个新 attribute；`dayFactor` |
| `ChunkRenderer` / `SkyRenderer` | 每帧时间 uniform |
| `Game` | tick、可选加速键 |
| 火把 | `BlockId`、图集、快捷栏 |
