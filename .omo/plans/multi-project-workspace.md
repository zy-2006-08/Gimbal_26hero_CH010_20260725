# 多工程共用快捷键工作区 (Gimbal + Chassis)

## TL;DR (For humans)

把云台和底盘两个独立 STM32 工程放进**一个 VSCode 多根工作区**，实现"同一套快捷键 `Alt+, / Alt+N / Alt+M`，按当前编辑的文件自动编译烧录对应工程"。

**方案（方案一：按当前文件自动分流）**：VSCode 多根工作区里，运行 build/flash 任务时会自动使用"当前活动文件所属那个 folder 的 `.vscode/tasks.json`"。因此**两个工程各自保留独立的 tasks.json 即可，快捷键天然按当前文件分流**——无需合并成一套参数化脚本。

**为什么可行**：两个工程配置高度一致（都 STM32F405RG、产物都叫 `Gimbal_Demo.elf/.hex`、openocd.cfg 相同），只需给底盘补上和云台一样的 `flash.ps1`（DAP/J-Link 自动切换）并改 flash 任务即可对齐。

**改动清单（3 个文件）**：
1. 新建 `D:\Newcode\gimbal-chassis.code-workspace`（多根工作区）
2. 新建 `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\scripts\flash.ps1`（复制云台同名脚本，逐字相同）
3. 改 `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\tasks.json` 的 flash 任务（openocd 写死 → 调脚本）

**你现有的快捷键、云台工程的所有文件都不用动。**

---

## Context / Findings

调研已完成，事实如下：

- **两个工程平级根目录**：
  - 云台：`D:\Newcode\Gimbal_24hero_CH010_20260111`
  - 底盘：`D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE`（注意底盘多一层嵌套）
- **两个工程 `CMAKE_PROJECT_NAME` 都是 `Gimbal_Demo`**（底盘文件夹名叫 Chassis 但工程名没改）→ 产物都是 `build/Debug/Gimbal_Demo.elf` / `.hex`，路径一致，脚本无需改名。
- **两边 `CMakePresets.json`**：都有 Debug/RelWithDebInfo/Release/MinSizeRel，`binaryDir = ${sourceDir}/build/${presetName}`。因两工程在各自独立根目录，build 目录天然隔离，不会打架。
- **两边 `openocd.cfg` 完全相同**（cmsis-dap + swd + stm32f4x + speed 4000）。
- **云台已有** `scripts/flash.ps1`（DAP/J-Link 自动切换，84 行），且其 tasks.json flash 已改为调脚本：`powershell -ExecutionPolicy Bypass -File "${workspaceFolder}\scripts\flash.ps1"`。
- **底盘缺失** `scripts/` 目录，其 tasks.json flash 仍是写死的 openocd 命令：`openocd -f openocd.cfg -c "program build/Debug/Gimbal_Demo.elf verify reset exit"`。
- **两边 launch.json** 均为 cortex-debug + openocd + CMSIS-DAP，`${workspaceFolder}` 相对，多根工作区下各自解析正确，无需改动。
- **快捷键**（用户 keybindings.json，全局）：`Alt+,`→build、`Alt+N`→flash、`Alt+M`→build and flash，用 `workbench.action.tasks.runTask` + label。多根工作区下按当前活动文件所属 folder 解析 task，无需改动。

### 关键技术依据（方案一为何成立）

VSCode 多根工作区（`.code-workspace`）中，`workbench.action.tasks.runTask` 以 label 触发任务时，会在**当前活动编辑器文件所属的 workspace folder** 里查找同名 task 并执行，该 folder 的 `${workspaceFolder}` 解析为该工程根。因此在云台文件里按 `Alt+M` 跑云台的 build+flash，切到底盘文件按 `Alt+M` 跑底盘的——正是用户要的"按当前文件自动分流"，且不需要合并 task 或参数化。

---

## Todos

### 1. [x] 新建多根工作区文件 `D:\Newcode\gimbal-chassis.code-workspace`

**WHERE**: `D:\Newcode\gimbal-chassis.code-workspace`（新建）
**HOW**: 写入以下内容 —— 两个 folder 分别指向云台和底盘工程根：

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

**EXPECT**: 双击此文件用 VSCode 打开后，侧栏出现两个根 "Gimbal (云台)" 和 "Chassis (底盘)"。
**注意**: `path` 相对于 `.code-workspace` 所在目录 `D:\Newcode`。底盘路径含嵌套层，逐字保留。

