# 地形规划（v0.2.0）

仍是 2D：先算地面高度 Y，再纵向填方块。不上 3D 密度、不做洞穴。

---

## 目标

- 岸边不强制沙子（温湿地带草地可直接接水）
- 沙漠与恶地：沙 / 陶瓦 → 石头；山体极其平缓或为台地
- 极地雪原：整片积雪，水面结冰（寒漠，不是苔原）
- 苔原：林木线以北的低植被地带——苔藓土 + 残雪 + 碎石，无树，水面不结冰
- 针叶林：云杉林，水面不结冰
- 稀树草原：黄绿草叶、橙色树干、树木稀疏
- 陆地有河流雕刻；植物密度与群系气候严格绑定
- 新增针叶林（Taiga）与热带雨林（Jungle），保留原有温带森林（Forest）

## 不做

3D 洞穴、季节融冰、薄雪层、河水动力学、旧档地形迁移。

---

## 气候：三张 2D 噪声

采用三个**不相关**的 salted octave 场（和现有 `saltedOctaveNoise` 一样，换 salt）：

| 场 | 符号 | 尺度（约） | 作用 |
|---|---|---|---|
| 大陆度 | `C` | 0.004 | 海还是陆 |
| 温度 | `T` | 0.003 | 热 / 温 / 亚寒 / 寒 |
| 湿度 | `M` | 0.004 | 干 / 湿 |

值域都归一化到 `[-1, 1]`。

### 判定逻辑与群系矩阵

```text
C < -0.28 → Ocean （海洋，水下柱子依然是沙）
C ≥ -0.28 且看 (T, M)：
```

| 温度区间 | 湿度区间 | 群系名 (`BiomeType`) | 说明 |
| :--- | :--- | :--- | :--- |
| **$T \ge 0.3$ (热带)** | $M < -0.2$<br>$-0.2 \le M < 0.4$<br>$M \ge 0.4$ | **Desert** (热带沙漠)<br>**Savanna** (热带草原)<br>**Jungle** (热带雨林) | 沙丘，仙人掌<br>稀树，黄绿草<br>巨木，极密植被 |
| **$-0.2 \le T < 0.3$ (温带)** | $M < -0.4$<br>$-0.4 \le M < 0.2$ <br>$M \ge 0.2$ | **TemperateDesert** (温带沙漠)<br>**Grassland** (温带草原)<br>**Forest** (温带森林) | 砾石，荒凉<br>平坦草地，无树<br>橡树林，花草茂盛 |
| **$-0.6 \le T < -0.2$ (亚寒带)** | $M < 0.0$<br>$M \ge 0.0$ | **Tundra** (苔原)<br>**Taiga** (针叶林) | 苔藓土、残雪、碎石；无树<br>云杉林，阴冷多山 |
| **$T < -0.6$ (寒带)** | 任意 $M$ | **SnowyPlains** (极地雪原) | 终年积雪，极寒无树 |

交界处采用 **四段温度 × 湿度的组合权重** 进行 `smoothstepRange` 混合，确保地形高度过渡极其平滑，无方块悬崖。

---

## 温带与热带的沙漠/草原实现差异

为了在游戏中将它们直观地表现出来，我们在代码的四个核心层面做了如下实现设计：

### 1. 地形起伏与高度系数
在 [`WorldConstants.h`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/WorldConstants.h) 中配置不同的起伏乘数，决定地表的平坦度与山脉高度：
- **热带沙漠 (`Desert`)**：只留很矮的沙丘起伏，绝对无大山（`hillAmp = 0.35, mountAmp = 0.0`）。
- **温带沙漠 (`TemperateDesert`)**：地形整体低矮，但允许生成少量硬质的风化石丘（`hillAmp = 0.40, mountAmp = 0.10`）。
- **热带草原 (`Savanna`)**：平缓的草原丘陵，偶尔出露小型平顶山（`hillAmp = 0.60, mountAmp = 0.15`）。
- **温带草原 (`Grassland`)**：极其平坦宽阔的平原与低缓草浪（`hillAmp = 0.50, mountAmp = 0.20`）。

