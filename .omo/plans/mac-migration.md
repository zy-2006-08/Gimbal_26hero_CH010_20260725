# TL;DR (For humans)

**目标**：在一台全新的 Apple Silicon（M5）MacBook Pro 上，复刻 Windows 那套 STM32 开发环境（`新手配置指南.md` 描述的 14 章内容），并迁移 VSCode 配置。产出是**一份中文 md 文件 `Mac迁移配置指南.md`**，放在云台工程根，供 Mac 上的 agent 照着执行。

**已和你敲定的决策**：
1. Mac = Apple Silicon M5 → Homebrew 前缀 `/opt/homebrew`，原生 arm64。
2. 代码迁移 = **GitHub**（Mac clone）。仓库：云台 `github.com/zy-2006-08/Gimbal_26hero_CH010_20260725`、底盘 `github.com/zy-2006-08/Chassisl_26_SHANGTAIJIE`。指南处理"launch.json 被 git 忽略、clone 不带来"的问题。
3. 范围 = **两个工程**（Gimbal + Chassis）+ 多根工作区 `gimbal-chassis.code-workspace`。
4. 烧录 = **自动切换** J-Link ↔ CMSIS-DAP，重写成 macOS 版 `flash.sh`。
5. 快捷键 = **Cmd+, 编译 / Cmd+M 编译加烧录 / Cmd+N 烧录**。
6. Ozone = **要**（macOS Apple Silicon 版），修掉 `ozone/yuntai.jdebug` 里写死的 Windows 老路径。
7. VSCode 迁移 = 仓库配置自动跟代码走 + 装 clangd/cortex-debug 扩展 + 写 Cmd 快捷键 + 重建 launch.json。

**为什么这么做（关键技术依据，已核实）**：
- 工具链：`brew install --cask gcc-arm-embedded` + `brew install cmake ninja`；OpenOCD 用 **xPack 版**（因为 cortex-debug 官方警告 Homebrew 默认 openocd 与它不兼容）。
- J-Link 的 macOS CLI 是 `JLinkExe`（不是 `JLink.exe`），在 `/Applications/SEGGER/JLink_Vxxx/`；检测用 `system_profiler SPUSBDataType | grep 0x1366`。
- CMSIS-DAP 在 macOS 免驱（走 HID）。
- keybindings.json 在 `~/Library/Application Support/Code/User/`。

**下一步**：本文件是计划。真正的 `Mac迁移配置指南.md` 会在你启动执行（worker）后按下方 Todos 逐字写出。**我不会自己动手写产品文件。**

---

# Plan: mac-migration

## Context / Findings

