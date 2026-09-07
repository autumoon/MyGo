# 围棋（吃5子获胜）对弈程序

---

## 项目概述

基于 **C++**（逻辑）+ **Nana GUI**（界面）的**人机对弈围棋**程序，使用 **9 路小棋盘**（可扩展 19 路），胜负规则为**提子满 5 枚**（简化规则）。提供 3 档 AI 难度（简单/中等/困难）+ 1 档双人模式，**界面已全面中文化**。

---

## 当前已完善功能

- **人机对弈**：玩家执黑，AI 执白。玩家落子后 AI 自动应手。
- **三档 AI 难度**：
  - **简单（稳健）**：`assessMoveSimple` 净发展评估（提子/己方气/连接/中心），取最高分。有棋感但**不追击、不截断、不知逃跑**——行为接近早期未优化的中等算法（替代原无意义的随机）。
  - **中等（蒙特卡洛）**：模拟落子打分（攻守平衡评估，见 AI 算法表），取最高分。
  - **困难（混合）**：启发式筛出前 5 候选点 + 蒙特卡洛模拟（100 次/候选点，胜率仅 1/4 权重微调）。
- **双人模式**：难度下拉最后一项「双人模式」，黑白双方均由玩家轮流落子，AI 不应手（既是双人对弈，也可用来调试棋盘规则）。
- **棋盘操作**：落子、悔棋（Undo）、停一手（Pass）、认输（Resign）、重新开始（New Game）。
- **快捷键**：Ctrl+Z 悔棋、Ctrl+N 新游戏、空格停一手（对窗体与全部子控件统一绑定，任意焦点下生效）。
- **悬停落子预览**：鼠标悬停空点时显示当前走棋方的半透明棋子（黑=深棕灰、白=浅米灰），仅在格子变化时重绘。
- **提子规则**：落子后提走无气对方棋块，并处理打劫（ko）禁着点。
- **平局规则**：双方连续两次 Pass（`passCount>=2`）且无人吃满 5 子 → 平局，播放 `平局.wav`。`hasLegalMove()` 检测是否存在合法落子（含劫/自杀排除），AI 无合法落子时主动 Pass 触发终局，避免"空点全为自杀/劫点"时 AI 乱落子或无限对停。
- **最后落子红点高亮**：最近一步落子位置绘制红色圆点（半径 8），悔棋后会随历史状态一并恢复。
- **音效**：落子/提子/胜负播放 wav（集中在与 exe 同目录的 `res` 子目录：`place/capture/黑中盘胜/白中盘胜/平局.wav`），使用 `PlaySoundW(SND_ASYNC|SND_FILENAME|SND_NODEFAULT)`，播放前先 `GetFileAttributesW` 判断文件存在，缺失则静默跳过（无音频也能正常运行）。玩家与 AI 落子均发声；玩家（黑）赢 → `黑中盘胜.wav`，电脑（白）赢 → `白中盘胜.wav`，平局 → `平局.wav`。
- **AI 思考提示**：AI 计算期间状态栏显示 `AI 思考中...`，且棋盘上有红色文字提示。
- **配置记忆**：读写与 exe 同名 ini（`MyGo.ini`），保存并恢复难度、窗口尺寸。
- **窗口图标**：exe 图标 + 标题栏 + 任务栏均使用 `app.ico`。
- **棋谱导入/导出（SGF 子集）**：底部「新游戏→导出棋谱→导入棋谱→悔棋→停一手→认输→难度」单行布局（各按钮 66px），Win32 文件对话框（`GetSaveFileNameW`/`GetOpenFileNameW`）。格式：`(;GM[1]FF[4]CA[UTF-8]SZ[9]PB[玩家]PW[AI]RE[?|B+|W+|Draw] ;B[坐标];W[...])`，坐标列+行各一小写字母 `a-i`，Pass 为空节点 `;W[]`。导出记录全程落子、`RE` 按结果填；导入逐步重放并校验（非法手报错），导入后若轮到 AI 且非双人模式自动应手。

---

## 模块架构