**QA**:
- happy：用 VSCode 打开 `gimbal-chassis.code-workspace`，确认资源管理器显示两个工程根，各自能展开看到 Core/、CMakeLists.txt。
- failure：若某个根显示为空或路径错误 → 核对 path 拼写（尤其底盘的双层 `Chassisl_26_SHANGTAIJIE_SEND_DM/Chassisl_26_SHANGTAIJIE`）。

### 2. [x] 给底盘工程新建 `scripts\flash.ps1`（与云台逐字相同）

**WHERE**: `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\scripts\flash.ps1`（新建，需先建 `scripts\` 目录）
**HOW**: 内容与云台 `D:\Newcode\Gimbal_24hero_CH010_20260111\scripts\flash.ps1` **完全一致**（84 行，见下方完整内容）。因两工程产物名都是 `Gimbal_Demo`、芯片都是 STM32F405RG、openocd.cfg 相同，脚本无需任何修改即可通用。

完整内容：

```powershell
<#
.SYNOPSIS
    Auto-detect flasher (J-Link or CMSIS-DAP) and flash firmware.

.DESCRIPTION
    Same single shortcut, "plug in and flash". Scans USB for a Segger J-Link
    (VID 0x1366). If present, flashes with Segger's own JLink.exe (native,
    fast, uses the installed Segger driver). Otherwise falls back to OpenOCD
    with the CMSIS-DAP config. The DAP path is unchanged from before.

.PARAMETER Elf
    Path to the .elf (OpenOCD/DAP path). Default build/Debug/Gimbal_Demo.elf
.PARAMETER Hex
    Path to the .hex (JLink.exe path). Default build/Debug/Gimbal_Demo.hex
#>
param(
    [string]$Elf = "build/Debug/Gimbal_Demo.elf",
    [string]$Hex = "build/Debug/Gimbal_Demo.hex"
)

$ErrorActionPreference = "Stop"

# Project root = parent of this script's directory (scripts/ -> root)
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot

# --- Detect J-Link (VID_1366) --------------------------------------------
$jlink = Get-PnpDevice -ErrorAction SilentlyContinue |
    Where-Object { $_.InstanceId -match "VID_1366" -and $_.Status -eq "OK" } |
    Select-Object -First 1

if ($jlink) {
    # ---- J-Link path: use Segger's native JLink.exe -----------------------
    Write-Output "[flash] Detected J-Link (VID_1366), using Segger JLink.exe"

    $jlinkExe = (Get-Command JLink.exe -ErrorAction SilentlyContinue).Source
    if (-not $jlinkExe) {
        $fallback = "D:\SOFTWARE\JLink_V960\JLink.exe"
        if (Test-Path -LiteralPath $fallback) { $jlinkExe = $fallback }
        else { Write-Error "[flash] JLink.exe not found in PATH or $fallback"; exit 1 }
    }
    if (-not (Test-Path -LiteralPath $Hex)) {
        Write-Error "[flash] HEX not found: $Hex (build first?)"; exit 1
    }

    # Build a JLink CommanderScript: connect, program+verify, reset+run, exit
    $hexAbs = (Resolve-Path -LiteralPath $Hex).Path
    $script = @"
si SWD
speed 8000
device STM32F405RG
connect
loadfile "$hexAbs"
r
g
exit
"@
    $tmp = Join-Path $env:TEMP "flash_jlink_$PID.jlink"
    $script | Out-File -FilePath $tmp -Encoding ascii
    try {
        & $jlinkExe -CommanderScript $tmp -ExitOnError 1 -NoGui 1
        $code = $LASTEXITCODE
    } finally {
        Remove-Item $tmp -ErrorAction SilentlyContinue
    }
    exit $code
}

# ---- DAP path: OpenOCD (unchanged behavior) ------------------------------
Write-Output "[flash] No J-Link found, using CMSIS-DAP + OpenOCD"

$openocd = (Get-Command openocd -ErrorAction SilentlyContinue).Source
if (-not $openocd) {
    $fallback = "C:\Tools\xpack-openocd-0.12.0-7-win32-x64\xpack-openocd-0.12.0-7\bin\openocd.exe"
    if (Test-Path -LiteralPath $fallback) { $openocd = $fallback }
    else { Write-Error "[flash] openocd not found in PATH or $fallback"; exit 1 }
}
if (-not (Test-Path -LiteralPath $Elf)) {
    Write-Error "[flash] ELF not found: $Elf (build first?)"; exit 1
}

Write-Output "[flash] Config: openocd.cfg | ELF: $Elf"
& $openocd -f "openocd.cfg" -c "program `"$Elf`" verify reset exit"
exit $LASTEXITCODE
```

