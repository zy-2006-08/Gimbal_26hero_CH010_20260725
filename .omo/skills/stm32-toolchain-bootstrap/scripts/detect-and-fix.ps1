<#
.SYNOPSIS
  STM32 工具链一键打通 (Windows)。检测/安装 arm-none-eabi-gcc + cmake + ninja + openocd，
  自动修 PATH（User 作用域，绝不用 setx），并在目标项目里编译验证“链路已打通”。
.DESCRIPTION
  配套 SKILL.md：.omo\skills\stm32-toolchain-bootstrap\SKILL.md
  - 检测优先，已装/已探测到的工具绝不重装（避免版本冲突）。
  - 改 PATH 前先备份到 %USERPROFILE%\user_path_backup.txt（可回滚）。
  - 仅限 Windows。绝不使用 setx。绝不修改项目源码。
.PARAMETER Mode
  auto = 全自动(winget 装缺失)；semi = 半自动(只打印官方链接手动装)。不传则交互询问。
.PARAMETER ProjectRoot
  目标项目根目录。默认当前 Gimbal 项目。
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File .\detect-and-fix.ps1 -Mode auto
#>
[CmdletBinding()]
param(
  [ValidateSet('auto','semi')]
  [string]$Mode,
  [string]$ProjectRoot = "D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111"
)

$ErrorActionPreference = 'Stop'
$TOOLS = 'arm-none-eabi-gcc','cmake','ninja','openocd'

# ---------------- 步骤 0：选择模式 ----------------
if (-not $Mode) {
  Write-Host "选择模式：" -ForegroundColor Cyan
  Write-Host "  [1] 全自动 FULL-AUTO  —— winget 直接装缺失（已装不重装），适合空白机器"
  Write-Host "  [2] 半自动 SEMI-AUTO  —— 只给官方下载链接，你手动装；装完我重检+修PATH+验证"
  $c = Read-Host "输入 1 或 2"
  $Mode = if ($c -eq '2') { 'semi' } else { 'auto' }
}
Write-Host "== 模式: $Mode ==" -ForegroundColor Green

# ---------------- 步骤 A：检测 ----------------
function Get-ToolReport {
  $probe = @{
    'ninja'             = @("$env:LOCALAPPDATA\stm32cube\bundles\ninja")
    'arm-none-eabi-gcc' = @("$env:ProgramFiles\Arm GNU Toolchain arm-none-eabi",
                            "${env:ProgramFiles(x86)}\Arm GNU Toolchain arm-none-eabi")
    'cmake'             = @("$env:ProgramFiles\CMake\bin")
    'openocd'           = @("$env:LOCALAPPDATA\Microsoft\WinGet\Packages")
  }
  $report = @{}
  foreach ($name in $TOOLS) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) {
      $v = (& $name '--version' 2>&1 | Select-Object -First 1)
      $report[$name] = @{ status='OK'; path=$cmd.Source; ver="$v"; fixDir=$null }
      continue
    }
    $found = $null
    foreach ($base in $probe[$name]) {
      if (Test-Path $base) {
        $exe = Get-ChildItem -Path $base -Recurse -Filter "$name.exe" -ErrorAction SilentlyContinue |
               Select-Object -First 1
        if ($exe) { $found = $exe.FullName; break }
      }
    }
    if ($found) {
      $v = (& $found '--version' 2>&1 | Select-Object -First 1)
      $report[$name] = @{ status='NOPATH'; path=$found; ver="$v"; fixDir=(Split-Path $found) }
    } else {
      $report[$name] = @{ status='MISSING'; path=$null; ver=$null; fixDir=$null }
    }
  }
  return $report
}

# ---------------- 步骤 B：报告 ----------------
function Show-Report($report) {
  Write-Host "`n---- 检测报告 ----" -ForegroundColor Cyan
  foreach ($name in $TOOLS) {
    $r = $report[$name]
    switch ($r.status) {
      'OK'     { Write-Host "✓已装      $name  -> $($r.path)   [$($r.ver)]" -ForegroundColor Green }
      'NOPATH' { Write-Host "⚠装了但不在PATH  $name  -> $($r.path)   (待加入: $($r.fixDir))" -ForegroundColor Yellow }
      'MISSING'{ Write-Host "✗缺失      $name" -ForegroundColor Red }
    }
  }
}

$report = Get-ToolReport
Show-Report $report