| 模块 | 文件 | 职责 |
|------|------|------|
| 棋盘逻辑 | `Core\GoCore.h` / `Core\GoCore.cpp` | 棋盘状态、落子/提子/打劫、胜负判断、悔棋历史、`assessMove` 打分接口 |
| 用户界面 | `UI\GoUI.h` / `UI\GoUI.cpp` | 表单与棋盘绘制、鼠标/键盘/按钮事件、AI 计算入口、状态栏、音效、配置读写 |
| 主程序 | `MyGo\main.cpp` | 创建 `GoUI ui(9)` 并 `run()` |

设计原则：
- **逻辑与界面分离**：`GoCore` 不含任何 GUI 依赖，可独立测试/复用。
- **AI 算法按难度模块化**：`doSimpleMove()` / `doHeuristicMove()` / `doHybridMove()`（`doRandomMove` 已弃用未删，无调用方）。
- **悔棋历史**：`std::stack<HistoryState>` 保存每步前状态，含 `lastMoveRow/lastMoveCol`（红点随历史恢复），支持 Undo。
- **棋谱记录**：另维护 `std::vector<MoveRecord> moveLog`（黑白交替的坐标或 Pass），`placeStone/pass` 追加、`undo` 弹出、`reset` 清空；`undo` 连撤为对象内多次调用，每条对应一手。

---

## 目录结构

```
MyGo\
├── Core\
│   ├── GoCore.h          # 棋盘逻辑头（UTF-8 with BOM）
│   └── GoCore.cpp        # 棋盘逻辑实现（UTF-8 with BOM）
├── UI\
│   ├── GoUI.h            # UI 头（UTF-8 with BOM）
│   └── GoUI.cpp          # UI 实现：绘制、事件、AI 调用（UTF-8 with BOM）
├── MyGo\
│   ├── MyGo.vcxproj      # VS 工程（含 /utf-8、wav 复制、.rc 资源编译）
│   ├── MyGo.rc           # 资源脚本：IDI_APP ICON "..\app.ico"
│   ├── Resource.h        # #define IDI_APP 101
│   ├── main.cpp          # 入口（UTF-8 with BOM）
│   ├── app.ico           # 图标源文件
│   ├── res\              # 音效源文件：place/capture/黑中盘胜/白中盘胜/平局.wav（<None> 复制到输出目录 res\）
│   └── x64\Release\MyGo.exe   # 构建产物
├── app.ico               # vcxproj 中 <None Include> 复制到输出目录
├── lib\
│   └── nana_v140_Release_x64.lib
├── nana\                 # Nana 源码/头文件
└── AGENTS.md             # 本文件（UTF-8 无 BOM）
```

---

## 关键实现说明

### GoCore（棋盘逻辑）
- 成员：`board(boardSize*boardSize)`、`blackTurn`、`capturesBlack/White`、`passCount`、`koPoint`、`history`、`lastMoveRow/lastMoveCol`。
- 核心方法：`placeStone()`（合法检查→提子→劫留→记历史→换手）、`pass()`、`undo()`、`isGameOver()`（吃5子 或 `passCount>=2` 平局）、`getWinner()`（黑胜/白胜/平局）、`hasLegalMove()`（检测是否有任一合法落子点）、`assessMove()`（**攻守平衡评分**，见 AI 算法表）。
- 高亮支持：`getLastMoveRow()/getLastMoveCol()` 返回最近落子坐标（pass/undo/reset 时重置为 -1）。
- `HistoryState` 结构包含 `lastMoveRow/lastMoveCol`，`placeStone` 手动构造历史时必须一并保存，否则悔棋后红点丢失。

### GoUI（界面）
- 窗口尺寸：构造函数初始化列表 `nana::form(nana::API::make_center(580, 700))`。
  - **调整初始窗口大小只改这一行**：`make_center(宽度, 高度)`。
  - `loadConfig()` 会从 ini 读取窗口尺寸恢复（仅当 `width>=600 && height>=700` 才应用）。
- 棋盘居中：`onDraw()` 内用 `padX`/`padY` **分别**按窗口宽、高度居中：
  ```cpp
  cellSize = maxSize / boardSize;
  int totalGrid = (boardSize - 1) * cellSize;
  padX = (width - totalGrid) / 2;            // 水平居中
  padY = (effectiveHeight - totalGrid) / 2;  // 垂直居中（避开底部按钮区）
  ```
  `effectiveHeight = 窗口高 - bottomReserved(90)`；`onDraw` 每次按当前窗口尺寸重算，窗口缩放棋盘始终居中。点击换算也用 `padX/padY`。
