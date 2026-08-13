# 真光照规划

v0.1.0 已按本规划落地。摘要见 [OVERALL.md](OVERALL.md)。

---

## 目标

- 白天地表亮，树荫略暗，水下比空气暗一档
- 地下接近黑，只有洞口附近漏阳光
- 火把周围一圈亮，拆掉立刻变暗
- 傍晚天空 / 雾变色，户外变暗，**火把亮度不变**
- 保留方向光 × 四角 AO，乘在光值上

## 不做

- 阴影贴图、Phong、体积光、延迟渲染
- 把光写进 `ChunkBlock::meta`（已被水位占用）
- 昼夜每帧重建全图 mesh
- 持久化光图（存档只存 `id+meta`，读档后重算）
- 壁挂火把 / 十字 mesh（整立方体即可）

---

## 公式

```text
shade     = AO × cardinal
sky01     = brightness(sky 0–15)
block01   = brightness(block 0–15)
light     = max(sky01 × dayFactor, block01)
color.rgb = albedo × shade × light
```

`dayFactor` 是 shader uniform（白天 1、午夜约 0.05），日落不 remesh。

```text
brightness(t) = mix(0.04, 1.0, t * t)    // t = level / 15
```

不要把 `max(sky, block)` 烤进一个 float，否则夜晚火把和月光糊在一起，只能整图重建。

AO 只给实心立方体面；水 / 花草用该格光值 × cardinal，可不做 AO。

---

## 数据

每个 `ChunkSection` 两张 16³ 图，不碰 `ChunkBlock`：

```text
uint8_t skyLight[4096];    // 0–15
uint8_t blockLight[4096];  // 0–15
```

接口与 `getBlockLocked` 同一套边界：

```text
World::getSkyLight / getBlockLight / setSkyLight / setBlockLight
```

未加载列阳光 / 方块光当 0，邻居加载后再传播一次。

`.block` 增加两行。`isOpaque` 只管面剔除 / AO，**不要当滤光**——树叶 `Opaque 0`，但仍应挡一格阳光。

| 字段 | 空气 / 花草 | 水 | 树叶 | 石头等固体 | 火把 |
|---|---|---|---|---|---|
| `lightOpacity` | 0 | 2 | 1 | 15 | 0 |
| `lightEmission` | 0 | 0 | 0 | 0 | 14 |

固体 `opacity = 15` 表示光进不去。

---

## 规则

**阳光**

1. 列从上往下扫：`y = 127` 之上视为 15
2. `opacity == 0`：继承上方的 15
3. `opacity > 0`：`sky = max(0, above - opacity)`，再往下改走传播
4. 水每格 opacity 2，湖底会明显变暗

**传播**（sky / block 各一遍），对 6 邻居：

```text
next = max(0, current - max(1, neighborOpacity))
```

发光方块：`blockLight = emission` 再向外传。固体内部为 0（火把自己除外）。

**建网**

- 采 **面外侧那一格** 的光，不是实心块内部（内部是 0）
- 顶点对 AO 用过的 3 个侧格 + 面外格做平均；跨 Section / Chunk 走邻接查询
- 全空气 Section 可跳过 mesh，但 **光图仍要算**

---

## 管线

光插在 mesh 之前；写光不能和读光建网抢。

```text
生成列
  fillChunk + 装饰
  LightEngine::computeColumn
  插入 m_chunks
  四邻已在则 propagateFromNeighbors + 邻列 remesh

setBlock / 流水
  LightEngine::updateAfterEdit
  受影响 Section markDirty → remesh

建网
  shade = AO × cardinal
  顶点采样外侧 sky / block
```

`buildMesh` 只读光图。写光放在列尚未进地图时，以及主线程改块 / 邻居整合处。

改块用 **局部盒子重算**（中心曼哈顿半径 15+1），不要上增减光 BFS。流水攒脏列，一帧 relight 一次。

---

## 着色器与昼夜

```text
location 2: aShade     // AO × cardinal
location 3: aSky       // 0–15
location 4: aBlock
```

```text
light = max(brightness(aSky/15) * dayFactor, brightness(aBlock/15))
```

`ChunkRenderer` 每帧设 `dayFactor`、`fogColor`；天空色随时间插值。世界 tick 周期 24000，按住 `T` 加速。不必做太阳 / 月亮网格。水下继续用现有雾，不要再做一套水下光。

---

## 火把

整立方体：`BlockId::Torch`，`Opaque 0`、`Collidable 0`、`LightEmission 14`、`LightOpacity 0`。放 / 挖走现有 `setBlock`。以后十字 mesh 或贴墙只改 mesh，不改光。

---

## 步骤

0. **数据**：两张光图默认 sky=15、block=0；读写 opacity / emission；建网仍只用 cardinal × AO
1. **天空柱**：生成后、改块后传播 sky；顶点采外侧光。地面亮、坑里黑、湖底比岸暗、接缝无亮带
2. **方块光**：emission + `max(sky * dayFactor, block)`。火把亮一圈，隔墙不透，拆掉变暗
3. **昼夜**：tick + `dayFactor` + 天空 / 雾变色。户外变暗、洞里火把仍亮、整图不卡（没 remesh）

---

## 坑

1. 采样外侧，不是块内
2. 顶点平均的 4 格必须跨列读；邻居未加载先当 0
3. worker 只读，写光不要放进 `buildMesh`
4. 空 Section 仍要填光图
5. `isOpaque` 与 `lightOpacity` 分开
6. sky / block 两个 attribute 分开，昼夜才能只改 uniform

---

## 改动面

| 模块 | 改什么 |
|---|---|
| `ChunkSection` | `skyLight[]` / `blockLight[]` |
| `World/LightEngine.*` | 天空柱、传播、改块局部重算 |
| `World.cpp` | 生成后算光；改块 / 流体 / 邻居加载后更新 |
| `BlockData` + `.block` | opacity / emission |
| `ChunkSection` 建网 | 顶点 shade + sky + block |
| `ChunkMesh` / `chunk.vert` / `chunk.frag` | 新 attribute；`dayFactor` |
| `ChunkRenderer` / `SkyRenderer` | 每帧时间 uniform |
| `Game` | tick、加速键 `T` |
| 火把 | `BlockId`、图集、快捷栏 |