### 2. 纵向方块分层（`getBlock` 填充）
- **热带沙漠 (`Desert`)**：表面是厚达 3~4 格的沙子（`Sand`），下方叠有约 8 格的沙砖（`Sandstone`），极深处才是基岩石头。
- **温带沙漠 (`TemperateDesert` / 戈壁)**：薄沙层，地表 60% 概率为沙子、40% 概率直接暴露石头，地下直接过渡为石头（`Stone`），无深沙砖层。
- **热带草原 (`Savanna`)**：表面覆盖偏黄色的草原草块（`SavannaGrass`），其下为泥土（`Dirt`）。
- **温带草原 (`Grassland`)**：表面覆盖标准翠绿草块（`Grass`），其下为泥土（`Dirt`）。

### 3. 树木生成判定（`shouldPlaceTree` / `placeDecorations`）
- **树木概率**：
  - `Desert` / `TemperateDesert`：不种树。
  - `Savanna`：树木较稀（`base = 1.0 / 90.0`）。
  - `Grassland`：几乎无树（`base = 1.0 / 150.0`），呈现“风吹草低见牛羊”的开阔草原。
- **树木类型**：
  - `Savanna`：生成金合欢树（使用 `SavannaBark` 橙色树干和 `SavannaLeaf` 黄绿叶）。
  - `Grassland`：生成普通橡树（`OakBark` / `OakLeaf`）。

### 4. 地表植被与花卉装饰（`placeDecorations`）
- **热带沙漠 (`Desert`)**：生成仙人掌（`Cactus`）与少量枯灌木（`DeadShrub`）。
- **温带沙漠 (`TemperateDesert`)**：仅生成大量枯灌木（`DeadShrub`），完全不生成仙人掌。
- **热带草原 (`Savanna`)**：生成偏黄的草原高草（`SavannaTallGrass`），不刷新花卉。
- **温带草原 (`Grassland`)**：生成高密度的普通长草（`TallGrass`）和大量色彩丰富的野花（`Rose` 等）。

---

## 寒带三分：雪原 / 苔原 / 针叶林

三者气候相邻，必须一眼能分开。**雪原是极地寒漠，苔原是林木线以北的低植被地带，针叶林才是有树的北方森林。** 不要把苔原做成「略带枯灌木的雪原」。

气候位置保持不变：最冷 → `SnowyPlains`；亚寒带偏干 → `Tundra`，偏湿 → `Taiga`。这和 Whittaker 图一致——苔原降水并不多，地面显得湿是因为冻土排不出去水，不是因为更潮湿。

### 1. 定位

| | 雪原 `SnowyPlains` | 苔原 `Tundra` | 针叶林 `Taiga` |
|---|---|---|---|
| 气候 | 寒带（最冷） | 亚寒带、偏干 | 亚寒带、偏湿 |
| 一眼认出来 | 全是雪、没植物 | 苔藓地面、无树、斑状残雪 | 云杉林 |
| 树 | 无 | **无** | 密（云杉） |
| 水面 | 结冰 | **不结冰**（生长季） | 不结冰 |

### 2. 地形起伏

现有幅度已经合适，苔原比针叶林平、比雪原略有起伏，避免无树雪山：

- **SnowyPlains**：`hillAmp = 0.5, mountAmp = 0.2`
- **Tundra**：`hillAmp = 0.6, mountAmp = 0.3`（可再把 `mountAmp` 压到 `0.15`）
- **Taiga**：`hillAmp = 0.9, mountAmp = 0.8`

热喀斯特浅塘（河之外再挖浅坑）作为第二阶段，第一阶段有河即可。

### 3. 纵向方块分层（`getBlock`）

- **SnowyPlains**：表面整片 `Snow`，其下泥土，再往下石头。水面最上一格 `Ice`。
- **Tundra**：表面**不要**整片雪。按列 `columnRoll` 混合（与戈壁沙/石同一写法）：
  - 约 **60%** `TundraGrass`（橄榄褐/灰绿苔藓土）
  - 约 **25%** `Snow`（洼地、阴坡残雪）
  - 约 **15%** `Stone`（冻融碎石滩 / fellfield）
  - 其下仍是三层泥土再接石头（泥炭 / 冻土）。不要沙、不要沙砖。
- **Taiga**：表面 `TaigaGrass`，其下泥土再石头。水面保持水。

### 4. 树木

- **SnowyPlains / Tundra**：`shouldPlaceTree` 返回 false，不种树。和针叶林的交界靠气候混合带自然过渡，不在苔原里种残株云杉。
- **Taiga**：云杉，`base ≈ 1/15`。

### 5. 地表植被（`placeDecorations`）

核心规则：**苔原的地面本身就是植被，不要靠种树撑场面。**

