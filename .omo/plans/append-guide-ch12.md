# 把多工程工作区配置写进新手配置指南

## TL;DR (For humans)

在 `D:\Newcode\Gimbal_24hero_CH010_20260111\新手配置指南.md` **末尾追加第12章**，介绍"多工程共用一套快捷键"的配置方法与优势。文档现为 655 行，第11章结尾在第655行。追加内容与全文风格一致（速查风格：做什么+命令+一句话结果+本章小结）。

**唯一改动**：向 `新手配置指南.md` 追加第12章（不改动前655行任何内容）。

## Todos

### 1. [x] 向 `新手配置指南.md` 末尾追加第12章

**WHERE**: `D:\Newcode\Gimbal_24hero_CH010_20260111\新手配置指南.md`，在第655行（当前文件最后一行）之后追加。
**HOW**: 用 edit 工具，oldString 定位到当前最后一行：
```
- 说明：调试（`launch.json` 的 cortex-debug）仍固定走 DAP，本方案只做**烧录**的自动切换；想让调试也自动切换要另配，暂未做。
```
newString = 上面这行原文 + 下面的完整第12章内容（在原行后追加，中间空一行加 `---` 分隔）。

**追加的第12章完整内容**（Markdown）：

````markdown

---

## 第12章 多工程共用一套快捷键（同一个键，按当前文件编烧对应工程）

把云台、底盘等多个独立工程放进同一套流程：**同一个 `Alt+, / Alt+N / Alt+M`，你在编辑哪个工程的文件，就编译烧录哪个工程**。无论"单独打开某个工程文件夹"还是"打开多根工作区"，都能用。

### 12.1 目录怎么放

多个工程**平级**放在上层目录，例如：

```
D:\Newcode\
├─ Gimbal_24hero_CH010_20260111\          ← 云台工程（自带 CMakeLists、Core、scripts 等）
├─ Chassisl_26_SHANGTAIJIE_SEND_DM\
│  └─ Chassisl_26_SHANGTAIJIE\            ← 底盘工程（结构同上）
└─ gimbal-chassis.code-workspace          ← 多根工作区文件（见 12.3）
```

每个工程都是**完整独立**的：各有自己的 `CMakeLists.txt`、`CMakePresets.json`、`cmake/`、`openocd.cfg`、`scripts/flash.ps1`、`.vscode/`。互不干扰，各自的 `build/` 也天然隔离（在各自目录下）。

### 12.2 核心原理：`${fileWorkspaceFolder}` 一招通吃

关键是把任务里的 `${workspaceFolder}` 换成 **`${fileWorkspaceFolder}`**（当前打开文件所属的工程根）。这个变量在两种打开方式下都正确：

| 打开方式 | `${fileWorkspaceFolder}` 解析为 | 快捷键效果 |
| --- | --- | --- |
| 单独打开云台文件夹 | 云台工程根 | 编烧云台 |
| 单独打开底盘文件夹 | 底盘工程根 | 编烧底盘 |
| 打开多根工作区 | 当前编辑文件所属的工程根 | 按当前文件分流 |

> **为什么不能用 `${workspaceFolder}`**：多根工作区里，VSCode 有个已知限制——用快捷键触发**同名** task（build/flash）时，它永远执行"第一个根"里那份，不按当前文件分流（microsoft/vscode #227350）。改用 `${fileWorkspaceFolder}` 后，即便 VSCode 选了第一个根的任务定义，命令实际仍指向当前文件的工程，从而正确分流。一份配置，单根/多根两种模式通吃。

### 12.3 新建多根工作区文件（逐字照抄）

在 `D:\Newcode\` 下新建 `gimbal-chassis.code-workspace`：

```json
{
    "folders": [
        {
            "name": "Gimbal (云台)",
            "path": "Gimbal_24hero_CH010_20260111"
        },
        {
            "name": "Chassis (底盘)",
            "path": "Chassisl_26_SHANGTAIJIE_SEND_DM/Chassisl_26_SHANGTAIJIE"
        }
    ],
    "settings": {
        "C_Cpp.errorSquiggles": "disabled"
    }
}
```

- `path` 相对于 `.code-workspace` 所在目录（这里是 `D:\Newcode`）。底盘路径含两层嵌套，逐字保留。
- **注意**：任务不放在这里（见 12.2 原因），只放 folders + settings。
- 用法：**双击这个文件用 VSCode 打开**（不再是"打开文件夹"），侧栏出现"Gimbal (云台)"和"Chassis (底盘)"两个根。

### 12.4 每个工程的 tasks.json（两个工程内容完全一致，逐字照抄）

每个工程的 `.vscode/tasks.json` 都写成下面这样——三任务全部用 `${fileWorkspaceFolder}`：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "cmake --build --preset Debug",
            "options": {
                "cwd": "${fileWorkspaceFolder}"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "flash",
            "type": "shell",
            "command": "powershell -ExecutionPolicy Bypass -File \"${fileWorkspaceFolder}\\scripts\\flash.ps1\"",
            "problemMatcher": []
        },
        {
            "label": "build and flash",
            "dependsOrder": "sequence",
            "dependsOn": [
                "build",
                "flash"
            ],
            "problemMatcher": []
        }
    ]
}
```