Windows 侧现状（已核对真实文件）：
- `D:\Newcode\` 下有两个独立 git 工程：`Gimbal_26hero_CH010_20260725`、`Chassisl_26_SHANGTAIJIE`，外加 `gimbal-chassis.code-workspace`（多根工作区，不属于任一仓库）。
- 当前仓库**未配 GitHub 远程**（用户将自行 push）。
- `.gitignore` 忽略 `/.vscode/launch.json` 和 `/build` → clone 不会带来这两者（属正常，需在 Mac 重建 launch.json、重新编译）。
- `scripts/flash.ps1` 是纯 Windows PowerShell（`Get-PnpDevice` VID_1366 → `JLink.exe` 烧 hex，否则 OpenOCD 烧 elf）。
- `.vscode/tasks.json` 三任务用 `${fileWorkspaceFolder}`，flash 调 `powershell ... flash.ps1`。
- `ozone/yuntai.jdebug` 写死老 Windows 路径 `D:/Newcode/Gimbal_24hero_CH010_20260111/...`（两处 AddPathSubstitute + 一处 File.Open）。
- 仓库内跨平台可移植文件：`CMakeLists.txt`、`cmake/gcc-arm-none-eabi.cmake`、`CMakePresets.json`、`openocd.cfg`、`.clangd`、`.vscode/settings.json`。

macOS 事实（librarian 核实，附引用）：
- `brew install --cask gcc-arm-embedded`（Arm GNU toolchain，原生 arm64，含 gcc/g++/gdb/objcopy/size）。`brew install cmake ninja`。
- OpenOCD：Homebrew 公式名 `open-ocd`，但 **cortex-debug 明确警告 macOS brew 默认版不兼容** → 用 xPack OpenOCD（github.com/xpack-dev-tools/openocd-xpack/releases，darwin-arm64）。
- J-Link Software Pack 有 macOS Apple Silicon 安装器；CLI 是 `JLinkExe`（在 `/Applications/SEGGER/JLink_Vxxx/`），并含 `JLinkGDBServer`。
- 检测 J-Link：`system_profiler SPUSBDataType | grep -qi 0x1366`（VID 0x1366 = SEGGER）。
- CMSIS-DAP 在 macOS 免驱（HID，open-ocd 依赖 hidapi+libusb）。
- Ozone 有 macOS Apple Silicon 版（segger.com/downloads/jlink/#Ozone）。
- keybindings.json：`~/Library/Application Support/Code/User/keybindings.json`；`code --install-extension` 可用；扩展 ID `llvm-vs-code-extensions.vscode-clangd`、`marus25.cortex-debug`。

## Approach

产出 ONE 份自包含中文 md（`Mac迁移配置指南.md`，放云台工程根），共 10 章 + 3 附录，供 Mac agent 端到端执行。内容已完整定稿（见下方"完整文稿"区）——worker 只需把该文稿**逐字写入**目标文件，无需再做任何判断。

## Scope OUT / Must-NOT-Have
- 不迁移 VSCode 主题、与本项目无关的扩展、Settings Sync（用户明确只要仓库配置 + clangd/cortex-debug + Cmd 快捷键 + launch.json）。
- 不改动固件代码、`CMakeLists.txt`、`cmake/*.cmake`、`CMakePresets.json`、`openocd.cfg`（跨平台通用）。
- 不实现 Windows 端的 git push（用户自行完成）。
- 不做"整盘拷贝"迁移路线（用户选 GitHub 路线）。
- worker 写文稿时**一字不改**，不得自行"优化"或增删章节。

## Todos

### 1. [x] `Mac迁移配置指南.md`：在云台工程根逐字写入下方完整文稿 - expect 文件存在且内容与文稿一致
- WHERE: `D:\Newcode\Gimbal_26hero_CH010_20260725\Mac迁移配置指南.md`（Mac 端为 `~/Newcode/Gimbal_26hero_CH010_20260725/Mac迁移配置指南.md`）
- HOW: 用 write 工具把"## 完整文稿"分隔线之后的全部 Markdown 逐字写入，不增删不改写。
- WHY: 这是交付物本体——供 Mac agent 照做。
- 验收：文件存在；含"第0章"到"第10章"及附录 A/B/C；`flash.sh` 代码块、`tasks.json`、`keybindings.json`、`launch.json` 两方案、Ozone 改法均在。
- QA（agent 执行，happy + failure）：
  - happy：`grep -c "^## 第" Mac迁移配置指南.md` 返回 ≥ 10；`grep "system_profiler SPUSBDataType" Mac迁移配置指南.md` 命中。
  - failure：若 `grep "flash.ps1" Mac迁移配置指南.md` 命中"powershell -File"形式的**任务命令**（附录对照表里出现是允许的），说明误抄了 Windows 版 flash 任务 → 修正为 `bash .../flash.sh`。
  - 证据：把两条 grep 输出贴进执行日志。
- commit：`docs: add macOS migration guide (Mac迁移配置指南.md)`
- 依赖：无（单文件文档写入）。

### 2. [~] （可选，用户要求时才做｜SKIPPED：用户未要求）在 Windows 端校验文稿内引用的真实文件片段仍一致 - expect 无漂移
- WHERE: 只读比对 `新手配置指南.md`、`scripts/flash.ps1`、`.vscode/tasks.json`、`ozone/yuntai.jdebug`
- HOW: 若用户在 push 前又改过这些文件，重新核对文稿里"逐字照抄"的块。
- WHY: 保证 Mac 端 clone 到的内容和指南描述一致。
- QA：diff 无差异；有差异则更新文稿对应块。
- commit：`docs: sync migration guide with latest source configs`
- 依赖：Todo 1。

## Dependency matrix
- Todo 1：独立，先做。
- Todo 2：依赖 Todo 1，且仅在用户明确要求时执行。

## Test strategy
- 无自动化测试框架（这是文档交付物）。QA = agent 执行的 grep 断言（见 Todo 1）。文档在 Mac 端的真实验证由指南自身的"第10章 逐项验证清单"完成（编译/clangd/DAP烧录/J-Link烧录/调试/Ozone/多工程八项）。

---

## 完整文稿（worker 逐字写入 `Mac迁移配置指南.md`）

# Mac 迁移配置指南｜在全新 Apple Silicon Mac 上复刻 STM32 开发环境 + VSCode 配置

> 本文给**在 Mac 上执行的 agent** 看：从一台全新 M 系列（M5）MacBook Pro，到"多工程一键编译 / 烧录 / 调试 + clangd 智能提示 + DAP/J-Link 自动切换 + Ozone 调试"，全流程照做即可。
>
> 配套的 Windows 原始文档是同目录的 `新手配置指南.md`（14 章）。本文是它的 **macOS 移植版 + 迁移操作手册**，凡本文没覆盖的细节（各配置文件"为什么这么写"）都可回查那份。
>
> **执行者须知（重要）**：
> - 目标机型：**Apple Silicon（M5）**，Homebrew 前缀为 `/opt/homebrew`，所有工具链走原生 arm64，无需 Rosetta。
> - 代码通过 **GitHub** 迁移：用户已把两个工程分别推到远程私有仓库，本文第 2 章负责 clone。
> - 每完成一章末尾的"验证"，打印结果确认通过再进下一章。任何一步失败，先按该章的"排障"处理，不要跳过。

---

## 第0章 迁移全景（先读这一段）

Windows 那套东西分三类，迁移方式各不同：

| 类别 | 具体内容 | 怎么过来 Mac |
| --- | --- | --- |
| **工具链** | arm-none-eabi-gcc / cmake / ninja / openocd / J-Link / Ozone | Mac 上**重新安装**（换 Homebrew + SEGGER macOS 版），第 1 章 |
| **仓库内配置（跟代码走）** | `CMakeLists.txt`、`cmake/`、`CMakePresets.json`、`openocd.cfg`、`.clangd`、`.vscode/tasks.json`、`.vscode/settings.json` | git clone 自动带过来（跨平台可移植），但 **tasks.json 的 flash 命令是 Windows powershell，需改**，第 5 章 |
| **不跟代码走的东西** | `.vscode/launch.json`（被 .gitignore 忽略）、`build/`（忽略）、VSCode 用户级快捷键、VSCode 扩展 | Mac 上**重建 / 重装**，第 4、6、7 章 |

**四个必须处理的平台差异**（这是移植的核心，不是照抄）：
1. **烧录脚本**：`scripts/flash.ps1`（PowerShell，纯 Windows）→ 重写为 `scripts/flash.sh`（bash），第 5 章。
2. **快捷键**：Windows 用 `Alt`，Mac 改用 `Cmd`（`Cmd+,` 编译 / `Cmd+M` 编译加烧录 / `Cmd+N` 烧录），第 6 章。
3. **launch.json**：被 git 忽略，clone 不会带过来，需在 Mac 重建，第 7 章。
4. **Ozone 工程文件** `ozone/yuntai.jdebug` 里写死了 Windows 老路径（`D:/Newcode/...`），需改成 Mac 路径，第 8 章。

**工程结构**（clone 后 Mac 上的目标布局，两个平级 + 一个工作区文件）：

```
~/Newcode/                                    ← 自定，下文统一用这个根
├─ Gimbal_26hero_CH010_20260725/              ← 云台工程（含本指南）
├─ Chassisl_26_SHANGTAIJIE/                   ← 底盘工程
└─ gimbal-chassis.code-workspace              ← 多根工作区（见第 9 章）
```

> `gimbal-chassis.code-workspace` 是**独立于两个工程仓库的一个文件**，Windows 上它在 `D:\Newcode\` 下。迁移时需单独带过来或按第 9 章重建。

---

## 第1章 安装工具链（Homebrew + SEGGER）

### 1.1 装 Homebrew（若没装）

```bash
# 官方安装脚本；装完按提示把 brew 加进 PATH（Apple Silicon 在 /opt/homebrew）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 按安装结束的提示执行（通常是这两行），把 brew 加进当前 shell 和 ~/.zprofile：
echo >> ~/.zprofile
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"

brew --version   # 验证：打出版本号
```

### 1.2 装编译 / 构建工具链

```bash
# arm-none-eabi 工具链（Arm 官方预编译包的 cask，原生 arm64，含 gcc/g++/gdb/objcopy/size）
brew install --cask gcc-arm-embedded

# 构建工具
brew install cmake ninja
```

> ⚠ **OpenOCD 有坑，看清楚再装**：Homebrew 的默认 `open-ocd` 公式**与 cortex-debug 调试插件不兼容**（cortex-debug 官方明确警告 macOS 不要用 brew 默认版）。为兼顾"命令行烧录"和"VSCode 调试"，本文用 **xPack 版 OpenOCD**（一个自包含的解压即用包），不用 brew 那个。

安装 xPack OpenOCD（原生 arm64）：

```bash
# 1) 去 https://github.com/xpack-dev-tools/openocd-xpack/releases 下最新的 macOS arm64 包
#    文件名形如 xpack-openocd-<版本>-darwin-arm64.tar.gz
# 2) 解压到 ~/opt（示例，版本号按实际改）
mkdir -p ~/opt
tar -xzf ~/Downloads/xpack-openocd-*-darwin-arm64.tar.gz -C ~/opt

# 3) 把它的 bin 加进 PATH（写进 ~/.zprofile 永久生效；路径按解压出的实际目录改）
echo 'export PATH="$HOME/opt/xpack-openocd-0.12.0-6/bin:$PATH"' >> ~/.zprofile
source ~/.zprofile

openocd --version   # 验证
```

> macOS 首次运行可能弹"无法验证开发者"——到「系统设置 → 隐私与安全性」点"仍要打开"，或对该二进制执行 `xattr -dr com.apple.quarantine ~/opt/xpack-openocd-*`。

### 1.3 装 SEGGER J-Link 软件包（用 J-Link 的必装）

去 <https://www.segger.com/downloads/jlink/> 下 **"J-Link Software and Documentation Pack"** 的 **macOS Apple Silicon** 安装器（有专门的 Apple Silicon 版；也有 Universal 版）。装完：

- CLI 工具是 **`JLinkExe`**（注意：macOS 上叫 `JLinkExe`，**不是** Windows 的 `JLink.exe`），默认在 `/Applications/SEGGER/JLink_Vxxx/`（`xxx` 是版本号）。
- 同时装了 `JLinkGDBServer`（VSCode 调试 J-Link 时用）。
- macOS 上 J-Link 走用户态 USB，**没有内核驱动 / kext 要签**，插上即用。

把 JLinkExe 所在目录加进 PATH（版本号按实际改）：

```bash
echo 'export PATH="/Applications/SEGGER/JLink_V794a:$PATH"' >> ~/.zprofile
source ~/.zprofile
JLinkExe -? 2>/dev/null | head -n 3   # 验证：打出 SEGGER J-Link Commander 版本信息
```

> 只用 CMSIS-DAP（正点原子 DAP-Link）的可**跳过本节**：DAP 在 macOS 上同样免驱（走 HID），OpenOCD 直接能用。但既然你要"两个都要自动切换"，建议还是装上 J-Link 包。

### 1.4 装 VSCode 与 CLI 命令

1. 从 <https://code.visualstudio.com/> 下 macOS（Apple Silicon）版，拖进「应用程序」。
2. 打开 VSCode → `Cmd+Shift+P` → 运行 **`Shell Command: Install 'code' command in PATH`**（这样终端里 `code` 命令才可用，后面装扩展要）。

```bash
code --version   # 验证
```

### 1.5 本章验证（全部打出版本号才算过）

```bash
arm-none-eabi-gcc --version
cmake --version
ninja --version
openocd --version
JLinkExe -? 2>/dev/null | head -n 1   # 用 J-Link 才需要
code --version
```

哪条报 `command not found`，就是那个工具没进 PATH：重开一个新终端（或 `source ~/.zprofile`）再试；仍不行就回本章检查对应的 PATH 行是否写到了含二进制的那层目录。

---

## 第2章 从 GitHub 克隆两个工程

> 前提：用户已把 **两个工程分别推送到 GitHub 远程仓库**（它们在 Windows 上是各自独立的 git 仓库）。仓库地址：
> - 云台：`https://github.com/zy-2006-08/Gimbal_26hero_CH010_20260725`
> - 底盘：`https://github.com/zy-2006-08/Chassisl_26_SHANGTAIJIE`

```bash
mkdir -p ~/Newcode && cd ~/Newcode

# 云台工程（含本指南）：
git clone https://github.com/zy-2006-08/Gimbal_26hero_CH010_20260725 Gimbal_26hero_CH010_20260725

# 底盘工程：
git clone https://github.com/zy-2006-08/Chassisl_26_SHANGTAIJIE Chassisl_26_SHANGTAIJIE
```

> 上面第二参数显式指定了目录名，确保 clone 出的目录名（`Gimbal_26hero_CH010_20260725` / `Chassisl_26_SHANGTAIJIE`）与第 9 章工作区文件里的 `path` **完全一致**。私有仓库首次 clone 会要求登录（浏览器授权或 Personal Access Token）。
>
> **多根工作区文件 `gimbal-chassis.code-workspace` 不在这两个仓库里**（它在 Windows 的 `D:\Newcode\` 上层目录，不属于任一仓库），所以 clone **不会**带来它——由第 9 章在 Mac 上重新创建。

**验证**：两个目录都存在，且各自含 `CMakeLists.txt`、`CMakePresets.json`、`cmake/`、`openocd.cfg`、`.clangd`、`scripts/`、`.vscode/tasks.json`：

```bash
cd ~/Newcode
for p in Gimbal_26hero_CH010_20260725 Chassisl_26_SHANGTAIJIE; do
  echo "== $p =="
  ls "$p"/CMakeLists.txt "$p"/CMakePresets.json "$p"/openocd.cfg "$p"/.clangd "$p"/.vscode/tasks.json 2>&1
done
```

> **注意 clone 不会带来的东西**（.gitignore 忽略了）：`build/`（编译产物，本来就该重建）和 `.vscode/launch.json`（调试配置，第 7 章重建）。这两个缺失是**正常**的。也正因为是全新 clone、没有旧 `build/`，你**不会**遇到 Windows 文档第 14 章那种"搬家后 CMake 缓存记死旧绝对路径"的报错——干净起步。

---

## 第3章 校验仓库内构建配置（通常无需改）

这些文件跨平台可移植，clone 已带来，正常不用动。**只做一次通读确认**，重点看工具链文件里没有写死 Windows 专属路径。

要点（详细解释见 `新手配置指南.md` 第 3、4 章）：
- `CMakeLists.txt`：工程名 `Gimbal_Demo`，含 `.c` 强制按 C++ 编 + `-fpermissive` 的处理。**跨平台通用，不用改。**
- `cmake/gcc-arm-none-eabi.cmake`：编译器前缀 `arm-none-eabi-`（靠 PATH 找，第 1 章已装），F405 芯片 flag。**跨平台通用，不用改。**
- `CMakePresets.json`：generator=Ninja，`binaryDir=${sourceDir}/build/${presetName}`，toolchainFile 用 `${sourceDir}` 相对路径。**跨平台通用，不用改。**

**验证**（确认工具链文件靠 PATH 找编译器、没有写死绝对路径）：

```bash
cd ~/Newcode/Gimbal_26hero_CH010_20260725
grep -nE "arm-none-eabi-|C:\\\\|/c/|Program Files" cmake/gcc-arm-none-eabi.cmake
```
应只看到 `TOOLCHAIN_PREFIX arm-none-eabi-` 这类相对引用，**不应**出现任何 `C:\` 或 Windows 绝对路径。若出现，把该行改成靠 PATH 解析的相对形式（即只保留 `arm-none-eabi-gcc` 而非绝对路径）。

---

## 第4章 装 VSCode 扩展 + 关微软 IntelliSense

```bash
code --install-extension llvm-vs-code-extensions.vscode-clangd
code --install-extension marus25.cortex-debug
```

- **clangd**（LLVM）：写代码补全 / 跳转 / 实时报错。首次启用会弹窗提示下载 clangd 语言服务器本体，**同意下载**。
- **cortex-debug**（marus25）：STM32 调试底层。

关掉微软 C/C++ 的 IntelliSense（避免和 clangd 打架）：

- 若已装微软 C/C++ 插件，clangd 会弹"检测到与 Microsoft C++ 冲突"，点 **`Disable IntelliSense`**。
- **不要卸载**微软 C/C++ 插件（cortex-debug 依赖其部分能力）；只关它的 IntelliSense。
- 若没装微软 C/C++ 插件，可忽略——clangd 直接接管即可。

> clangd 找编译数据库靠每个工程根的 `.clangd`（内容 `CompilationDatabase: build/Debug`），clone 已带来，无需改；但要等第 10 章编译过一次生成 `build/Debug/compile_commands.json` 后才生效。

**验证**：VSCode 扩展面板里 clangd、Cortex-Debug 均显示已安装启用。

---

## 第5章 重写烧录脚本 flash.sh + 改 tasks.json（核心移植）

Windows 的 `scripts/flash.ps1` 是 PowerShell，Mac 跑不了。**在每个工程的 `scripts/` 下新建 `flash.sh`**，逻辑与 Windows 版一致：扫 USB 找 SEGGER J-Link（VID `0x1366`），有就用 `JLinkExe` 烧 `.hex`，没有就回退 OpenOCD 烧 `.elf`。

### 5.1 新建 `scripts/flash.sh`（两个工程各放一份，内容完全一致，逐字照抄）

在 `~/Newcode/Gimbal_26hero_CH010_20260725/scripts/flash.sh` 和 `~/Newcode/Chassisl_26_SHANGTAIJIE/scripts/flash.sh` 各写入：

```bash
#!/usr/bin/env bash
# Auto-detect flasher (J-Link or CMSIS-DAP) and flash firmware — macOS version.
# Same single shortcut, "plug in and flash":
#   - If a SEGGER J-Link (USB VID 0x1366) is present -> flash .hex with JLinkExe (native, fast).
#   - Otherwise -> fall back to OpenOCD + CMSIS-DAP, flashing .elf. (unchanged DAP behavior)
# Mirrors the Windows scripts/flash.ps1.
set -euo pipefail

ELF="${1:-build/Debug/Gimbal_Demo.elf}"
HEX="${2:-build/Debug/Gimbal_Demo.hex}"

# Project root = parent of this script's dir (scripts/ -> root), so relative paths resolve.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# --- Detect J-Link (USB VID 0x1366) --------------------------------------
if system_profiler SPUSBDataType 2>/dev/null | grep -qi "0x1366"; then
  echo "[flash] Detected J-Link (VID_1366), using SEGGER JLinkExe"

  # Find JLinkExe: PATH first, then common install dir.
  JLINK="$(command -v JLinkExe || true)"
  if [ -z "$JLINK" ]; then
    JLINK="$(ls -d /Applications/SEGGER/JLink_V*/JLinkExe 2>/dev/null | sort | tail -n 1 || true)"
  fi
  if [ -z "$JLINK" ]; then
    echo "[flash] ERROR: JLinkExe not found in PATH or /Applications/SEGGER/JLink_V*/" >&2
    exit 1
  fi
  if [ ! -f "$HEX" ]; then
    echo "[flash] ERROR: HEX not found: $HEX (build first?)" >&2
    exit 1
  fi

  HEX_ABS="$(cd "$(dirname "$HEX")" && pwd)/$(basename "$HEX")"
  TMP="$(mktemp -t flash_jlink).jlink"
  cat > "$TMP" <<EOF
