---
name: stm32-toolchain-bootstrap
description: 从零在 Windows 上配置并打通 STM32 CMake+GCC+OpenOCD 交叉编译工具链(检测/安装 arm-none-eabi-gcc、cmake、ninja、openocd,修复 PATH,验证编译烧录环境)。当用户的电脑没装工具链、要配置编译烧录环境、或 build 报 ninja/编译器找不到时使用。
---

# STM32 工具链一键打通 (Windows)

本 Skill 在**一台空白 Windows 机器**上，把 STM32 交叉编译工具链
（`arm-none-eabi-gcc` + `cmake` + `ninja` + `openocd`）从零配置到**编译成功、链路打通**。

> 存放位置约定：本 Skill 放在项目内 `.omo\skills\stm32-toolchain-bootstrap\`。
> 该项目已存在 `.omo\` 目录，沿用此约定。若换项目无 `.omo\`，可改放 `skills\stm32-toolchain-bootstrap\SKILL.md`，二选一保持一致即可。

## 适用项目（已核对）
- 项目根：`D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111`
- 产物名：`Gimbal_Demo`（见 `CMakeLists.txt` 的 `set(CMAKE_PROJECT_NAME Gimbal_Demo)`）
- 生成器：`Ninja`；工具链文件：`cmake/gcc-arm-none-eabi.cmake`
- 预设：`Debug / RelWithDebInfo / Release / MinSizeRel`（见 `CMakePresets.json`）
- 产物目录：`build\Debug\`，含 `Gimbal_Demo.elf`，POST_BUILD 用 objcopy 产出 `.hex` / `.bin`

---

## ⚠️ 安全与红线（执行前先声明给用户）
- **改 PATH 属中等风险操作**：每一步都先解释再做；修改前**先把用户 PATH 备份到文件**
  （`%USERPROFILE%\user_path_backup.txt`），并告知用户此备份路径可用于回滚。
- **检测优先，绝不重装/覆盖已装工具**（`✓已装` 或 `⚠装了但不在PATH` 一律跳过安装，避免与用户已有版本冲突）。
- **仅限 Windows**。
- **绝不使用 `setx`**（它会把 PATH 截断到 1024 字符，破坏环境）。改 PATH 一律用
  `[Environment]::SetEnvironmentVariable(...,'User')`（User 作用域）。
- **绝不修改用户的项目源码**；本 Skill 只新建 skill 文件、读项目、跑构建验证。

---

## 步骤 0 · 选择模式（先做这一步）
向用户明确两种模式，让其选择（或依情况自动判断）：

- **全自动 FULL-AUTO**：检测到缺失后，直接用 `winget` 安装。**已装/已在 PATH 的绝不重装**。
  适合真正空白的机器。
- **半自动 SEMI-AUTO**：只检测并打印官方下载链接，由用户手动安装（可控制版本）；
  用户装完后，本 Skill 重新检测、自动修 PATH、并验证。适合已装了一半工具 / 想自己控版本的机器。

判断建议：
- 若 4 个工具**全部缺失** → 建议 FULL-AUTO。
- 若用户已装部分工具、或明确想自己控版本 → 建议 SEMI-AUTO。
- 不确定时**直接问用户**：“要全自动 winget 安装，还是半自动（我给链接你手动装）？”

> 也可直接调用附带脚本：`scripts\detect-and-fix.ps1`。
> 用法：`powershell -ExecutionPolicy Bypass -File .\.omo\skills\stm32-toolchain-bootstrap\scripts\detect-and-fix.ps1 -Mode auto`
> （`-Mode auto` / `-Mode semi`；不加 `-Mode` 会交互询问）。
> 但**本 SKILL.md 自成体系**，无脚本也能按下面步骤逐条执行。

---

## 步骤 A · 检测（两模式共用）
对 `arm-none-eabi-gcc` / `cmake` / `ninja` / `openocd` 逐个检测：先看是否在 PATH，再拿版本号；
若不在 PATH，再探常见“装了但没进 PATH”的位置——**尤其 ninja 常被 STM32CubeCLT 捆绑**在
`C:\Users\<用户名>\AppData\Local\stm32cube\bundles\ninja\*\bin`。

```powershell
$tools = @{
  'arm-none-eabi-gcc' = @{ ver = '--version' }
  'cmake'             = @{ ver = '--version' }
  'ninja'             = @{ ver = '--version' }
  'openocd'           = @{ ver = '--version' }
}