**EXPECT**: 底盘工程出现 `scripts\flash.ps1`，与云台版本 diff 为空。
**QA**:
- happy：文件存在，`Split-Path -Parent $PSScriptRoot` 逻辑保证从 `scripts/` 上溯到底盘工程根，相对路径 `build/Debug/Gimbal_Demo.elf`、`openocd.cfg` 对得上。
- failure：若烧录报 "ELF not found" → 先执行过 build 生成产物；若报 JLink.exe/openocd 路径 → 核对脚本内两处 `$fallback` 是否与本机安装路径一致（与云台相同，用户机器已验证可用）。

### 3. [x] 改底盘 `tasks.json` 的 flash 任务：openocd 写死 → 调脚本

**WHERE**: `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\tasks.json` 第 14-19 行的 flash 任务
**HOW**: 把 flash 任务的 command 从
```json
"command": "openocd -f openocd.cfg -c \"program build/Debug/Gimbal_Demo.elf verify reset exit\"",
```
改为（与云台一致）：
```json
"command": "powershell -ExecutionPolicy Bypass -File \"${workspaceFolder}\\scripts\\flash.ps1\"",
```
`build` 和 `build and flash` 两个任务**不动**（`build and flash` 靠 `dependsOn:[build,flash]` 自动继承新 flash）。

改完底盘 tasks.json 完整应为：
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "cmake --build --preset Debug",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "flash",
            "type": "shell",
            "command": "powershell -ExecutionPolicy Bypass -File \"${workspaceFolder}\\scripts\\flash.ps1\"",
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

**EXPECT**: 底盘的 flash 任务改为调 flash.ps1，与云台行为一致（DAP/J-Link 自动切换）。
**注意坑（来自新手指南第11.4）**：`${workspaceFolder}` 含反斜杠，路径必须加引号；JSON 里 `\\` 是反斜杠转义。照抄上面写法即可。

**QA**:
- happy：在多根工作区里打开底盘的任一源文件（如其 Core/Src 下的文件），按 `Alt+M`，终端首行应打印 `[flash] Detected J-Link ...` 或 `[flash] No J-Link found, using CMSIS-DAP + OpenOCD`，随后编译+烧录成功。
- failure：报 "-File 形式参数...不存在" 且路径中反斜杠被吃掉 → 引号/转义写错，核对为 `\"${workspaceFolder}\\scripts\\flash.ps1\"`。

---

## 最终验证（三个改动全部落地后）

1. 双击 `D:\Newcode\gimbal-chassis.code-workspace` 打开，确认两个工程根都在。
2. 打开**云台**的一个源文件（如 `Gimbal_24hero_CH010_20260111\Core\Src\my_main.cpp`），按 `Alt+M` → 应编译并烧录云台（产物 `Gimbal_24hero_CH010_20260111\build\Debug\Gimbal_Demo.elf`）。
3. 打开**底盘**的一个源文件，按 `Alt+M` → 应编译并烧录底盘（产物在底盘工程 build 目录）。
4. 分别按 `Alt+,`（只编）、`Alt+N`（只烧）验证各自独立生效。
5. 确认两工程 build 目录互不干扰（各在自己根目录下）。

**判定标准**：同一套快捷键，在哪个工程的文件里按，就作用于哪个工程；两工程产物在各自 build 目录，无覆盖。

---

---

## 修订 (REV2)：原方案失效，改用工作区级任务 + ${fileWorkspaceFolder}

### 问题
实测 `Alt+M` 永远只编云台。根因是 VSCode 已知限制（microsoft/vscode #227350、#144761）：
**多根工作区里，`workbench.action.tasks.runTask` 以同名 label 触发时，总执行"第一个根"里的同名任务，不按当前活动文件分流。** 云台是第一个根，故快捷键恒定跑云台。原 REV1 方案"两工程各留同名 tasks.json 自动分流"是错误前提。

### 正确方案（真实案例验证：github rivy/less 用 `options.cwd=${fileWorkspaceFolder}` 实现按当前文件构建）
在 `.code-workspace` 里定义**唯一一套**任务，命令/cwd 用 `${fileWorkspaceFolder}`（当前打开文件所属的工程根）。两工程各自 tasks.json 的同名任务清空，消除冲突。

### REV2 Todos

- [x] **R1** 改 `D:\Newcode\gimbal-chassis.code-workspace`：加 `"tasks"` 段（工作区级，version 2.0.0，仅 shell 类型），三任务用 `${fileWorkspaceFolder}` 定位当前工程：
  - build: `cmake --build --preset Debug`，`"options": { "cwd": "${fileWorkspaceFolder}" }`，group build+isDefault，problemMatcher `$gcc`
  - flash: `powershell -ExecutionPolicy Bypass -File "${fileWorkspaceFolder}\\scripts\\flash.ps1"`
  - build and flash: `dependsOrder sequence`, `dependsOn [build, flash]`
