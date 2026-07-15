# toolchain-guide-and-skill - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 两份产物—�?1)工程根目录一份中文《新手配置指南�?教一个空 Windows 机的新手从下载工具、配 PATH、写 CMakeLists、到一键编�?烧录/编译烧录全走�?每步都讲"为什�?;(2)一�?skill,新手空电脑触发它就能自动检测四件套、装缺的、修 PATH、验�?一次打通链路�?
**Why this approach:** 你我这一路踩过的 16 个坑(.c 里的 C++、ninja 不在 PATH、误�?EIDE、VSCode Profile 快捷键坑、红波浪线、打开目录层级错等)全部写进文档�?避坑"小节,并配原因说明——这正是新手最容易卡住、而普通教程不会写的地方。skill 按你的要求做成闭�?检测→装→重检→自动修 PATH→验�?�?已装的不重装"避免和你现有工具冲突�?
**What it will NOT do:** 只做 Windows、只用中�?文档不写"别点 EIDE 编译按钮"、不�?串口打印不动"小节(这套流程没改 .c�?cpp,压根没碰串口打印);skill 不重装已有工具、不用会截断 PATH �?setx;本任务只生成文档�?skill,不改你现有工程的任何代码配置�?
**Effort:** Medium
**Risk:** Low - 只是产出两份新文�?不动现有工程;唯一要留意的是文档里的配置片段必须逐字抄自你已验证通过的真实文�?计划已要求执行时读文件比�?�?**Decisions to sanity-check:** 文档放工程根目录、中文、删�?EIDE 告诫和串口打印小�?skill 一个文件含"全自�?+ 半自�?两种模式(全自�?winget 装、半自动只引导下�?,都检测优�?已装不碰/PATH 用安�?PowerShell 方式;�?Windows;�?CubeMX 共存章节�?
Your next move: 认可后运�?`/start-work` 生成这两份产�?由执行环节实际写文件,我作为规划角色不直接�?。全部执行细节在下方�?
---

> TL;DR (machine): Medium effort, Low risk. Deliverables: (1) Chinese Windows 0�? tutorial at project root with all 16 real pitfalls + why + CubeMX section; (2) SKILL.md that detects/installs 4 tools, auto-fixes PATH, verifies chain on a blank Windows machine.

## Scope
### Must have
- **文档(产物1)**: 中文, 放工程根目录(�?`新手配置指南.md`)。从"下载工具"�?一键编�?烧录/编译烧录"全流�? 每一步配"为什么这么做"的解�? 内嵌可复制的参考配�?CMakePresets/工具链文�?openocd.cfg/tasks.json/keybindings), 把本项目实际踩过�?16 个坑都写�?避坑"小节, �?�?STM32CubeMX 共存"小节, 明确不加 printf 重定向�?- **skill(产物2)**: SKILL.md(�?AI 代理执行)。空 Windows 机触发即可打通链�? 检测四件套(arm-none-eabi-gcc/cmake/ninja/openocd) �?缺的�?winget �?已装的绝不重�?覆盖) �?用户装完后重新检�?�?PATH 不对用安�?PowerShell 方式(SetEnvironmentVariable User 作用�? �?setx)自动�?�?跑一�?configure+build 验证真的通了�?
### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不写 Linux/macOS 内容(�?Windows)�?- skill 不重�?覆盖已存在或已在 PATH 的工具�?- skill 不用 setx �?PATH(会截断到 1024 字符); 改前先备�?PATH�?- 本任务只"生产"文档+skill, **不改**用户的实际工程源�?CMakeLists/tasks/keybindings(那些上一个计划已完成)�?- 文档不教�?printf 重定�?串口打印保持 INFO/sprintf 原样)�?- 不做机器英译, 只用中文�?
## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none(文档+skill, 无单测框�?+ 代理执行的内容审查与 skill 逻辑自检。文档正确性的"真验�?= 内嵌的配置片段与工程里已验证通过的实际文件逐字一�? skill �?真验�?= 检�?�?PATH/验证逻辑可被无歧义执行且与本项目经验一致�?- Evidence: .omo/evidence/task-<N>-toolchain-guide-and-skill.txt

