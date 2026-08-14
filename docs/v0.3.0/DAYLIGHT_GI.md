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

光图仍是 v0.1.0 的 0–15 天空光 / 方块光。着色器拆成直射 + 间接：

```text
sky01     = brightness(sky 0–15)
block01   = brightness(block 0–15)
N         = 面朝向（顶点 faceId 解码）
ndotl     = max(N · sunDir, 0)          // 十字花草用 wrap
hemi      = mix(0.62, 1.0, 0.5 + 0.5 Ny)
direct    = ndotl × sunColor × sky01
indirect  = hemi × skyLightColor × sky01
torch     = block01 × (1.00, 0.80, 0.52)
light     = max(direct + indirect, torch)
color.rgb = albedo × AO × light
```

`sunColor` / `skyLightColor` 是每帧 uniform（已含昼夜强度）。日落不 remesh。

`brightness` 与 v0.1.0 相同。`dayFactor` 只留给水面着色。

---

## 数据

顶点 `aLight` 为 `vec3(AO, sky, block)`，另传 `aNormal`（面朝向）。花草用对角法线，双面 wrap。

`LightEngine`、存档、opacity / emission **不动**。

---

## 太阳

tick 0 从东边（+X）升起，正午在南边高空（约 50°，不在天顶），12000 西落：

```text
a      = tick / 24000 × 2π
sunDir = normalize(cos(a), sin(a)×0.72, sin(a)×0.70)
```

正午太阳偏南，南墙亮、北墙暗。若正午在天顶，四面墙直射都是 0，看起来就像「只画了太阳、方块不受光」。

太阳低于地平线时 `sunColor → 0`，户外只剩夜空间接光（约 `NIGHT_DAY_FACTOR`）。

黄昏 / 黎明用现有天空色给 `skyLightColor` 加暖，直射改成橙红。天空里的太阳 / 月亮圆盘和这套方向一致。

---

## 管线

```text
生成 / 改块
  LightEngine 照旧（天空柱 + 六向传播）

建网
  shade = AO（不再乘 cardinal）
  顶点写 AO + sky + block + faceId

每帧
  Atmosphere 算 sunDir / sunColor / skyLightColor
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
| `WorldConstants` / `World::getAtmosphere` | `sunDir`、`sunColor`、`skyLightColor`、日月圆盘 |
| `ChunkMesh` / `ChunkSection` | `aLight` 四分量；AO 与 cardinal 拆开 |
| `chunk.vert` / `chunk.frag` | Lambert + 天空 GI；火把暖色 |
| `sky.frag` / `SkyRenderer` | 太阳 / 月亮圆盘 |
| `ChunkRenderer` | 每帧太阳 / 天空光 uniform |
