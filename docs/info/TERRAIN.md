# 地形设计

仍是 2D 高度图：先算地面高度 `Y`，再纵向填方块。不上 3D 密度、不做洞穴。

实现：`World/TerrainGenerator.cpp`，常量在 `World/WorldConstants.h`。

同一种子、同一列坐标，气候和高度可复现。未写入磁盘的列会按生成器重算；旧档脏列不迁移，建议新开世界看本版地形。

---

## 气候：三张 2D 噪声

三张**不相关**的 salted octave 场，值域约 `[-1, 1]`。陆地群系由 `(C, T, M)` 查表，不用一条噪声硬切。

| 场 | 符号 | 尺度 | 约一块有多宽 | 作用 |
|---|---|---|---|---|
| 大陆度 | `C` | `CONTINENT_SCALE = 0.004` | ~250 格 | 海还是陆 |
| 温度 | `T` | `TEMPERATURE_SCALE = 0.003` | ~330 格 | 热 / 温 / 寒 |
| 湿度 | `M` | `MOISTURE_SCALE = 0.004` | ~250 格 | 干 / 湿；河网、植物 |

尺度越小，同一气候铺得越开。温度场用 3 层 octave，大陆度和湿度用 4 层。

海陆交界在 `C ∈ [CONTINENT_OCEAN_TH ± BIOME_BLEND]`，即约 `[-0.36, -0.20]`，只混**高度**。表面方块跟群系走（森林草可以直接接到水边）。

---

## 每种地形的参数范围

噪声值大约都在 `[-1, 1]`。群系由 `(C, T, M)` **同时**决定。

陆地（`C ≥ -0.28`）按温度 × 湿度：

| | **`M < 0` 干** | **`M ≥ 0` 湿** |
|---|---|---|
| **`T ≥ 0.25` 热** | Desert | Forest |
| **`-0.25 ≤ T < 0.25` 温** | Savanna | Forest |
| **`T < -0.25` 寒** | Tundra | Tundra |

完整范围：

| 地形 | 大陆度 `C` | 温度 `T` | 湿度 `M` |
|---|---|---|---|
| Ocean | `[-1, -0.28)` | 任意 | 任意（不算植物） |
| Forest | `[-0.28, 1]` | `[-0.25, 1]` | `[0, 1]` |
| Savanna | `[-0.28, 1]` | `[-0.25, 0.25)` | `[-1, 0)` |
| Desert | `[-0.28, 1]` | `[0.25, 1]` | `[-1, 0)` |
| 深沙漠（无植物） | 同上 | 同上 | `[-1, -0.50)` |
| Tundra | `[-0.28, 1]` | `[-1, -0.25)` | 任意 |

对应代码：`TerrainGenerator::biomeFromClimate`。

`±0.25` 只决定热 / 温 / 寒各占多少比例，不决定斑块在世界上有多大。要让寒带、热带铺得更开，改 `TEMPERATURE_SCALE`（更小 → 更大片），不要拉开阈值。

---

## 群系外观

| `BiomeType` | 表面 | 柱子（上→下） | 起伏 |
|---|---|---|---|
| Ocean | 沙 | 沙 → 石 | 浅海盆，约海平面以下 |
| Forest | 绿草 `Grass` | 草 → 土 → 石 | 丘陵 + 可出大山 |
| Savanna | `SavannaGrass` | 草 → 土 → 石 | 缓丘，山很小 |
| Desert | 沙 | 沙（约 3～4 格）→ 沙砖（约 8 格）→ 石 | 沙丘级，山幅度 ≈ 0 |
| Tundra | `Snow` | 雪 → 土 → 石 | 丘陵，山比森林矮一截 |

河流不是独立群系，是叠在陆地上的雕刻。

岸边不强制沙子：Ocean 柱水下仍是沙；Desert 碰到海也是沙；Forest / Savanna 草接到水边；Tundra 雪接到冰面。

---

## 高度

先算共用的平原 / 丘陵 / 山脉，再乘群系幅度后相加。交界对 amp 做 `smoothstepRange` 插值，避免沙漠–森林突然少几十格。

```text
h = WATER_LEVEL + 2
  + plains
  + hills     * hillAmp
  + mountains * mountAmp
  + desertW   * dunes * 2.5
```

| 群系 | hillAmp | mountAmp |
|---|---|---|
| Forest | 1.0 | 1.0 |
| Tundra | 0.8 | 0.5 |
| Savanna | 0.6 | 0.15 |
| Desert | 0.35 | 0 |
| Ocean | — | 用 `oceanHeight`，再与陆地高度按 `C` 混合 |

`WATER_LEVEL = 62`。高度钳在 `[2, WORLD_HEIGHT - 16]`。

---

## 柱状填充

```text
y > height:
    y <= WATER_LEVEL → 水；若 Tundra 且 y == WATER_LEVEL → 冰
    否则空气
y == height:
    Ocean / Desert → 沙
    Forest → Grass
    Savanna → SavannaGrass
    Tundra → Snow
height-3 .. height-1:     沙漠 / 海洋 → 沙；其它 → 土
height-11 .. height-4:    沙漠 → 沙砖；其它 → 石
再往下：石头
```

`SUBSOIL_LAYERS = 3`，`SANDSTONE_LAYERS = 8`。

---

## 河流

脊线噪声，不是群系。`ridge = 1 - |n|`（1 = 谷底）。坐标先 warp 再采样，让河道弯。

```text
mask = smoothstep(ridgeLo, ridgeHi, ridge)
     * landMask              // 不上大洋
     * (1 - mountMask * mountAmp)  // 不上实际高峰
     * moistureWiden         // 湿润更密，沙漠很稀
```

只降低、不抬高：`height = lerp(height, WATER_LEVEL - 1, mask)`（仅当当前高度高于目标）。

然后 `y > height && y <= WATER_LEVEL` 灌**源水**（`meta = 0`），不跑流体扩散。河宽大约 3～7 格。

---

## 冰和雪

只由 **Tundra** 决定，不跟昼夜挂钩。

- 地表整格 `Snow`，实心可碰撞
- 有水时只把最上一格（`y == WATER_LEVEL`）换成 `Ice`，下面仍是水
- `Ice` 实心、可站立、`Water::isWater` 为假，`lightOpacity = 2`
- 不结整柱冰

---

## 植物

`World::placeDecorations`。`roll ∈ [0, 1)`，放置当且仅当 `roll < base * wet`。

```text
wet = clamp(0.35 + 0.65 * (M+1)/2, 0.2, 1)
```

| 群系 | 树 | 地面 |
|---|---|---|
| Forest | 橡树，`base ≈ 1/18`，高处再稀 | 高草 `1/28`，玫瑰 `1/120` |
| Savanna | 草原树，`base ≈ 1/90`，树干 3～5 | 高草 `1/48`，无玫瑰 |
| Desert | 无 | 仅边缘：仙人掌 `1/64`、枯灌 `1/32`；`M < -0.50` 无 |
| Tundra / Ocean / 河面 | 无 | 无 |

树只站在对应的草上，且 `surfY > WATER_LEVEL`。Savanna 树外形同橡树，方块为 `SavannaBark` / `SavannaLeaf`。

---

## 相关方块

`BlockId` 追加在 `Torch` 后面，不改已有 id：`Sandstone, Ice, Snow, SavannaGrass, SavannaBark, SavannaLeaf`。
