# clangd 多根工作区生效（改用 .clangd 文件方案）

## TL;DR (For humans)

clangd 在多根工作区里默认单实例、`--compile-commands-dir=${workspaceFolder}/...` 只对第一个根/工作区文件目录生效，无法按每个工程分流（clangd/vscode-clangd issue #438、#696、#923 确认的已知限制）。单根打开正常正是因为那时 `${workspaceFolder}` 明确。

**正确解法（官方推荐）**：每个工程根放一个 `.clangd` 文件，用 `CompilationDatabase: build/Debug`（相对该文件目录）指定编译数据库。clangd 打开某工程文件时自动读该工程的 `.clangd`，单根/多根都正确。同时从两个 settings.json 移除 `clangd.arguments`（多根下会干扰）。

**改动**：新建 2 个 `.clangd` 文件；改 2 个 settings.json（删 clangd.arguments）。

## Findings
- clangd 扩展多根下单实例、只在第一个根应用 `--compile-commands-dir`；不支持 `${workspaceFolder:name}`（#696/#923）。
- 官方推荐用工程根 `.clangd` 的 `CompilationDatabase` 指向编译库目录（相对路径），按目录树自动匹配，多根可共存。
- 两工程 compile_commands.json 均在各自 `build/Debug/`。

## Todos

### 1. [x] 云台工程根新建 .clangd
**WHERE**: `D:\Newcode\Gimbal_24hero_CH010_20260111\.clangd`（工程根，非 .vscode）
**HOW**: 新建，内容：
```yaml
CompileFlags:
  CompilationDatabase: build/Debug
```
**EXPECT**: clangd 打开云台文件时读 build/Debug/compile_commands.json。
**QA**: 读回确认内容正确（YAML 两行）。

### 2. [x] 底盘工程根新建 .clangd
**WHERE**: `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.clangd`（工程根）
**HOW**: 新建，内容同上：
```yaml
CompileFlags:
  CompilationDatabase: build/Debug
```
**EXPECT**: clangd 打开底盘文件时读底盘的 build/Debug/compile_commands.json。
**QA**: 读回确认。

### 3. [x] 云台 settings.json 移除 clangd.arguments
**WHERE**: `D:\Newcode\Gimbal_24hero_CH010_20260111\.vscode\settings.json`
**HOW**: 整个文件替换为：
```json
{
    "C_Cpp.errorSquiggles": "disabled",
    "files.associations": {
        "rm_lib.h": "c"
    }
}
```
**EXPECT**: 不再有 clangd.arguments 干扰，配置全交给 .clangd。
**QA**: 读回确认 JSON 有效、无 clangd.arguments。

### 4. [x] 底盘 settings.json 移除 clangd.arguments
**WHERE**: `D:\Newcode\Chassisl_26_SHANGTAIJIE_SEND_DM\Chassisl_26_SHANGTAIJIE\.vscode\settings.json`
**HOW**: 整个文件替换为：
```json
{
    "C_Cpp.errorSquiggles": "disabled"
}
```
**EXPECT**: 同上。
**QA**: 读回确认。

## 用户验证
1. 重启 clangd（命令面板 `clangd: Restart language server`）或重开 VSCode。
2. 打开 gimbal-chassis.code-workspace，分别打开云台文件和底盘文件。
3. 两边波浪线都应消失、补全都应工作（clangd 按各自 .clangd 读对应 compile_commands.json）。
4. 单独打开某个工程文件夹时也应正常（.clangd 同样生效）。

## Must-NOT-Have
- 不改 CMakeLists、tasks.json、launch.json、固件代码。
- 不删云台的 files.associations。
- .clangd 放工程根，不是 .vscode 里。
- CompilationDatabase 用相对路径 build/Debug（相对 .clangd 所在目录）。