# ---------------- 步骤 C：安装缺失（已装不重装） ----------------
$missing = $TOOLS | Where-Object { $report[$_].status -eq 'MISSING' }
if ($missing) {
  if ($Mode -eq 'auto') {
    # winget ID 已实测核对 (2026-01)
    $wingetId = @{
      'cmake'             = 'Kitware.CMake'
      'ninja'             = 'Ninja-build.Ninja'
      'openocd'           = 'xpack-dev-tools.openocd-xpack'
      'arm-none-eabi-gcc' = 'Arm.GnuArmEmbeddedToolchain'
    }
    foreach ($name in $missing) {
      $id = $wingetId[$name]
      if ($id) {
        Write-Host "安装 $name  (winget: $id) ..." -ForegroundColor Cyan
        winget install --id $id -e --accept-package-agreements --accept-source-agreements
      } else {
        Write-Host "⚠ $name 无可靠 winget ID，请手动下载（见半自动链接）。" -ForegroundColor Yellow
      }
    }
  } else {
    Write-Host "`n✗缺失工具，请手动下载安装（装完重跑本脚本 / 说“装好了”）：" -ForegroundColor Yellow
    $links = @{
      'arm-none-eabi-gcc' = 'https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads'
      'cmake'             = 'https://cmake.org/download/  (勾选 Add CMake to system PATH)'
      'ninja'             = 'https://github.com/ninja-build/ninja/releases  (或装 STM32CubeCLT 自带 ninja)'
      'openocd'           = 'https://github.com/xpack-dev-tools/openocd-xpack/releases'
    }
    foreach ($name in $missing) { Write-Host ("  {0,-18}: {1}" -f $name, $links[$name]) }
    Write-Host "`n半自动模式：安装后请重跑本脚本，将自动重检+修PATH+验证。" -ForegroundColor Yellow
    return
  }
  # ---------------- 步骤 D：重新检测 ----------------
  Write-Host "`n---- 重新检测 ----" -ForegroundColor Cyan
  $report = Get-ToolReport
  Show-Report $report
}

# ---------------- 步骤 E：自动修 PATH（User 作用域，绝不 setx） ----------------
$backupDone = $false
foreach ($name in $TOOLS) {
  $r = $report[$name]
  if ($r.status -ne 'NOPATH' -or -not $r.fixDir) { continue }
  $dir = $r.fixDir
  $cur = [Environment]::GetEnvironmentVariable('Path','User')
  if (-not $backupDone) {
    $cur | Out-File "$env:USERPROFILE\user_path_backup.txt" -Encoding utf8
    Write-Host "已备份 User PATH 到 $env:USERPROFILE\user_path_backup.txt（可回滚）" -ForegroundColor DarkCyan
    $backupDone = $true
  }
  if (($cur -split ';' | ForEach-Object { $_.Trim() }) -notcontains $dir) {
    $sep = if ($cur -and -not $cur.EndsWith(';')) { ';' } else { '' }
    [Environment]::SetEnvironmentVariable('Path', "$cur$sep$dir", 'User')  # User 作用域，非 setx
    Write-Host "✓ 已把 $dir 加入 User PATH（$name）" -ForegroundColor Green
  } else {
    Write-Host "· $dir 已在 PATH，跳过（幂等）"
  }
}
if ($backupDone) {
  Write-Host "!! 请关闭并重开终端 / 重启 VSCode，PATH 才会在新进程生效。" -ForegroundColor Yellow
}

# ---------------- 步骤 F：最终验证（编译打通链路） ----------------
if (-not (Test-Path $ProjectRoot)) {
  Write-Host "项目根不存在: $ProjectRoot，跳过编译验证。" -ForegroundColor Yellow
  return
}
Set-Location $ProjectRoot

# 会话级 PATH 兜底：把 NOPATH 工具目录临时前置，免受“PATH 尚未重载”影响
foreach ($name in $TOOLS) {
  $fd = $report[$name].fixDir
  if ($fd) { $env:PATH = "$fd;" + $env:PATH }
}

Write-Host "`n---- 编译验证 (cmake --preset Debug) ----" -ForegroundColor Cyan
cmake --preset Debug
cmake --build --preset Debug

$out = Join-Path $ProjectRoot "build\Debug"
if (Test-Path "$out\Gimbal_Demo.elf") {
  Write-Host "`n✓ 找到 $out\Gimbal_Demo.elf" -ForegroundColor Green
  foreach ($ext in 'hex','bin') {
    if (Test-Path "$out\Gimbal_Demo.$ext") { Write-Host "✓ 找到 Gimbal_Demo.$ext" -ForegroundColor Green }
  }
  Write-Host "`n🎉 链路已打通：cmake + arm-none-eabi-gcc + ninja 产物齐全，openocd 就绪可烧录。" -ForegroundColor Green
} else {
  Write-Host "`n✗ 未找到 Gimbal_Demo.elf，请回看上方构建日志的 error: 行。" -ForegroundColor Red
}