si SWD
speed 8000
device STM32F405RG
connect
loadfile "$HEX_ABS"
r
g
exit
EOF
  trap 'rm -f "$TMP"' EXIT
  "$JLINK" -CommanderScript "$TMP" -ExitOnError 1 -NoGui 1
  exit $?
fi

# --- DAP path: OpenOCD (unchanged behavior) ------------------------------
echo "[flash] No J-Link found, using CMSIS-DAP + OpenOCD"

OPENOCD="$(command -v openocd || true)"
if [ -z "$OPENOCD" ]; then
  echo "[flash] ERROR: openocd not found in PATH" >&2
  exit 1
fi
if [ ! -f "$ELF" ]; then
  echo "[flash] ERROR: ELF not found: $ELF (build first?)" >&2
  exit 1
fi

echo "[flash] Config: openocd.cfg | ELF: $ELF"
"$OPENOCD" -f "openocd.cfg" -c "program \"$ELF\" verify reset exit"
```

给两份脚本加可执行权限：

```bash
chmod +x ~/Newcode/Gimbal_26hero_CH010_20260725/scripts/flash.sh
chmod +x ~/Newcode/Chassisl_26_SHANGTAIJIE/scripts/flash.sh
```

> 逐段对应 Windows 版：定位工程根 → `system_profiler` 扫 VID_1366 检测 J-Link → J-Link 分支（找 JLinkExe → 生成临时 `.jlink` 命令脚本：SWD/8MHz/STM32F405RG/连接/烧 hex/复位运行/退出 → 用完删）→ DAP 回退分支（OpenOCD 烧 elf，行为不变）。两个工程产物名都是 `Gimbal_Demo`、都是 STM32F405RG，脚本通用无需改。

### 5.2 改每个工程的 `.vscode/tasks.json` 里 flash 任务

clone 来的 `tasks.json` 里 flash 任务调的是 Windows 的 `powershell ... flash.ps1`，改成调 `flash.sh`。**两个工程都要改**。把 `.vscode/tasks.json` 内容整体替换为（三任务统一用 `${fileWorkspaceFolder}`，多工程通用）：

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
            "command": "bash \"${fileWorkspaceFolder}/scripts/flash.sh\"",
            "options": {
                "cwd": "${fileWorkspaceFolder}"
            },
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

- **build**：和 Windows 一致（`cmake --build --preset Debug`），跨平台通用。
- **flash**：由 `powershell -File ...flash.ps1` 改为 `bash "${fileWorkspaceFolder}/scripts/flash.sh"`。macOS 路径用正斜杠，不需要 Windows 那种 `\\` 转义。
- **build and flash**：`dependsOn` 顺序不变，自动继承改后的 flash。

**验证**（生成产物后，见第 10 章；这里先确认脚本能独立跑）：

```bash
cd ~/Newcode/Gimbal_26hero_CH010_20260725
bash scripts/flash.sh 2>&1 | head -n 3
# 未编译时应报 "ELF/HEX not found (build first?)" —— 说明脚本逻辑正常，只是还没产物。
# 若打印 [flash] Detected J-Link 或 [flash] No J-Link found，说明检测分支工作正常。
```

---

## 第6章 快捷键（Cmd+, / Cmd+M / Cmd+N）

macOS 用 `Cmd`，不用 Windows 的 `Alt`。映射（用户指定）：

- `Cmd+,` → `build`
- `Cmd+M` → `build and flash`
- `Cmd+N` → `flash`

> ⚠ `Cmd+,` 在 macOS VSCode 默认是"打开设置"，下面的绑定会**覆盖**它（在编辑器聚焦时）。这是有意为之，符合用户要求。

打开：VSCode → `Cmd+Shift+P` → **`Preferences: Open Keyboard Shortcuts (JSON)`**（打开当前激活 profile 的 `keybindings.json`，文件位于 `~/Library/Application Support/Code/User/keybindings.json`）。把下面几条加进该 JSON 数组（逐字照抄）：

```json
[
    {
        "key": "cmd+,",
        "command": "workbench.action.tasks.runTask",
        "args": "build",
        "when": "editorTextFocus"
    },
    {
        "key": "cmd+m",
        "command": "workbench.action.tasks.runTask",
        "args": "build and flash"
    },
    {
        "key": "cmd+n",
        "command": "workbench.action.tasks.runTask",
        "args": "flash"
    },
    {
        "key": "cmd+,",
        "command": "-workbench.action.openSettings",
        "when": "editorTextFocus"
    }
]
```

- `args` 要和第 5 章任务的 `label` **一字不差**（`build` / `flash` / `build and flash`）。
- 最后一条用 `-workbench.action.openSettings` 解绑默认的"打开设置"，避免 `Cmd+,` 冲突。
- `Cmd+N` 默认是"新建文件"；上面的绑定在无 `when` 限制下会覆盖它。若你想保留"新建文件"，把 `Cmd+N` 换成别的键（如 `Cmd+Shift+N`）并告知用户。

> 坑（同 Windows 文档第 7 章）：VSCode 每个 profile 有独立 keybindings.json，写错 profile 就没反应；别选带 `(Default)` 的只读内置表。

**验证**：在某工程的 `.c/.cpp` 文件里按 `Cmd+,`，底部终端应跑起 `cmake --build --preset Debug`。

---

## 第7章 重建 launch.json（cortex-debug 调试）

`launch.json` 被 `.gitignore` 忽略（`/.vscode/launch.json`），clone **不会带来**，需在 Mac 重建。**两个工程各建一份** `.vscode/launch.json`。

macOS 上有两种调试后端，二选一（也可两个都放进同一个 `configurations` 数组，调试时下拉选）：

### 7.1 方案 A：CMSIS-DAP + OpenOCD（免驱，推荐日常用 DAP 时）

`~/Newcode/Gimbal_26hero_CH010_20260725/.vscode/launch.json`（底盘同理）：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "OpenOCD Debug (CMSIS-DAP)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/Debug/Gimbal_Demo.elf",
            "device": "STM32F405RG",
            "configFiles": [
                "${workspaceFolder}/openocd.cfg"
            ],
            "runToEntryPoint": "main",
            "preLaunchTask": "build",
            "showDevDebugOutput": "none",
            "serverpath": "REPLACE_WITH_XPACK_OPENOCD/bin/openocd"
        }
    ]
}
```

