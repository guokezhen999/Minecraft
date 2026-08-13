# 地形规划（v0.2.0）

仍是 2D：先算地面高度 Y，再纵向填方块。不上 3D 密度、不做洞穴。

---

## 目标

- 岸边不强制沙子
- 沙漠：沙 → 沙砖 → 石头；没有大山
- 寒带：雪地表，海 / 河最上一格水结冰
- 稀树草原：绿偏黄的草和叶、橙色树干、树很稀
- 现有绿色森林保留（就是现在的 Grassland）
- 陆地有河流；植物密度跟气候走

## 不做

3D 洞穴、季节融冰、薄雪层、河水动力学、旧档地形迁移。

---

## 气候：三张 2D 噪声

现在只有一条 `climate` 切 Ocean / Grassland / Desert，起伏和气候几乎无关，所以沙漠里会长山，湖边一律是沙。

改成三个**不相关**的 salted octave 场（和现有 `saltedOctaveNoise` 一样，换 salt）：

| 场 | 符号 | 尺度（约） | 作用 |
|---|---|---|---|
| 大陆度 | `C` | 0.004，很大块 | 海还是陆 |
| 温度 | `T` | 0.006 | 热 / 温 / 寒 |
| 湿度 | `M` | 0.008 | 干 / 湿；也管河网疏密和植物 |

值域都归一化到 `[-1, 1]`。陆地群系由 `(T, M)` 查表，**不要**再用一条噪声的阈值硬切三种。

```text
C < -0.28                → Ocean          （与陆地在 ±BIOME_BLEND 里混高度）
C ≥ -0.28 且看 (T, M)：

            M < 0 干              M ≥ 0 湿
T ≥  0.25   Desert                Forest
|T| < 0.25  Savanna（草原）        Forest
T < -0.25   Tundra                Tundra
```

交界用现有的 `smoothstepRange` 混，避免群系方块悬崖。Ocean ↔ 陆地只混**高度**，表面方块跟群系走（森林可以直接长到水边）。

湿度在海洋上不算植物。沙漠内部 `M` 越低越贫瘠（沿用「深沙漠无仙人掌」）。

---

## 群系

| `BiomeType` | 表面 | 柱子（上→下） | 起伏 |
|---|---|---|---|
| Ocean | 沙 | 沙 → 石 | 浅海盆，约海平面以下 |
| Forest | 绿草（现有 Grass） | 草 → 土 → 石 | 丘陵 + 可出大山 |
| Savanna | 新 SavannaGrass | 草 → 土 → 石 | 缓丘，山很小 |
| Desert | 沙 | 沙（约 3～4 格）→ 沙砖（约 8 格）→ 石 | 沙丘级，**山幅度 ≈ 0** |
| Tundra | 新 Snow | 雪 → 土 → 石 | 丘陵，山比森林矮一截 |

河流不是独立群系，是叠在陆地上的雕刻，见下文。

### 高度 = 起伏 × 群系幅度

先算现在的 `landHeight` 三件套（平原抖动、丘陵、山脉），再乘群系系数后相加：

```text
h = WATER_LEVEL + 2
  + plains
  + hills     * hillAmp[biome]
  + mountains * mountAmp[biome]
```

| 群系 | hillAmp | mountAmp |
|---|---|---|
| Forest | 1.0 | 1.0 |
| Tundra | 0.8 | 0.5 |
| Savanna | 0.6 | 0.15 |
| Desert | 0.35 | **0**（只留很矮的沙丘噪声） |
| Ocean | — | 用现在的 `oceanHeight` |

混界处对 amp 做插值，不要在沙漠—森林边界突然少 40 格高度。

沙漠额外：`+ dunes * 2.5`（现有逻辑可留）。

### 岸边沙子

删掉 Forest / Savanna 里 `height <= WATER_LEVEL + 1 → Sand`。

- Ocean 柱：水下仍是沙
- Desert：表面仍是沙，碰到海也是沙
- Forest / Savanna：草可以接到水边
- Tundra：雪接到冰面

---

## 河流

用 **脊线** 而不是再开一个群系：

```text
n = saltedOctaveNoise(x * 0.0035, z * 0.0035, 4, 2.0, 0.5, riverSalt)
ridge = 1 - abs(n)                         // 1 = 谷底中心
warp 后再采一次，让河道弯
mask = smoothstep(0.90, 0.97, ridge)
     * landMask                            // 不上大洋
     * (1 - mountMask)                     // 不上高峰
     * moistureWiden                       // 湿润更宽更密，沙漠很稀
```

`moistureWiden`：`M` 高则降低阈值（河更密）、略加宽；`M` 很低则几乎没河。

雕刻（只降低、不抬高）：

```text
target = WATER_LEVEL - 1
height = lerp(height, target, mask)
```

然后该柱 `y > height && y <= WATER_LEVEL` 灌水。寒带这些水的**最上一格**改成冰（见下）。

生成时放 **源水**（`meta = 0`）即可，不跑流体扩散。河宽大约 3～7 格。

---

## 冰和雪

只由 **Tundra** 决定，不跟昼夜挂钩。

- 地表：`y == height` → Snow（整格实心，可碰撞）
- 其下几格土，再往下石头
- 该柱若有水（海、湖、河）：只把 **最上面那一格水** 换成 Ice，下面仍是水
- Ice：实心、可站立、不是流体（`Water::isWater` 为假），`lightOpacity` 建议 2
- 不结整柱冰，避免挖不开的冰海

---

## 植物密度

装饰仍在 `World::placeDecorations`，但阈值改成 **群系表 + 湿度**，不要所有 Grassland 共用一套。