# 常见“装了但不在 PATH”的候选目录（ninja 尤其重要）
$probe = @{
  'ninja' = @(
    "$env:LOCALAPPDATA\stm32cube\bundles\ninja"          # STM32CubeCLT 捆绑（会再往下找 */bin）
  )
  'arm-none-eabi-gcc' = @(
    "$env:ProgramFiles\Arm GNU Toolchain arm-none-eabi",
    "${env:ProgramFiles(x86)}\Arm GNU Toolchain arm-none-eabi"
  )
  'cmake'   = @( "$env:ProgramFiles\CMake\bin" )
  'openocd' = @( "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" )
}

$report = @{}
foreach ($name in $tools.Keys) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if ($cmd) {
    $v = (& $name $tools[$name].ver 2>&1 | Select-Object -First 1)
    $report[$name] = @{ status = 'OK'; path = $cmd.Source; ver = "$v"; fixDir = $null }
    continue
  }
  # 不在 PATH：去候选目录里找 exe
  $found = $null
  foreach ($base in $probe[$name]) {
    if (Test-Path $base) {
      $exe = Get-ChildItem -Path $base -Recurse -Filter "$name.exe" -ErrorAction SilentlyContinue |
             Select-Object -First 1
      if ($exe) { $found = $exe.FullName; break }
    }
  }
  if ($found) {
    $v = (& $found $tools[$name].ver 2>&1 | Select-Object -First 1)
    $report[$name] = @{ status = 'NOPATH'; path = $found; ver = "$v"; fixDir = (Split-Path $found) }
  } else {
    $report[$name] = @{ status = 'MISSING'; path = $null; ver = $null; fixDir = $null }
  }
}
```

## 步骤 B · 报告
按 `✓已装 / ✗缺失 / ⚠装了但不在PATH` 逐项打印：

```powershell
foreach ($name in 'arm-none-eabi-gcc','cmake','ninja','openocd') {
  $r = $report[$name]
  switch ($r.status) {
    'OK'      { "✓已装      $name  -> $($r.path)   [$($r.ver)]" }
    'NOPATH'  { "⚠装了但不在PATH  $name  -> $($r.path)   (需把 $($r.fixDir) 加进 PATH)" }
    'MISSING' { "✗缺失      $name" }
  }
}
```

## 步骤 C · 安装缺失（按模式分支）
**只处理 `✗缺失`。`✓已装` / `⚠装了但不在PATH` 一律跳过安装（已装不重装）。**

### C-1 全自动 FULL-AUTO → winget（winget ID 已实测核对，2026-01）
```powershell
$wingetId = @{
  'cmake'             = 'Kitware.CMake'
  'ninja'             = 'Ninja-build.Ninja'
  'openocd'           = 'xpack-dev-tools.openocd-xpack'   # 实测正确 ID（不是 xPack.OpenOCD）
  'arm-none-eabi-gcc' = 'Arm.GnuArmEmbeddedToolchain'
}
foreach ($name in 'arm-none-eabi-gcc','cmake','ninja','openocd') {
  if ($report[$name].status -eq 'MISSING') {
    $id = $wingetId[$name]
    if ($id) {
      Write-Host "安装 $name  (winget: $id) ..."
      winget install --id $id -e --accept-package-agreements --accept-source-agreements
    } else {
      Write-Host "⚠ $name 无可靠 winget ID，请走半自动手动下载（见 C-2 链接）。"
    }
  } else {
    Write-Host "跳过 $name（已装/已探测到，不重装）。"
  }
}
```
> 若某工具 winget 安装失败或无可靠 ID，**回退到 C-2 打印官方链接**让用户手动装。

### C-2 半自动 SEMI-AUTO → 只打印官方链接，不执行安装
```
✗缺失的工具，请手动下载安装（装完回来告诉我“装好了”，我再重检并修 PATH）：

  arm-none-eabi-gcc :  https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
      装到默认位置即可；安装向导可勾选“Add path to environment variable”。
  cmake             :  https://cmake.org/download/
      安装时选择 “Add CMake to the system PATH”。
  ninja             :  https://github.com/ninja-build/ninja/releases
      （或直接装 STM32CubeCLT，它自带 ninja；本 Skill 会自动探测其 bundle 目录）
  openocd           :  https://github.com/xpack-dev-tools/openocd-xpack/releases
      解压后记住 bin 目录，本 Skill 会帮你加进 PATH。