- **build**：`options.cwd` 设为 `${fileWorkspaceFolder}`，这样 `cmake --preset Debug` 在当前工程目录里构建。
- **flash**：调各工程自己的 `scripts/flash.ps1`（第11章的自动切换脚本），路径用 `${fileWorkspaceFolder}`。反斜杠必须 `\\` 转义、内层引号 `\"` 转义（见 11.4 的坑）。
- 两个工程这份文件**内容一模一样**，直接复制即可。
- 前提：每个工程都要有自己的 `scripts/flash.ps1`（把第11章云台那份原样复制过去，因两工程产物名都是 `Gimbal_Demo`、都是 STM32F405RG，脚本无需改动即可通用）。

### 12.5 代码同步问题（重要澄清）

**单根编辑和多根编辑完全同步、双向同步。** 因为 `.code-workspace` 不复制任何代码，它只是"同时显示这几个文件夹"的视图配置。代码永远只有磁盘上那一份：

- 单独打开云台文件夹改了文件 → 多根工作区里看到的就是改后的。
- 多根工作区里改了文件 → 单独打开也是改后的。

没有"两份副本"，不会分叉、不会丢。就像用两个资源管理器窗口看同一个文件夹，改哪个都是同一个文件。

### 12.6 用法和验证

先**完全关闭 VSCode 再重开**（配置改动只对新窗口生效），然后三种模式都可用：

1. **单独打开云台文件夹** → `Alt+,` 编译、`Alt+N` 烧录、`Alt+M` 编译加烧录，作用于云台。
2. **单独打开底盘文件夹** → 同样的键，作用于底盘。
3. **双击打开 `gimbal-chassis.code-workspace`** → 在云台文件里按键编烧云台，切到底盘文件按键编烧底盘。

验证分流对不对：按 `Alt+,` 后看终端打印的构建路径，或 `[flash]` 那行的 ELF 路径，是云台目录还是底盘目录。

> 首次编译某工程若报 "ELF/HEX not found"，先按 `Alt+,` 编译一次生成产物再烧。

### 12.7 优势小结

- **一套快捷键，多个工程**：不用记"现在要编哪个"、不用切配置，按当前文件自动对号入座。
- **两种打开方式通吃**：单独开工程、开多根工作区都能用，靠的是 `${fileWorkspaceFolder}` 一个变量。
- **零副本、双向同步**：代码只有一份，怎么切都不会分叉或丢失。
- **各工程完全独立**：build 目录、配置、烧录脚本互不干扰；新工程按同样结构丢进 `D:\Newcode\` 并加进 `.code-workspace` 即可纳入同一套流程。
- **复用第11章成果**：烧录仍是 DAP/J-Link 自动切换，多工程照样享受。

### 12.8 本章新增 / 改动的文件小结

- **新增** `D:\Newcode\gimbal-chassis.code-workspace`：多根工作区文件（folders + settings，不含 tasks）。
- **改** 每个工程的 `.vscode/tasks.json`：三任务统一用 `${fileWorkspaceFolder}`（把原来的 `${workspaceFolder}` 换掉）。
- **新增** 底盘 `scripts/flash.ps1`：从云台原样复制（DAP/J-Link 自动切换，无需修改）。
- **未动** 两工程的 `CMakeLists.txt`、`CMakePresets.json`、`openocd.cfg`、`launch.json`、快捷键 keybindings、固件代码。
````

**EXPECT**: `新手配置指南.md` 从655行增加到约 800 行，第12章完整追加，前655行一字未改。
**QA**:
- happy：读取文件确认第12章存在、第655行原有内容保留、Markdown 代码块闭合正确（追加内容里 json 代码块的三反引号成对）。
- failure：若 oldString 匹配失败 → 重新读文件末尾核对最后一行原文再匹配。

## Must-NOT-Have
- 不改动文档前655行任何内容。
- 不改动任何工程配置文件（本任务纯文档）。
- 追加内容里 markdown 代码围栏用三反引号，不要破坏文档其他围栏。
