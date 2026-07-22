<#
.SYNOPSIS
    RM 战队嵌入式开发环境 一键配置脚本（给新电脑/队友用）
.DESCRIPTION
    自动装 Scoop + 工具链(gcc/cmake/ninja/openocd) + VSCode 扩展，自动配 PATH 并验证。
    需要手动装的（J-Link 驱动、Ozone）会在最后提示。
    用法：右键"用 PowerShell 运行"，或在 PowerShell 里执行：
        powershell -ExecutionPolicy Bypass -File scripts\setup.ps1
#>

$ErrorActionPreference = "Stop"
Write-Host "==== RM 战队开发环境一键配置 ====" -ForegroundColor Cyan

# ---------- 0. 前置检查 ----------
Write-Host "`n[0/5] 检查前置..." -ForegroundColor Yellow
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "  ! 未检测到 Git，Scoop 需要它。请先装 Git: https://git-scm.com/download/win" -ForegroundColor Red
    Write-Host "    装完 Git 重开 PowerShell 再跑本脚本。" -ForegroundColor Red
    exit 1
}
Write-Host "  Git OK"

# ---------- 1. 装 Scoop ----------
Write-Host "`n[1/5] 检查/安装 Scoop 包管理器..." -ForegroundColor Yellow
if (-not (Get-Command scoop -ErrorAction SilentlyContinue)) {
    Write-Host "  未装 Scoop，正在安装..."
    Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser -Force
    Invoke-RestMethod -Uri https://get.scoop.sh | Invoke-Expression
} else {
    Write-Host "  Scoop 已装"
}

# ---------- 2. 装工具链 ----------
Write-Host "`n[2/5] 安装工具链 (gcc/cmake/ninja/openocd)..." -ForegroundColor Yellow
scoop install gcc-arm-none-eabi cmake ninja openocd
# 说明：Scoop 装的工具自动加入 PATH（scoop shims），无需手动配

# ---------- 3. 装 VSCode 扩展 ----------
Write-Host "`n[3/5] 安装 VSCode 扩展 (clangd, cortex-debug)..." -ForegroundColor Yellow
if (Get-Command code -ErrorAction SilentlyContinue) {
    code --install-extension llvm-vs-code-extensions.vscode-clangd
    code --install-extension marus25.cortex-debug
    Write-Host "  提示：clangd 首次启用会自动下载语言服务器本体，同意即可；并在弹窗点 Disable IntelliSense。"
} else {
    Write-Host "  ! 未检测到 code 命令。请先装 VSCode 并在安装时勾 'Add to PATH'，或手动装扩展 clangd / cortex-debug。" -ForegroundColor Red
}

# ---------- 4. 验证 ----------
Write-Host "`n[4/5] 验证工具版本..." -ForegroundColor Yellow
$ok = $true
foreach ($t in @('arm-none-eabi-gcc', 'cmake', 'ninja', 'openocd')) {
    $c = Get-Command $t -ErrorAction SilentlyContinue
    if ($c) { Write-Host "  [OK] $t -> $($c.Source)" -ForegroundColor Green }
    else { Write-Host "  [缺] $t 未找到（可能需要重开终端刷新 PATH）" -ForegroundColor Red; $ok = $false }
}

# ---------- 5. 手动项提示 ----------
Write-Host "`n[5/5] 需要你手动做的（脚本没法全自动）：" -ForegroundColor Yellow
Write-Host "  - 用 J-Link 的队友：装 SEGGER J-Link 驱动 https://www.segger.com/downloads/jlink/"
Write-Host "    （用 DAP-Link 的可跳过，DAP 免驱）"
Write-Host "  - 需要 Ozone 图形调试的：SEGGER 官网下载 Ozone（可选，后面调试才用）"
Write-Host "  - 首次编译：VSCode 打开工程文件夹，按 Alt+, 编译一次（生成 compile_commands.json 给 clangd）"

Write-Host "`n==== 配置结束 ====" -ForegroundColor Cyan
if ($ok) {
    Write-Host "工具链验证通过。若上面有 [缺]，关掉所有终端和 VSCode 重开一次再验证（PATH 需要新进程生效）。" -ForegroundColor Green
} else {
    Write-Host "有工具没验证过 —— 多半是 PATH 还没刷新。关掉所有终端/VSCode 重开，再跑一次本脚本。" -ForegroundColor Red
}
