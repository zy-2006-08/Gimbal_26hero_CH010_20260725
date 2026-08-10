# 配置 clangd 指向 compile_commands.json（消除波浪线）

## TL;DR (For humans)

clangd 满屏红波浪线的原因：`compile_commands.json` 在 `build/Debug/` 子目录，但 clangd 默认只在工程根找，没找到 → 无编译信息 → 头文件和类型全报红。两个工程都已编译、该文件都存在。修复=给两个工程的 `.vscode/settings.json` 加 `clangd.arguments` 指向 `build/Debug`。

**改动**：2 个文件（两个工程各自的 settings.json）。

## Findings
- 云台 `D:\Newcode\Gimbal_24hero_CH010_20260111\build\Debug\compile_commands.json` 存在。
- 底盘 `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\build\Debug\compile_commands.json` 存在。
- 两工程 settings.json 均未配 clangd.arguments。
- 云台 settings 现有：`C_Cpp.errorSquiggles: disabled` + `files.associations: {rm_lib.h: c}`。
- 底盘 settings 现有：`C_Cpp.errorSquiggles: disabled`。

## Todos

### 1. [x] 云台 settings.json 加 clangd.arguments
**WHERE**: `D:\Newcode\Gimbal_24hero_CH010_20260111\.vscode\settings.json`
**HOW**: 整个文件替换为：
```json
{
    "C_Cpp.errorSquiggles": "disabled",
    "files.associations": {
        "rm_lib.h": "c"
    },
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}/build/Debug",
        "--header-insertion=never"
    ]
}
```
**EXPECT**: clangd 读到 build/Debug/compile_commands.json，重启 clangd 后波浪线消失、补全恢复。
**QA**: 读回确认 JSON 有效、含 clangd.arguments 两项。

### 2. [x] 底盘 settings.json 加 clangd.arguments
**WHERE**: `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\settings.json`
**HOW**: 整个文件替换为：
```json
{
    "C_Cpp.errorSquiggles": "disabled",
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}/build/Debug",
        "--header-insertion=never"
    ]
}
```
**EXPECT**: 同上。
**QA**: 读回确认 JSON 有效。

## 用户验证（配置后）
1. VSCode 命令面板运行 `clangd: Restart language server`（或重开 VSCode）。
2. 打开 my_main.cpp 等文件，等 clangd 重新索引。
3. 波浪线应大量消失；输入 `结构体名.` 应弹出成员补全。

## Must-NOT-Have
- 不改 CMakeLists、tasks.json、launch.json、固件代码。
- 不删 files.associations（云台保留）。
- ${workspaceFolder} 在单根/多根工作区下都解析为当前工程根，两种打开方式通用。