## Execution strategy
### Parallel execution waves
> 两份产物相互独立, 可完全并行。T1(文档)体量�? 可再�? T2(skill)独立。Wave 2 = 最终审查�?
- Wave 1: T1(文档), T2(skill) �?独立文件, 并行
- Wave 2: 最终审�?
### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| T1 文档 | - | - | T2 |
| T2 skill | - | - | T1 |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. 编写新手教程文档(中文, 工程根目�?
  What to do / Must NOT do: 在工程根目录创建 `新手配置指南.md`(中文)。必须先读取工程里这些已验证通过的真实文�? 把配置片�?*逐字**抄进文档作为参�?不要凭记忆写): CMakePresets.json, cmake/gcc-arm-none-eabi.cmake, CMakeLists.txt, openocd.cfg, .vscode/tasks.json, C:\Users\zy147\AppData\Roaming\Code\User\profiles\ff7ccb6\keybindings.json。文档结�?
    �?�?前言/适用范围(STM32F405RGT6, Windows, CMSIS-DAP, OpenOCD)�?    �?�?下载与安装四件套: arm-none-eabi-gcc(Arm GNU Toolchain 15.2.Rel1)、CMake(�?.22)、Ninja(重点: STM32CubeCLT 自带, 路径 C:\Users\<用户�?\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin)、OpenOCD(xPack 0.12.0)。给官方下载地址 + 装到�?+ 为什么�?    �?�?配置 PATH(PowerShell 用户级方�? 解释为何不用 setx; 如何验证 `xxx --version`)�?    �?�?CMakeLists.txt 配置: enable_language(C CXX ASM)、C++17、target_sources 加源文件、set_source_files_properties 强制 .c �?C++ �?+ 为什�?Keil 不分.c/.cpp 都当C++, GCC 按扩展名)�?fpermissive 为什么、include 路径、objcopy 生成 hex/bin 为什么�?    �?�?工具链文�?gcc-arm-none-eabi.cmake: MCU flags(cortex-m4/fpv4-sp-d16/hard)、g++ 编译+链接、nano.specs�?lstdc++/-lsupc++、Debug/Release 优化 flags 要同时给 C �?C++(解释那个 bug)�?    �?�?openocd.cfg(CMSIS-DAP+SWD+stm32f4x)+ 为什�?OpenOCD �?elf 不必�?hex/bin�?    �?�?一键任�?tasks.json: build / flash / build and flash(dependsOrder sequence)+ 为什么用 CMake 任务而不�?EIDE 按钮�?    �?�?快捷�? Alt+, / Alt+N / Alt+M, 讲清 VSCode Profile �?keybindings 要写进当前激�?profile 的文�? �?Open Keyboard Shortcuts (JSON)"而非 Default 只读那个)、GitLens 占用 Alt+, 要解绑、EIDE �?F7/Ctrl+Alt+D�?    �?�?编译/烧录操作: Ctrl+Shift+B �?Alt+, ; 如何看编译错�?只看 error 不看 warning, file:line:col)�?    �?�?�?STM32CubeMX 共存: 为什么改动放在主 CMakeLists 和工具链文件而不�?cmake/stm32cubemx/CMakeLists.txt(后者会�?CubeMX 重生成覆�?; main.c 不改名不改内�? 只在 CMake 层设 LANGUAGE�?    �?0�?常见问题/避坑合集: 覆盖以下真实踩过的坑(每个�?症状→原因→解决): .c含C++编不�?unknown type name 'class')�?fpermissive(int32_t*→int* 在C++是硬�?、ninja不在PATH(从STM32CubeCLT bundle加PATH)、VSCode Profile快捷键坑(keybindings写进当前激活profile)、Default键位表只�?要用"Open Keyboard Shortcuts (JSON)"非Default)、GitLens占Alt+,要解绑、文件夹vs工作区打开方式(读不同settings/切profile)、红波浪�?files.associations改cpp + compile_commands + Reset IntelliSense)�?没有找到要运行的任务"(打开目录层级�? 必须打开 ...20260111 这层)、如何看编译错误(只看error)�?  Must NOT(用户修订): **不写"别点 EIDE 编译按钮"这条指引, 整体淡化 EIDE**(EIDE 只在"红波浪线/历史遗留"角度顺带一提即�? 不作为操作告�?�?*不写"串口打印不动"小节**——本流程�?CMake LANGUAGE CXX 强制按C++编、并未把 .c 改成 .cpp, 串口打印(INFO)从未被触碰、该话题根本没出�? 不应写进文档。不�?Linux/macOS; 不教 printf 重定�? 不凭记忆编配�?必须抄真实文�?; 不改任何现有工程文件(只新建这一�?md)�?  Parallelization: Wave 1 | Blocked by: none | Blocks: none
  References (executor has NO interview context - be exhaustive): �?draft �?"Findings(16�?" �?"working config artifacts"。实际文�? CMakeLists.txt, cmake/gcc-arm-none-eabi.cmake, CMakePresets.json, openocd.cfg, .vscode/tasks.json, profiles/ff7ccb6/keybindings.json。芯�?STM32F405RGT6(1MB flash/192KB RAM)。ninja 实际路径 C:\Users\zy147\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin�?  Acceptance criteria (agent-executable): `新手配置指南.md` 存在于工程根目录; 含第1-10�? 内嵌�?CMakeLists/工具�?openocd.cfg/tasks.json 片段与真实文件逐字一�?�?diff 校验关键�?; 避坑关键词能 grep �?class/-fpermissive/ninja/profile/files.associations/compile_commands/CubeMX/没有找到 �?; **文档中不�?别点 EIDE 编译按钮"这类操作告诫, 不含"串口打印/printf 重定�?_write"相关小节**�?  QA scenarios: happy = 新手按文档从零能走�? failure = 抽查 3 个配置片段与真实文件比对、grep 避坑关键词是否齐全、grep 确认�?"printf 重定�?/"_write"/"别点 EIDE 编译" 字样。Evidence .omo/evidence/task-1-toolchain-guide-and-skill.txt
  Commit: N | docs: add Windows CMake+GCC+OpenOCD beginner setup guide

- [x] 2. 编写 SKILL.md(�?Windows 机一键打通链�?
  What to do / Must NOT do: 创建一�?skill(SKILL.md + 必要脚本), �?AI 代理执行, 目标: 新手�?Windows 机触发即打�?CMake+GCC+OpenOCD 链路。SKILL.md 需含标�?frontmatter(name, description 说明触发场景: "从零配置 STM32 CMake+GCC+OpenOCD 工具�?编译烧录环境")�?  **一�?skill 内含两种模式, 开头让 AI/用户先�?**
    - **全自�?FULL-AUTO)**: 检测出缺失的工具后, 直接�?winget �?已装/已在PATH的绝不重�?。适合空机、图省事�?    - **半自�?SEMI-AUTO)**: 只检测并给出官方下载链接, 由用户手动装; 用户装完 skill 重新检测、自动修 PATH、验证。适合已装了一半工具、想自己控制版本的机器�?  两种模式共用同一�?检测→(�?→重检→修PATH→验�?闭环, 区别仅在"�?这一步是 winget 自动还是引导手动�?  执行逻辑(闭环):
    步骤0 选模�? 询问/判断走全自动还是半自动�?    步骤A 检测四件套: 逐个 `Get-Command`/`--version` �?arm-none-eabi-gcc、cmake、ninja、openocd 是否�?PATH 及版本。同时探测常�?装了但不在PATH"的位�?尤其 ninja �?C:\Users\<用户>\AppData\Local\stm32cube\bundles\ninja\*\bin)�?    步骤B 报告缺失: 明确列出"已装�?缺失�?装了但不在PATH�?三类�?    步骤C 安装缺失(按模式分�?:
       · 全自�?�?winget 直接�?arm gnu toolchain、cmake、ninja、openocd �?winget id; winget 装不了的给官方链接兜�?, **已装/已在PATH的绝不重�?*�?       · 半自�?�?只输出每个缺失工具的官方下载地址 + 装到�?+ 装完提示, 不自己跑安装命令�?    步骤D 用户装完重新检�? 用户�?装好�?�?重跑步骤A(半自动必�? 全自动装完自动重检)�?    步骤E 自动�?PATH: �?装了但不在PATH"�?�?ninja bundle 目录), �?`[Environment]::SetEnvironmentVariable('Path', <cur>;<dir>, 'User')` 追加(先备份到文件, 幂等去重, 处理结尾分号), **禁止 setx**。提示需重启终端/VSCode 生效。两种模式都做�?    步骤F 最终验�? 在目标工程跑 `cmake --preset Debug` + `cmake --build --preset Debug`(临时�?ninja 目录 prepend 到本会话 PATH 以防未重�?, 确认�?error 且生�?elf(�?objcopy �?hex/bin)。报�?链路已打�?。两种模式都做�?  安全: �?PATH 属中等风险操�? skill 要说明每步在做什么、可回滚(备份文件路径)。检测优先、不覆盖已有工具(避免与用户已装版本冲�?�?  Must NOT: 不支持非 Windows; 不用 setx; 不重装已存在工具; 不改用户项目源码�?  Parallelization: Wave 1 | Blocked by: none | Blocks: none
  References: draft "Decisions" + "Findings" �?/8/9/12/14 �?ninja PATH、EIDE、profile、文件夹层级)。安�?PATH 方法见本项目历史(user_path_backup.txt 的做�?。winget �?Windows 10/11 自带�?  Acceptance criteria (agent-executable): skill 目录�?SKILL.md, frontmatter �?name/description 合法且触发词覆盖"配置工具�?编译烧录环境从零"; **含全自动 + 半自动两种模式且开头选模�?*; 文档化的执行步骤�?选模式→检测→(�?winget或引�?→重检→修PATH→验�?完整闭环; PATH 修改片段�?SetEnvironmentVariable(User) 且无 setx; 明确"已装不重�?�?  QA scenarios: happy = 干净机走全自动能 build 成功; 半自动模式模�?�?ninja"能给下载链接并在装后重检修PATH。failure = 审查 skill 是否会重装已有工具、是否用�?setx、缺少重检测环节、是否缺某一种模式。Evidence .omo/evidence/task-2-toolchain-guide-and-skill.txt
  Commit: N | feat(skill): add blank-Windows STM32 toolchain bootstrap skill

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit �?文档含第1-10章且 16 个避坑点齐全; skill 含检测→装→重检→修PATH→验证闭环�?- [x] F2. Accuracy review �?文档内嵌配置片段与真实文件逐字比对(抽查 CMakeLists force-C++ 行、工具链 flags、openocd.cfg、tasks.json、keybindings); skill �?winget id / ninja 路径 / PATH 方法准确�?- [x] F3. Real manual QA �?文档: 新手按第1-8章能否走�?逻辑连贯、无断链); skill: 模拟"�?ninja"场景, 确认它能检测出、给出装法、重检、修 PATH、验证�?- [x] F4. Scope fidelity �?仅中�? �?Windows; 未教 printf 重定�? **文档�?别点 EIDE 编译按钮"告诫、无"串口打印不动"小节**; skill **含全自动+半自动两种模�?*、不重装已有工具、不�?setx; 未改任何现有工程源码/配置; �?CubeMX 共存章节�?
## Commit strategy
- 可选单次提�?`docs+skill: STM32 toolchain onboarding`。除非用户要�? 否则不提交�?
## Success criteria
- 工程根目录有一份中文《新手配置指南�? 一个新手从�?Windows 机照着能走�?下载→PATH→CMakeLists→工具链→openocd→一键任务→快捷键→编译烧录", 且遇到本项目 16 个坑时文档里都能查到症状/原因/解决�?- 有一�?skill, 新手�?Windows 机触发它即可闭环打通链�?检测→winget装缺失→重检→自动修PATH→configure+build验证), 且不破坏已装工具、不�?setx�?- 两份产物均为中文、仅 Windows、不�?printf 重定向、含 CubeMX 共存说明�?