- 最后落子高亮：`GoUI::onDraw` 末尾读取 `getLastMoveRow/Col()`，在棋子中心绘制红色圆点（`round_rectangle` 半径 8）。
- 悬停预览：`onMouseMove` 换算行列，仅当「预览格子变化」才 `refresh_window()`；白棋回合/双人模式下预览变浅色（按当前走棋方取半透明黑/白）。
- 窗口标题：`updateStatus()` 中 `this->caption("围棋吃5子  " + 走棋方 + "走")`（不带棋盘路数）。
- 难度下拉文案：`简单 (稳健)` / `中等 (蒙特卡洛)` / `困难 (混合算法)` / `双人模式`；combox 宽度 150（位于底部右侧）。
- `assessMoveSimple`（净发展评估）：只算提子 ×100、落子后己方块气 ×5、相邻己子 +12、中心偏好 ≤10——与 `assessMove` 完全独立，不含打吃/截断/紧气/全局逃命。**简单档用它，因此不追击黑棋、不会救自己濒死块**，与中等/困难形成梯度。
- AI 思考提示：`GoUI::doAIMove()` 开头：
  ```cpp
  aiThinking = true;
  lblStatus.caption("AI 思考中...");
  nana::API::refresh_window(*this);
  ::RedrawWindow(reinterpret_cast<HWND>(this->native_handle()), NULL, NULL,
      RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
  ```
  `onDraw` 检测 `aiThinking` 时在状态栏区域绘制红色 "AI 思考中..."；AI 计算完后置回 false 并 `updateStatus()`。
  **注意**：必须用 `RedrawWindow(RDW_UPDATENOW)` 同步强制重绘——Nana 的 `refresh_window` 是 lazy 的，AI 计算期间消息循环被占用时不会真正刷新界面。