- [x] **R2** 清空云台 `D:\Newcode\Gimbal_24hero_CH010_20260111\.vscode\tasks.json` 的 tasks 数组为 `[]`（保留文件与 version），消除同名任务冲突。
- [x] **R3** 清空底盘 `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\tasks.json` 的 tasks 数组为 `[]`。
- [x] **R4** （被 REV3 取代）配置侧已完成核对。实机验证并入 R8。

### REV2 注意
- 工作区配置文件只允许 `shell`/`process` 类型任务（官方限制）——三任务都是 shell，OK。
- 快捷键 keybindings 不变（仍 runTask + label；现在全局只剩工作区这一套同名任务，不再有多根冲突）。
- flash.ps1 内部 `Set-Location` 到脚本上级，与 cwd 双保险。
- launch.json 调试仍各用各的（cortex-debug 按 folder，本就正确），不动。

---

---

## 修订 (REV3)：同时支持"单独打开工程文件夹"和"多根工作区"

### 新需求
用户要求：单独打开某个工程文件夹时也能用快捷键编译烧录（REV2 把两工程 tasks.json 清空了，导致单独打开工程时无任务可用）。

### 方案
把三任务放回**每个工程各自的 tasks.json**，命令统一用 `${fileWorkspaceFolder}`（而非 `${workspaceFolder}`）。移除工作区文件的 tasks 段（避免与 per-folder 重复定义）。
- 单独打开工程：`${fileWorkspaceFolder}` == 该工程根 → 正常编烧。
- 多根工作区：即使 VSCode 已知 bug 选了第一个根的任务定义，命令用 `${fileWorkspaceFolder}` 仍解析为当前文件所属工程 → 正确分流。
- 一份逻辑，两种模式通吃。

### REV3 Todos

- [x] **R5** 移除 `D:\Newcode\gimbal-chassis.code-workspace` 的 `"tasks"` 段，恢复为仅 folders + settings。
- [x] **R6** 云台 `D:\Newcode\Gimbal_24hero_CH010_20260111\.vscode\tasks.json`：恢复 build/flash/build-and-flash 三任务，build 用 `options.cwd=${fileWorkspaceFolder}`，flash 用 `${fileWorkspaceFolder}\\scripts\\flash.ps1`。
- [x] **R7** 底盘 `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\tasks.json`：同 R6，内容一致。
- [x] **R8** 配置侧已完成并核对：三文件 JSON 有效，两工程 tasks.json 均用 ${fileWorkspaceFolder}（单独打开工程 + 多根工作区两种模式通吃），工作区文件已移除 tasks 段。实机按键验证委托用户执行（重开 VSCode → 各模式按 Alt+,/Alt+M 看终端构建路径）。

### REV3 注意
- 三任务在两个 per-folder tasks.json 中内容完全一致（都用 ${fileWorkspaceFolder}）。
- 快捷键不变。
- flash.ps1 两工程各已就位，不动。

---

## Must-NOT-Have（防止范围外改动）

- **不改** 快捷键 keybindings.json（现有 `Alt+,/N/M` 直接复用）。
- **不改** 云台工程的任何文件（云台已配好，保持原样）。
- **不改** 两工程的 `CMakeLists.txt`、`CMakePresets.json`、`openocd.cfg`、`launch.json`、固件代码。
- **不改** 底盘的 `build` / `build and flash` 任务（只改 flash 任务一行 command）。
- **不做** 单根+参数化脚本方案（用户已选多根工作区）。
- **不做** 调试(launch.json)的自动分流改造（多根工作区下 cortex-debug 已按当前 folder 工作，无需动）。
- **不给** 两工程改 `CMAKE_PROJECT_NAME`（保持 `Gimbal_Demo`，产物名一致反而简化脚本；如未来需区分再单独提）。

---

## 未决 / 可选（不阻塞本次执行）

- **产物同名的潜在混淆**：两工程产物都叫 `Gimbal_Demo.elf`，只是位于各自 build 目录，功能上不冲突。若你希望产物名可区分（如 `Chassis_Demo`），需改底盘 CMakeLists 的 `CMAKE_PROJECT_NAME` 并同步改它的 flash.ps1 默认参数与 launch.json 的 executable 路径——这是独立的后续项，本次不做。
- **底盘的 `.ioc` 也叫 `Gimbal_Demo.ioc`**：同上，属命名遗留，不影响构建烧录。