- **SnowyPlains**：无装饰。
- **Tundra**：
  - 蕨 `Fern`：`base ≈ 1/14`（莎草 / 苔丛）
  - 枯灌木 `DeadShrub`：`base ≈ 1/40`（矮柳、矮桦）
  - 不刷新花、高草、仙人掌。
- **Taiga**：蕨 `1/20`，枯灌木 `1/40`。

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

`moistureWiden`：湿度 `M` 高则降低阈值（河更密）、略加宽；`M` 很低（如沙漠和温带沙漠）则几乎没河。

雕刻（只降低、不抬高）：
```text
target = WATER_LEVEL - 2
height = lerp(height, target, mask)
```
然后该柱 `y > height && y <= WATER_LEVEL` 灌水。仅 **极地雪原 (`SnowyPlains`)** 把最上一格水改成冰（`Ice`）；苔原与针叶林保持水。河心两格深，冰下仍是水。

生成时放 **源水** 即可，不跑流体扩散。河宽大约 3～7 格。

---

## 冰和雪

不跟昼夜挂钩。结冰只属于极地雪原；苔原按生长季处理。

- **地表**：
  - `SnowyPlains` 的 `y == height` $\rightarrow$ 整片 `Snow`（整格实心，可碰撞，材质为雪顶）。
  - `Tundra` 的 `y == height` $\rightarrow$ `TundraGrass` / `Snow` / `Stone` 按列混合（见上节），不是整片雪。
  - `Taiga` 的 `y == height` $\rightarrow$ `TaigaGrass`。
- **水面结冰**：仅 `SnowyPlains` 把 **最上面那一格水** 换成 `Ice`，下面仍是水。`Tundra` 与 `Taiga` 不结冰。
- **Ice 属性**：实心、可站立、不是流体，`lightOpacity` 建议 2。不结整柱冰，避免挖不开的冰海。

---

## 群系地形与高度填充

### 高度 = 起伏 × 群系幅度
```text
h = WATER_LEVEL + 2
  + plains
  + hills     * hillAmp[biome]
  + mountains * mountAmp[biome]
```

| 群系 (`BiomeType`) | 表面方块 | 柱子分层 (上→下) | hillAmp | mountAmp |
| :--- | :--- | :--- | :--- | :--- |
| **Ocean** | Sand | 沙 → 石 | — | 浅海盆地 |
| **SnowyPlains** | Snow | 雪 → 土 → 石 | 0.5 | 0.2 |
| **Tundra** | TundraGrass / Snow / Stone | 苔藓土或雪或石 → 土 → 石 | 0.6 | 0.3 |
| **Taiga** | TaigaGrass | 针叶林草 → 土 → 石 | 0.9 | 0.8 |
| **TemperateDesert**| Sand / Stone | 砂砾/薄沙 → 石 | 0.4 | 0.1 |
| **Grassland** | Grass | 草 → 土 → 石 | 0.5 | 0.2 |
| **Forest** | Grass | 草 → 土 → 石 | 1.0 | 1.0 |
| **Desert** | Sand | 沙 (3~4格) → 沙砖 (8格) → 石 | 0.35 | 0.0 (只叠矮沙丘) |
| **Savanna** | SavannaGrass | 草草原 → 土 → 石 | 0.6 | 0.15 |
| **Jungle** | Grass | 草 → 土 → 石 | 1.2 | 0.9 |

---

## 植物与装饰密度表

装饰判定在 `World::placeDecorations`。概率公式为：`roll < base * wet`。
其中 `wet = clamp(0.35 + 0.65 * (M+1)/2, 0.2, 1.0)` (干旱稀疏，湿润浓密)。

| 群系 | 树木类型与密度系数 `base` | 地表植被与密度系数 `base` |
| :--- | :--- | :--- |
| **SnowyPlains** | 无 | 无（仅水面结冰） |
| **Tundra** | 无 | 蕨 `1/14`，枯灌木 `1/40` |
| **Taiga** | 针叶木/云杉 (Spruce)，`base ≈ 1/15` | 蕨 `1/20`，枯灌木 `1/40` |
| **TemperateDesert**| 无 | 枯灌木 `1/30` |
| **Grassland** | 孤立橡木 (极稀)，`base ≈ 1/150` | 高草 `1/15`，玫瑰/小花 `1/60` |
| **Forest** | 橡木 (Oak)，`base ≈ 1/18` | 高草 `1/28`，玫瑰 `1/120` |
| **Desert** | 无 | 边缘：仙人掌 `1/64`，枯灌 `1/32`；深处无 |
| **Savanna** | 稀树金合欢 (Savanna)，`base ≈ 1/90` | 草原高草 `1/48` |
| **Jungle** | 雨林巨木 (Jungle)，`base ≈ 1/22` (高8-15格) | 蕨类 `1/12`，藤蔓，西瓜 `1/100` |