```text
roll = hash(wx, wz) ∈ [0,1)
放置 iff roll < base * wet
wet = clamp(0.35 + 0.65 * (M+1)/2, 0.2, 1)   // 干少湿多
```

| 群系 | 树 | 地面 |
|---|---|---|
| Forest | 橡树，`base ≈ 1/18`，高处再稀一点 | 高草 `1/28`，玫瑰 `1/120` |
| Savanna | 草原树，`base ≈ 1/90`（明显稀） | 高草 `1/48`，无玫瑰 |
| Desert | 无树 | 仅边缘：仙人掌 `1/64`、枯灌 `1/32`；深沙漠无 |
| Tundra | 无 | 无 |
| Ocean / 河面 | 无 | 无 |

树只站在对应的草上，且 `surfY > WATER_LEVEL`（不种进水里）。河岸跟邻接群系，不单独种。

Savanna 树：复用现在的橡树外形（矮一点，树干 3～5），方块改成 `SavannaBark` / `SavannaLeaf`。

---

## 新方块与贴图

`defaultPack.png` 仍是 256×256、16×16 一格。现占用：

```text
行 0: 0草顶 1草侧 2土 3石 4橡树侧 5橡树顶 6橡叶 7沙 8水 9仙人掌顶 10玫瑰 11高草 12枯灌
行 1: 0火把          9仙人掌侧
```

建议空位（实现时可微调，`.block` 写死坐标）：

| 格 | 用途 |
|---|---|
| 13,0 | 沙砖 |
| 14,0 | 冰 |
| 15,0 | 雪顶 |
| 1,1 | 雪侧 |
| 2,1 | 草原草顶（绿偏黄） |
| 3,1 | 草原草侧 |
| 4,1 | 草原树干侧（偏橙） |
| 5,1 | 草原树干顶 |
| 6,1 | 草原树叶（绿偏黄） |

`BlockId` 接在 `Torch` 后面追加，不要改已有 id（旧存档脏列才能对上）：

```text
Sandstone, Ice, Snow, SavannaGrass, SavannaBark, SavannaLeaf
```

| 方块 | 碰撞 | 不透明 | 光阻 | mesh |
|---|---|---|---|---|
| Sandstone | 是 | 是 | 15 | 立方体 |
| Ice | 是 | 建议是（避免排序） | 2 | 立方体 |
| Snow | 是 | 是 | 15 | 立方体 |
| SavannaGrass | 是 | 是 | 15 | 立方体，底可用土 2,0 |
| SavannaBark | 是 | 是 | 15 | 立方体 |
| SavannaLeaf | 是 | 否 | 1 | 立方体，shader Flora |

快捷栏仍 9 格：换掉一格次要用的（例如仙人掌或橡叶），让沙砖 / 冰 / 雪 / 草原草能放到。

加方块流程与现在相同：`BlockId` → `resources/blocks/*.block` → `BlockDatabase` → 图画进图集。

---

## 柱状填充（`getBlock`）

```text
y > height:
    y <= WATER_LEVEL → 水；若 Tundra 且这是最上一格水 → 冰
    否则空气
y == height:
    Ocean/Desert → 沙
    Forest → Grass
    Savanna → SavannaGrass
    Tundra → Snow
height-3..height-1:
    Desert → 沙
    Tundra → 土
    其它陆地 → 土
height-11..height-4:
    Desert → 沙砖
    其它 → 土（若还在土层）或石头
再往下：石头
```

厚度用常量，混界处不要突变。

---

## 存档

列格式不变（`id + meta`）。新 id 追加在后面。

**未写入磁盘的列**会按新生成器重算，和旁边旧脏列可能接不上。文档和菜单不强制升级：建议新开世界。若以后要冻地形，再升 `world.dat` Version。

---

## 步骤

0. **方块与贴图**：六种新方块、图集格、`.block`、数据库、快捷栏能拿。创造里能放下、能看见。
1. **气候 + 高度 + 岸边**：三噪声、五群系、沙漠无高山、去掉草地强制沙滩、沙漠沙砖层。飞一圈能分出热干 / 温湿 / 寒冷，沙漠没有森林那种高峰。
2. **寒带冰雪**：Tundra 雪地表；海、河最上一格冰，冰上能走。
3. **稀树草原**：黄绿草 / 橙树干 / 黄绿叶，树明显稀于森林。
4. **河流 + 植物表**：陆地有弯河；湿润多河、沙漠少河；高峰无河；植物密度按上表。

---

## 验收

1. 森林草直接接到湖 / 海边，不再一圈强制沙
2. 沙漠剖面能看到沙 → 沙砖 → 石；几乎无大山
3. 寒带一片白，水面一层冰，冰下仍是水
4. 草原草偏黄，树少，树干偏橙
5. 森林仍有绿草、密一些的橡树、可有高山
6. 陆地上有连续的河连向海边或 freeze 成冰河
7. 同一种子地形可复现；深沙漠仍几乎无植物，森林草多树多

---

## 改动面

| 模块 | 改什么 |
|---|---|
| `TerrainGenerator` | C/T/M、群系表、高度幅度、河流雕刻、`getBlock` 分层 |
| `World::placeDecorations` / 种树 | 按群系选方块和密度 |
| `BlockId` + `BlockDatabase` + `.block` | 六种新方块 |
| `resources/textures/defaultPack.png` | 新格 |
| `UI/Hotbar.cpp` | 创造栏能放到新方块 |
| `WorldConstants.h` | 气候尺度、河阈值、沙砖厚度等常量 |