- 快捷键（关键！）：Nana 的 `events().key_press` **只派发给当前有焦点的控件**。点过按钮后焦点随按钮走，窗体上的 key_press 就不再触发。处理函数提取为 `onKeyPress()`，用 lambda `bindKeys(w)` 统一绑定到**窗体 + 全部子控件**（label/button/combox）。**不要用** `nana::key::space`（`nana::key` 是模板类，会 C2955）或 `btn.emit_click()`（非 button 类成员，C2039）。
- 配置读写：`configPath()` 用 `GetModuleFileNameW` 取得 exe 全路径并把 `.exe` 替换为 `.ini`。`loadConfig()` 在构造函数中 `updateStatus()` **之前**调用（保证难度选项先就位）；`saveConfig()` 挂 `events().unload`。键：`Difficulty`、`Width`、`Height`。
- 双人模式：凡涉及「是否 AI 应手 / 允许谁落子 / 悔棋步数」的判断处都要检查 `cmbDifficulty.option() == 3`（详见下）。
- 音效：GoUI.cpp 顶部 `#include <mmsystem.h>` + `#pragma comment(lib, "winmm.lib")`；`playSound()` 缓存 exe 目录后拼 `res\文件名` 播放，播放前 `GetFileAttributesW` 判存在、缺失静默跳过。vcxproj 中用 `<None Include="res\xx.wav"><Link>res\xx.wav</Link></None>` 把音频复制到输出目录 `res\` 子目录。
- 图标：`GoUI::run()` 中 `LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP))` + `SendMessage(hwnd, WM_SETICON, ICON_SMALL/ICON_BIG)`。

### 双人模式（index 3）相关判断点
- `onMouseClick`：`bool two = (cmbDifficulty.option() == 3); if (!two && !core.isBlackTurn()) return;`（双人可下黑白）；落子后 `if (!core.isBlackTurn() && !two) doAIMove();`。
- `onPass`：`else if (!core.isBlackTurn() && cmbDifficulty.option() != 3) doAIMove();`。
- `onUndo`：双人模式每次只撤一手；AI 模式连撤两步回到玩家（黑）回合。
- `onMouseMove`：双人模式下 `canPreview = true`（任意方可预览）。
- `onDraw`：预览色按 `core.isBlackTurn()` 取黑/白两种半透明色。
- `updateStatus`：case 3 显示「双人」。
- `loadConfig`：难度 `diff` 允许 0..3。

---

## AI 算法表

| 难度 | 下拉文案 | 实现 | 复杂度 | 特点 |
|------|------|------|--------|------|
| 简单 | 简单 (稳健) | 每个空点用 `assessMoveSimple` 净发展打分，取最高 | O(n²) | 有棋感、不追击不逃命（早期未优化中等算法） |
| 中等 | 中等 (蒙特卡洛) | 每个空点用 `assessMove` 攻守平衡打分，取最高 | O(n²) | 攻守兼备 |
| 困难 | 困难 (混合算法) | `assessMove` 筛前 5 候选，蒙特卡洛 100 次/点，最终 `评估分 + 胜率/4` 综合选点 | O(n²×模拟) | 评估分主导、胜率细调，实测略优于中等 |
| 双人 | 双人模式 | ——（AI 不应手，玩家轮流下黑白） | —— | 双人对弈 / 调试棋盘逻辑 |

### assessMove 攻守平衡评分（白棋视角，核心智力）
改版后不再只"自取发展"，而是同时考虑**攻击敌方**与**发展自身**：

| 项 | 分值 | 说明 |
|---|---|---|
| 提子 | 吃子数 ×100 | 立即得利 |
| 打吃 | 每邻近黑块气变为1 +30 | 叫吃 |
| **防守-紧气** | 气挤压量 ×25 ×威胁权重 | 威胁权重：黑块原≤2气（濒死大龙）时 ×2；黑块原1气被贴而不减气时 +15 牵制 |
| **防守-截断** | 每相邻黑子 +10 | 深入敌方腹地，阻碍其成大龙 |
| 进攻-己方气 | 落子后己块气 ×5 | 保持自身活力 |
| **进攻-连接** | 每相邻白子 +12 | 报团成势 |
| **己方濒死块营救** | 盘上最弱白块≤2气：救活 +块大小×80 / 缓解 +×40；最弱白块1气而本手不救 −块大小×80 | 逃跑**优先于**截断/连接（权重 80 可压过 3/4 号项的攻击分，且防止困难蒙特卡洛胜率随机把救援点挤出 top1） |
| **送死角惩罚** | 落子后己方块仅 1 气、**无提子收益**、且邻黑 −150 | 评估只看落子瞬时，见不到对方反提——打吃+濒死紧气可瞬时堆到 200+ 分，罚 80 曾压不住（实测 217−80=137 仍排第一）。改为 150 后彻底出局（137 对应点降至 top8 外）。提了子或空旷孤子（无邻黑）豁免，不误伤提子妙手 |
| 中心偏好 | ≤10 | 靠近中心加分 |

- 实现位于 `GoCore::assessMove()`：先对落点四邻收集**敌方黑块落子前的气**（`enemyBlocks`），在临时副本 `sim` 落子后对比气差求挤压量，再统计相邻黑/白子数。**己方逃命是全局的**：落子前全盘扫出最弱白块（气最小），落子后看它所在块的气变化——救活/缓解加分，放任 1 气块被杀则重罚（只有真能救它的点因此得高分，避开"远离濒死块的攻击点"）。
- **中等与困难共用该评分**（困难的蒙特卡洛候选筛选第一步也用它），因此两档棋力同时增强。
- **困难选点 = 评估分主导 + 模拟胜率/4 细调**（`doHybridMove` 第 3 步）：蒙特卡洛用**双方纯随机落子** playout，噪声很大——权重过高会被"远离战场、随机对局不易被抓的虚高胜率点"带偏，导致困难反而弱于纯评估的中等（实测：wins 权重 1:1 时困难 5:7、中等 8:4）。故取 `评估分 + wins(0~100)/4` 综合最大者：评估分差 1 点胜过 4 点胜率，只影响分数接近的点。实测该权重下困难 7:5、中等 6:6（vs 固定入门黑策略，12 局抽样）。**新增送死角惩罚后棋力小幅再升**：中等 7:5、困难 9:3；罚分提至 150 后抽样：中等 11:1、困难 8:4。

---

## 配置（MyGo.ini，与 exe 同目录）

```ini
[Settings]
Difficulty=0        ; 0简单 1中等 2困难 3双人
Width=580
Height=700
```

---

## 构建与运行

### 环境
- **编译器**：Visual Studio 2015（64 位工具链）。VS2015 位于 `D:\Tools\VS2015`（含 `D:\Tools\VS2015\VC\bin\cl.exe`）。
- **MSBuild**：`C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe`。
- 依赖 Nana GUI（头文件在仓库 `nana\`，静态库 `lib\nana_v140_Release_x64.lib`）。

### 命令行构建
```powershell
& "C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" "E:\Documents\Default Project\MyGo\MyGo\MyGo.vcxproj" /p:Configuration=Release /p:Platform=x64 /t:Build /v:minimal 2>&1 | findstr /V "LNK4099"
```
也可直接用 Visual Studio 打开 `MyGo.sln`，选 `Release x64` 编译。
- 偶发 `FTK1011`（FileTracker tlog 错误）属临时抖动，直接重新编译即可。
- 构建成功后若有多个 exe 副本（根 `x64\Release` 与 `MyGo\x64\Release`），以 vcxproj 输出路径为准。

### 运行
执行 `MyGo\MyGo\x64\Release\MyGo.exe`。

---

## 编码约定（重要！修改文件前必读）

| 文件 | 编码 |
|------|------|
| `Core\GoCore.h`, `Core\GoCore.cpp`, `UI\GoUI.h`, `UI\GoUI.cpp`, `MyGo\main.cpp` | **UTF-8 with BOM** |
| 根 `main.cpp`（旧/备用）, `AGENTS.md` | UTF-8（无 BOM） |

- **已统一为 UTF-8 with BOM**，且 vcxproj Release|x64 已加 `/utf-8` 编译选项，源文件里的中文可正常编译，不再有 C4819 警告。
- 编辑这些源文件时保持 BOM 不变（用 PowerShell `[System.IO.File]::WriteAllBytes` 前 3 字节为 `EF BB BF`）。
- C4267（int/size_t）为既有警告，可忽略。

---

## 历史教训（避免重蹈覆辙）

- **不要为显示提示把 AI 改成后台线程**：曾把 `doAIMove` 改为 `std::thread` + `nana::timer` 回调落子，导致第二次 AI 落子失败时回合停在白棋、玩家无法继续下棋（卡死）。**下棋流程必须保持同步**：AI 直接在 `doAIMove()` 内计算并落子。
- **提示刷新手段**：`update_window`/`refresh_window` 对 lazy 控件不生效；可靠方式是绘制回调（onDraw）+ `RedrawWindow(RDW_UPDATENOW)`。
- **快捷键必须绑定到窗体和全部子控件**：Nana 的 `key_press` 只派发给焦点控件，点过按钮后若只在窗体绑定，快捷键就失效。
- **悔棋红点**：历史状态里必须保存 lastMove，否则悔棋后红点标记丢失。
- **验证 AI 智力可脱离 GUI 直接编译测试**：`assessMove` 纯逻辑无 GUI 依赖，可用临时 main（如 `E:\Temp\opencode\test_assess.cpp`）构造局面打印各点得分，直观对比攻守倾向。编译命令参考 `E:\Temp\opencode\run.bat`（先 `vcvarsall.bat x86_amd64`）。
- **AI 无合法落子时必须 Pass，不能往自杀/劫点试错**：曾出现 AI 选到非法点 `placeStone` 失败但回合不换手 → 停在白棋、玩家无法继续。所有 AI 难度开头统一 `if (!core.hasLegalMove()) core.pass()` 守卫；`doRandomMove` 用 `std::shuffle` 逐个重试，其余难度 `placeStone` 失败也必加 `pass` 兜底。

---

## 未来扩展方向

- 支持 19 路：`MyGo\main.cpp` 改 `GoUI ui(19)`（需同步调窗口尺寸与 AI 模拟开销）。
- 实现更完整围棋规则（点目/贴目胜负）。
- 动态切换 9/13/19 路棋盘。

---

## 许可与支持

本项目用于学习和个人使用，无特殊授权限制。

开发不易，如果你愿意，希望支持一下开发者![](./开发不易，感谢支持wx.jpg)

---

![](./开发不易，感谢支持zfb.jpg)**更新日期**：2026-09-07  
**版本**：1.0（第一版定稿)