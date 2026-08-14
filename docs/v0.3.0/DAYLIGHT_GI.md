# 日光全局光照（beta）

v0.3.0 已按本规划落地。摘要见 [OVERALL.md](OVERALL.md)。

---

## 目标

- 上午东墙亮、下午西墙亮；正午顶面最亮
- 树荫、屋檐、北墙偏天空色，不是灰一把
- 黄昏直射变暖、变斜；火把亮度仍不随时间变
- 洞里还是黑（`skyLight = 0`）
- 转一天不卡：不 remesh、不重算光图

## 不做

- 阴影贴图 / CSM / Phong 高光
- 按太阳方向重算天空柱
- VXGI、SSGI、光追
- 彩色弹射（草地把光染绿）

---

## 公式

光图仍是 v0.1.0 的 0–15 天空光 / 方块光。着色器拆成直射 + 间接。`chunk.frag` 实际是：

```text
sky01     = brightness(sky 0–15)
block01   = brightness(block 0–15)
N         = aNormal（建网时由 faceId 转成法线）
sunN      = max(N · sunDir, 0)
moonN     = max(N · moonDir, 0)
hemi      = mix(0.50, 1.0, 0.5 + 0.5 Ny)
daylight  = sky01 × (skyLightColor × hemi + sunColor × sunN + moonColor × moonN)
torch     = block01 × (1.00, 0.80, 0.52)
light     = max(daylight, torch)
color.rgb = albedo × AO × light
```

十字花草（对角法线）用 wrap，太阳 / 月亮都是 `0.35 + 0.65 × abs(N · dir)`，双面都能接到光。

```text
brightness(t) = mix(0.12, 1.0, t² × (2 − t))    // t = level / 15
```

这条曲线和 v0.1.0 的 `mix(0.04, 1.0, t²)` 不同：暗部抬高一点，中间更平滑。

`sunColor` / `moonColor` / `skyLightColor` 是每帧 uniform（已含昼夜强度）。日落不 remesh。

`dayFactor` 给水面着色，以及关掉太阳/月亮时的旧方向明暗。日光 GI 开着时，户外明暗走 `sunColor` / `skyLightColor`，不再乘 `dayFactor`。

设置里关掉 **Sun / Moon** 后 `celestial = 0`，退回 v0.1.0 那套 `max(sky × dayFactor, block) × cardinal`。

---

## 数据

顶点 `aLight` 为 `vec3(AO, sky, block)`，另传 `aNormal`。建网仍用 `faceId`，CPU 侧转成法线再上传；着色器不再解码 faceId。花草用对角法线。

`LightEngine`、存档、opacity / emission **不动**。

---

## 太阳

tick 0 从东边（+X）升起，正午在南边高空（约 50°，不在天顶），12000 西落：

```text
a       = tick / 24000 × 2π
sunDir  = normalize(cos(a), sin(a)×0.72, sin(a)×0.70)
moonDir = normalize(−sunDir)
```

正午太阳偏南，南墙亮、北墙暗。若正午在天顶，四面墙直射都是 0，看起来就像「只画了太阳、方块不受光」。

太阳低于地平线时 `sunColor → 0`，月亮升起，`moonColor` 按 `MOON_STRENGTH` 给一点冷光。户外间接光仍按 `NIGHT_DAY_FACTOR` 压暗，不会全黑。

黄昏 / 黎明用现有天空色给 `skyLightColor` 加暖，直射改成橙红。天空里的太阳 / 月亮圆盘和这套方向一致。

设置里打开 **Fixed Noon** 后把 tick 锁在 6000，时间不再走，`T` 也无效。

---

## 管线

```text
生成 / 改块
  LightEngine 照旧（天空柱 + 六向传播）

建网
  shade = AO（不再乘 cardinal）
  顶点写 AO + sky + block；faceId → aNormal

每帧
  Atmosphere 算 sunDir / moonDir / sunColor / moonColor / skyLightColor
  chunk.frag / sky.frag 只改 uniform
```

---

## 验收

- 按住 `T`：东墙先亮，正午顶面最亮，西墙后亮；网格不重建
- 树下、屋檐比开敞地暗，且偏天空色
- 夜里户外暗、火把仍暖黄；洞里无天空光
- 抬头能看到太阳（白天，偏南）或月亮（夜里），南墙比北墙亮
- 挖 / 放 / 流水后光缝仍正确（光图规则没改）

---

## 改动面

| 模块 | 改什么 |
|------|--------|
| `WorldConstants` / `World::getAtmosphere` | `sunDir`、`moonDir`、`sunColor`、`moonColor`、`skyLightColor`、日月圆盘 |
| `ChunkMesh` / `ChunkSection` | `aLight` 为 `vec3(AO, sky, block)`；`aNormal` 由 faceId 转出；AO 与 cardinal 拆开 |
| `chunk.vert` / `chunk.frag` | Lambert + 天空 GI + 月光；火把暖色 |
| `sky.frag` / `SkyRenderer` | 太阳 / 月亮圆盘 |
| `ChunkRenderer` | 每帧太阳 / 月亮 / 天空光 uniform |
| `Game` / `Config` | Sun / Moon 开关；Fixed Noon |
