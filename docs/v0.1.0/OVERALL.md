# v0.1.0 总览

第一个可玩预发布。相对更早的「进游戏就开写死种子世界」原型，这一版补上了真光照、菜单选档，并修了一批内存 / GPU 空转问题。

详细设计仍见同目录：

- [LIGHTING.md](LIGHTING.md) — 天空光、方块光、火把、昼夜
- [MENU_AND_SAVES.md](MENU_AND_SAVES.md) — 标题 / 暂停、多存档、设置

---

## 真光照

原先只有建网时烘焙的方向光 × 四角 AO，挖洞全亮、放火把也不亮。

本版按 `LIGHTING.md` 落地：

- 每个 Section 两张 16³ 光图（`skyLight` / `blockLight`，0–15），不占用 `ChunkBlock::meta`
- `.block` 增加 `LightOpacity` / `LightEmission`；水挡光、树叶滤光、火把发光
- 生成后算天空柱并传播；改块 / 流水做局部重算；存档只存 `id+meta`，读档后重算光
- 顶点带 shade + sky + block；shader 用 `max(sky × dayFactor, block)`，昼夜只改 uniform，不整图 remesh
- 火把作为可放置方块；按住 `T` 可加快昼夜

---

## 菜单与存档

原先 ESC 第二次就关窗口，种子写死 `114514`，走出视野改过的方块会丢。

本版按 `MENU_AND_SAVES.md` 落地：

- 启动进标题，不自动建世界
- ESC 暂停（松鼠标、停模拟）；退出只走菜单
- 多存档：列表加载 / 删除，随机或手输种子新建
- 脏列写入 `saves/worlds/<folder>/c.<cx>.<cz>`，头文件 `world.dat` 存名字、种子、玩家、快捷栏、游戏时间
- 全局 `saves/settings.cfg`：FOV、视距、灵敏度、Vsync、全屏
- ASCII 点阵字 + `HudRenderer`，快捷栏和菜单共用

---

## 内存与 GPU

发版前修的运行时问题：

- `Model` 移动赋值补上 `DeleteData()` 和 `return *this`；EBO 记入 `m_buffers`，重建网格不再漏显存
- 去掉未使用的 `CubeRenderer` 堆数组；退出时释放 Shader / 纹理 / ChunkRenderer
- `ChunkSection` 方块与光照从 `vector` 改成固定 `array`，少掉每列 24 次小分配
- 菜单空闲时 `glfwWaitEvents` 休眠，有输入最多 30fps；避免标题界面把 M 系列 GPU 挂在高占用上

---

## 其它

- 水下挖掘 / 放置优先命中固体，不再隔着水打不中
- 昼夜曲线加长黄昏、压暗夜晚
- GitHub Release 提供 macOS arm64 包；源码编译见仓库根目录 [README.md](../../README.md)

---

## 本版不做

洞穴 / 矿石、掉落、生命值、实体、音效。创造背包在 v0.3.0 补上。存档格式已按「只存方块、不存光图」冻住，后面改地形生成时要升 `world.dat` Version 或丢弃旧列文件。