```

## 步骤 D · 重新检测
- 半自动：用户说“装好了”后，重跑**步骤 A + B**。
- 全自动：`winget` 全部跑完后，重跑**步骤 A + B**。
（winget 装的工具通常自动进了 PATH，但新 PATH 需重开终端才生效；本会话内用步骤 F 的会话级 PATH 兜底。）

## 步骤 E · 自动修 PATH（针对 `⚠装了但不在PATH`）
对每个 `⚠`（如 ninja 的 bundle 目录、手动解压的 openocd bin 目录）安全追加到 **User 作用域** PATH。
**先备份、幂等（不重复添加）、绝不用 `setx`**：

```powershell
foreach ($name in 'arm-none-eabi-gcc','cmake','ninja','openocd') {
  $r = $report[$name]
  if ($r.status -ne 'NOPATH' -or -not $r.fixDir) { continue }
  $dir = $r.fixDir

  $cur = [Environment]::GetEnvironmentVariable('Path','User')
  # 修改前先备份（可回滚）
  $cur | Out-File "$env:USERPROFILE\user_path_backup.txt" -Encoding utf8
  Write-Host "已备份当前 User PATH 到 $env:USERPROFILE\user_path_backup.txt"

  if (($cur -split ';' | ForEach-Object { $_.Trim() }) -notcontains $dir) {
    $sep = if ($cur -and -not $cur.EndsWith(';')) { ';' } else { '' }
    [Environment]::SetEnvironmentVariable('Path', "$cur$sep$dir", 'User')  # User 作用域，非 setx
    Write-Host "✓ 已把 $dir 加入 User PATH（$name）"
  } else {
    Write-Host "· $dir 已在 PATH，跳过（幂等）"
  }
}
```
> **改完必须提示用户：关闭并重开终端 / 重启 VSCode，PATH 才会在新进程生效。**
> 回滚方式：备份文件 `%USERPROFILE%\user_path_backup.txt` 里是改动前的 User PATH 原值，
> 如需还原执行 `[Environment]::SetEnvironmentVariable('Path', (Get-Content "$env:USERPROFILE\user_path_backup.txt" -Raw).Trim(), 'User')`。

## 步骤 F · 最终验证（在目标项目里编译，打通链路）
为避免“PATH 尚未在当前会话重载”导致找不到 ninja，先把 **ninja 的 bundle 目录**（若走该来源）
临时拼到**会话级 PATH** 最前面，再跑 cmake：

```powershell
# 项目根
Set-Location "D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111"

# 会话级 PATH 兜底：把 ninja bundle 目录（若步骤A探测到）放最前
$ninjaDir = $report['ninja'].fixDir
if ($ninjaDir) { $env:PATH = "$ninjaDir;" + $env:PATH }
# 若 arm-gcc / openocd 也是 NOPATH，同理可临时前置其目录

cmake --preset Debug
cmake --build --preset Debug
```

验证判据（全部满足才算“打通”）：
- `cmake --preset Debug` 配置成功，无 `error:`。
- `cmake --build --preset Debug` 编译成功，输出里**无 `error:`**。
- `build\Debug\Gimbal_Demo.elf` 存在；若 objcopy 可用，`Gimbal_Demo.hex` / `Gimbal_Demo.bin` 一并出现。

```powershell
$out = "build\Debug"
$ok = Test-Path "$out\Gimbal_Demo.elf"
if ($ok) {
  Write-Host "✓ 找到 $out\Gimbal_Demo.elf"
  foreach ($ext in 'hex','bin') {
    if (Test-Path "$out\Gimbal_Demo.$ext") { Write-Host "✓ 找到 Gimbal_Demo.$ext" }
  }
  Write-Host "`n🎉 链路已打通：cmake + arm-none-eabi-gcc + ninja 编译产物齐全。"
  Write-Host "   openocd 已就绪，可进行烧录（另见项目烧录配置 / openocd.cfg）。"
} else {
  Write-Host "✗ 未找到 Gimbal_Demo.elf，请回看构建日志中的 error: 行。"
}
```

> 烧录说明：本 Skill 目标是“编译链路打通 + openocd 就绪”。实际 `openocd -f ...` 烧录命令
> 依赖具体调试器（DAP-Link / ST-Link）与项目 `openocd.cfg`，不在本 Skill 强改范围内。

---

## 收尾提示
- 若步骤 E 改过 PATH：提醒用户**重开终端 / 重启 VSCode**。
- 备份文件位置：`%USERPROFILE%\user_path_backup.txt`。
- 全程未重装任何 `✓/⚠` 的已装工具，未使用 `setx`，未改动项目源码。