---

## 新增方块与贴图位置规划

### 已实现
在 [`BlockId`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/Block/BlockId.h) 尾部追加：
* `SpruceBark` (针叶木干)、`SpruceLeaf` (针叶树叶)
* `JungleBark` (雨林木干)、`JungleLeaf` (雨林树叶)
* `Fern` (蕨类地表草)
* `TaigaGrass` (针叶林草块)
* `TundraGrass` (苔原苔藓土)：顶 `(2, 2)` 橄榄褐/灰绿苔藓，侧 `(3, 2)` 泥土带一圈苔，底 `(2, 0)` 与其它草块相同

可选第二阶段：棉花草贴图、矮云杉、多边形碎石环、浅塘。

---

## 柱状填充（`getBlock`）

```text
y > height:
    y <= WATER_LEVEL → 水；若 SnowyPlains 且这是最上一格水 → 冰
    否则空气
y == height:
    Ocean / Desert → 沙
    TemperateDesert → 沙（60%）或石头（40%）
    Forest / Grassland / Jungle → Grass
    Taiga → TaigaGrass
    Savanna → SavannaGrass
    SnowyPlains → Snow
    Tundra → TundraGrass（60%）或 Snow（25%）或 Stone（15%）
height-3..height-1:
    Desert / Ocean → 沙
    TemperateDesert → 石头（戈壁石床）
    其它陆地 → 土
height-11..height-4:
    Desert → 沙砖
    其它 → 石头
再往下：石头
```

厚度用常量，混界处不要突变。

---

## 存档

列格式不变（`id + meta`）。新 id 追加在后面。

**未写入磁盘的列**会按新生成器重算，和旁边旧脏列可能接不上。建议新开世界。若以后要冻地形，再升 `world.dat` Version。

---

## 代码修改分布指南

1. **[`WorldConstants.h`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/WorldConstants.h)**:
   * 追加 4 个温度带和干湿度阈值常量，并扩充 9 个群系的 relief amp 常量。
2. **[`TerrainGenerator.h`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/TerrainGenerator.h)** / **[`TerrainGenerator.cpp`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/TerrainGenerator.cpp)**:
   * 扩充 [`BiomeType`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/TerrainGenerator.h#L12-L18) 枚举，重构判定、高度混合、[`TerrainGenerator::getBlock`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/TerrainGenerator.cpp#L301) 及树木概率。
   * `isColdBiome` **只含** `SnowyPlains`（结冰）。`Tundra` / `Taiga` 水面保持水。
   * 苔原表面用 `columnRoll` 混 `TundraGrass` / `Snow` / `Stone`。
3. **[`World.cpp`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/World.cpp)**:
   * 编写 `placeSpruceTree` 和 `placeJungleTree` 特有树形生成方法。
   * 更新 [`World::placeDecorations`](file:///Users/guokezhen/Desktop/计算机/计算机图形学/LearnOpengl/游戏/Minecraft/World/World.cpp#L672) 适配 9 群系的草、花、蕨及枯灌木生成。
   * 苔原：蕨 `1/14` + 枯灌木 `1/40`；不种树。

---

## 验收

1. 湿润气候区（森林、草原、雨林、针叶林）的草地直接接水，不再一圈强制沙。
2. 热带沙漠剖面呈现 沙 → 沙砖 → 石；高度平缓无高山。
3. 温带沙漠地表多为沙石混合，地下直接为石头；无仙人掌，生长大量枯灌木。
4. 仅极地雪原水面顶层结冰、冰下仍是水；雪原覆盖纯白雪块、无植物。
5. 苔原无树；地表为苔藓土为主、夹残雪与碎石；蕨与枯灌木稀疏分布；河湖不结冰。
6. 热带草原草色偏黄偏秋意，树木极其稀少且树干偏橙色。
7. 针叶林中刷新出塔状云杉树与大片绿色蕨类；水面不结冰。
8. 热带雨林中生长有高大的丛林巨木，植被极其浓密。
9. 陆地上有自然的弯曲河流，沙漠河流极稀；只有雪原的河面结为冰河。