> **必改**：`serverpath` 指向第 1.2 章装的 **xPack OpenOCD**（如 `${env:HOME}/opt/xpack-openocd-0.12.0-6/bin/openocd`）。原因：cortex-debug 官方说 Homebrew 默认 openocd 与它不兼容——虽然本文没用 brew 版，但要显式告诉 cortex-debug 用 xPack 那个二进制，别去找系统里其它 openocd。若你已把 xPack 的 bin 放在 PATH 最前，且 `command -v openocd` 就是它，可省略 `serverpath` 行。

### 7.2 方案 B：J-Link（JLinkGDBServer，用 J-Link 调试且要原生速度时）

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "J-Link Debug",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "jlink",
            "cwd": "${workspaceFolder}",
            "executable": "${workspaceFolder}/build/Debug/Gimbal_Demo.elf",
            "device": "STM32F405RG",
            "interface": "swd",
            "runToEntryPoint": "main",
            "preLaunchTask": "build",
            "showDevDebugOutput": "none"
        }
    ]
}
```

> 需第 1.3 章的 J-Link 包（提供 `JLinkGDBServer`）。cortex-debug 一般能自动找到它；找不到时在配置里加 `"serverpath": "/Applications/SEGGER/JLink_Vxxx/JLinkGDBServerCLExe"`（版本号按实际改）。

**验证**：先按第 10 章编译一次生成 `.elf`，再按 `F5`（或运行面板选对应配置），能进入 `main` 断点即成功。

---

## 第8章 修 Ozone 工程文件（写死了 Windows 老路径）

`ozone/yuntai.jdebug` 里有**写死的 Windows 绝对路径**（指向老工程 `D:/Newcode/Gimbal_24hero_CH010_20260111/...`），在 Mac 上无效，必须改。

需要改的两处（原文）：

```
Project.AddPathSubstitute ("D:/Newcode/Gimbal_24hero_CH010_20260111/ozone", "$(ProjectDir)");
Project.AddPathSubstitute ("d:/newcode/gimbal_24hero_ch010_20260111/ozone", "$(ProjectDir)");
...
File.Open ("D:/Newcode/Gimbal_24hero_CH010_20260111/build/Debug/Gimbal_Demo.elf");
```

改法：
1. 两行 `AddPathSubstitute` 直接**删掉**（它们是把老 Windows 绝对路径替换成 `$(ProjectDir)`，Mac 上没有这些老路径，留着无害但无意义；删掉最干净）。
2. `File.Open(...)` 改成 Mac 上的实际 ELF 路径（用 `$(ProjectDir)` 相对最稳，`$(ProjectDir)` 是 `.jdebug` 所在的 `ozone/` 目录）：

```
File.Open ("$(ProjectDir)/../build/Debug/Gimbal_Demo.elf");
```

其余（`SetDevice STM32F405RG`、`SetTargetIF SWD`、`SetTIFSpeed 4 MHz`、SVD 文件等）跨平台通用，不动。

安装 Ozone：去 <https://www.segger.com/downloads/jlink/#Ozone> 下 macOS（Apple Silicon）版，装进「应用程序」。

**验证**：Ozone 打开 `ozone/yuntai.jdebug`，能加载到 `build/Debug/Gimbal_Demo.elf`（先编译过），连上 J-Link 看到 `Found Cortex-M4` 即通。

> 底盘工程若也有 `ozone/*.jdebug` 且含写死路径，同样处理。

---

## 第9章 多根工作区文件（gimbal-chassis.code-workspace）

这个文件在 Windows 上位于 `D:\Newcode\`（两个工程的上层），**不属于任何一个工程仓库**，所以 clone 两个工程不会带来它。在 Mac 的 `~/Newcode/` 下新建 `gimbal-chassis.code-workspace`：

```json
{
    "folders": [
        {
            "name": "Gimbal (云台)",
            "path": "Gimbal_26hero_CH010_20260725"
        },
        {
            "name": "Chassis (底盘)",
            "path": "Chassisl_26_SHANGTAIJIE"
        }
    ],
    "settings": {
        "C_Cpp.errorSquiggles": "disabled"
    }
}
```

- `path` 相对 `.code-workspace` 所在目录（`~/Newcode`），**必须与第 2 章 clone 出的实际目录名一致**。若 clone 目录名不同，改这里。
- 任务不放这里（放各工程的 tasks.json，靠 `${fileWorkspaceFolder}` 分流，原理见 `新手配置指南.md` 第 12 章）。
- 用法：双击此文件用 VSCode 打开，侧栏出现"Gimbal (云台)"和"Chassis (底盘)"两个根；在哪个工程的文件里按快捷键，就编 / 烧哪个工程。

**验证**：双击打开工作区，侧栏两个根都正常展开（不空、不标红）；在云台文件里 `Cmd+,` 编云台，切到底盘文件 `Cmd+,` 编底盘。

---

## 第10章 首次编译 + 一键流验证（收尾）

对**每个工程**首次编译一次（生成 `build/Debug/` 产物 + `compile_commands.json`）：

```bash
cd ~/Newcode/Gimbal_26hero_CH010_20260725
cmake --preset Debug          # 配置（生成 CMakeCache + compile_commands.json）
cmake --build --preset Debug  # 编译

cd ~/Newcode/Chassisl_26_SHANGTAIJIE
cmake --preset Debug
cmake --build --preset Debug
```

看到 `Linking CXX executable Gimbal_Demo.elf` 和内存占用表（`FLASH: xx%`）即编译成功。

**逐项验证清单**（全绿才算迁移完成）：

1. **编译**：VSCode 打开工程，`.c/.cpp` 里按 `Cmd+,` → 终端跑 `cmake --build --preset Debug`，无 `error:`。
2. **clangd 提示**：`Cmd+Shift+P` → `clangd: Restart language server`，等右下角索引完；打开 `.c/.cpp`，输入 `结构体名.` 弹成员补全、红波浪线消失、`Cmd+点击` 能跳转。
   - 波浪线不消：多半没重启 clangd、或还没编译过（无 `compile_commands.json`）、或 `.clangd` 不在工程根。
3. **烧录（DAP）**：插 CMSIS-DAP（只插一个烧录器），`Cmd+N`，终端首行应打印 `[flash] No J-Link found, using CMSIS-DAP + OpenOCD` 并烧录成功。
4. **烧录（J-Link）**：改插 J-Link（拔掉 DAP），`Cmd+N`，终端首行应打印 `[flash] Detected J-Link (VID_1366), using SEGGER JLinkExe`，看到 `Found Cortex-M4` / `Programming flash [100%] Done` / `O.K.`。
5. **编译加烧录**：`Cmd+M` 先编后烧一键完成。
6. **调试**：`F5` 进入 `main` 断点（cortex-debug，第 7 章）。
7. **Ozone**：打开 `ozone/yuntai.jdebug` 能连 J-Link 调试（第 8 章）。
8. **多工程**：双击 `gimbal-chassis.code-workspace`，在云台 / 底盘文件间切换，快捷键按当前文件分流到对应工程。

---

## 附录A：macOS 与 Windows 的关键差异对照

| 项 | Windows（原文档） | macOS（本文） |
| --- | --- | --- |
| 包管理 / 工具链 | 手动下载 + 配 PATH / Scoop | Homebrew（`/opt/homebrew`）+ cask，原生 arm64 |
| OpenOCD | xpack 解压 | **xPack 版**（brew 默认版与 cortex-debug 不兼容） |
| J-Link CLI | `JLink.exe` | **`JLinkExe`**，在 `/Applications/SEGGER/JLink_Vxxx/` |
| 检测 J-Link | `Get-PnpDevice ... VID_1366` | `system_profiler SPUSBDataType \| grep 0x1366` |
| 烧录脚本 | `scripts/flash.ps1`（PowerShell） | `scripts/flash.sh`（bash，需 `chmod +x`） |
| flash 任务命令 | `powershell -File ...flash.ps1` | `bash "${fileWorkspaceFolder}/scripts/flash.sh"` |
| 快捷键 | `Alt+,` / `Alt+N` / `Alt+M` | `Cmd+,` / `Cmd+N` / `Cmd+M` |
| keybindings 路径 | `%APPDATA%\Code\User\keybindings.json` | `~/Library/Application Support/Code/User/keybindings.json` |
| launch.json | git 忽略，需重建 | git 忽略，需重建（cortex-debug serverpath 指 xPack） |
| PATH 持久化 | 用户级环境变量（别用 setx） | `~/.zprofile` 里 export |

## 附录B：迁移后新增 / 改动的文件小结

- **新增** `~/Newcode/Gimbal_26hero_CH010_20260725/scripts/flash.sh` 和 `~/Newcode/Chassisl_26_SHANGTAIJIE/scripts/flash.sh`：macOS 版自动切换烧录脚本（`chmod +x`）。
- **改** 两个工程的 `.vscode/tasks.json`：flash 任务命令改为 `bash .../flash.sh`。
- **新增** 两个工程的 `.vscode/launch.json`：cortex-debug 调试配置（git 忽略，本地重建）。
- **改** `ozone/yuntai.jdebug`：删除写死的 Windows 老路径，`File.Open` 指向 `$(ProjectDir)/../build/Debug/Gimbal_Demo.elf`。
- **新增** `~/Newcode/gimbal-chassis.code-workspace`：多根工作区文件（若未随代码带来）。
- **用户级新增**：`~/Library/Application Support/Code/User/keybindings.json` 里几条 Cmd 快捷键。
- **未动**（clone 自带、跨平台通用）：`CMakeLists.txt`、`cmake/gcc-arm-none-eabi.cmake`、`CMakePresets.json`、`openocd.cfg`、`.clangd`、`.vscode/settings.json`、固件代码。

## 附录C：常见问题速查

- **`command not found`（gcc/cmake/openocd/JLinkExe）**：PATH 没生效。重开终端或 `source ~/.zprofile`；确认第 1 章对应 export 行写对了 bin 目录。
- **openocd 报"无法验证开发者"**：`xattr -dr com.apple.quarantine ~/opt/xpack-openocd-*`，或系统设置里放行。
- **`flash.sh` 权限拒绝 / not executable**：`chmod +x scripts/flash.sh`。
- **`bash: build/Debug/...elf not found`**：还没编译，先 `Cmd+,` 或 `cmake --build --preset Debug`。
- **DAP 和 J-Link 同时插**：会抢 SWD 总线，只插一个。
- **clangd 满屏红波浪线但能编过**：没编译过（无 `compile_commands.json`）、没重启 clangd、或 `.clangd` 不在工程根。`Cmd+Shift+P` → `clangd: Restart language server`。
- **cortex-debug 调试起不来 / openocd 版本报错**：`serverpath` 没指向 xPack OpenOCD（第 7.1 章），或用了 brew 默认版（不兼容）。
- **`Cmd+,` 打开的是设置而不是编译**：第 6 章解绑 `-workbench.action.openSettings` 那条没加，或写进了错的 profile。
