# refine-guide-and-errorjump - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 更清爽的《新手配置指南》——第1章瘦成"下载啥+网址"的清单,全文删掉新手看不懂的啰嗦解释只留操作,并新增一节 J-Link 怎么用;同时给编译任务加个小配置,让报错能进"问题"面板、终端路径可以直接点击跳转、还有红黄颜色。

**Why this approach:** 你反馈文档太冗余、报错不好定位。报错跳转不是玄学——你 tasks.json 里 build 的 problemMatcher 是空的,VSCode 就不解析编译器输出;加上内置的 `$gcc` 解析器,报错立刻变成可点击、有颜色。J-Link 走你现有的 OpenOCD 路线只改一行 interface,最省事。

**What it will NOT do:** 只动两个文件(指南 + tasks.json),不碰工程源码/CMake/工具链/快捷键/openocd.cfg;仍不写 printf 重定向、不写 EIDE 告诫;只用中文、只 Windows;J-Link 只给最小改法不写成大教程。

**Effort:** Short
**Risk:** Low - 文档精简 + 一处 JSON 单行改,无代码风险。
**Decisions to sanity-check:** 第1章只留下载清单+网址;去冗余(看不懂的解释删、直接给操作);build 加 $gcc 让报错可点击;J-Link 走 OpenOCD 改 interface/jlink.cfg。

Your next move: 认可后运行 `/start-work` 执行(由执行环节改这两个文件,我作为规划角色不直接改)。全部细节在下方。

---

> TL;DR (machine): Quick-Short effort, Low risk. 精简《新手配置指南》第1章+全文去冗余、加 J-Link 节; 给 tasks.json build 加 $gcc 让报错可点击跳转+颜色。

## Scope
### Must have
- 新手配置指南.md 第1章精简为"下载清单+官方网址"(每工具 1-2 行, 顶多一句它干嘛), 删掉冗长的 what/why/装到哪。
- 全文去冗余: 删掉新手看不懂或非必要的原理长篇解释, 直接保留操作与命令; 仅保留真正避坑必需的极简"为什么"(如 setx 危险一句话、LANGUAGE CXX 为何需要一句话)。
- tasks.json 的 build 任务 `"problemMatcher": []` → `"problemMatcher": ["$gcc"]`, 使编译错误进"问题"面板、终端 file:line:col 可点击跳转、有红黄色区分。文档第6/8章补一句说明。
- 文档新增 J-Link 小节: 走 OpenOCD 路线, 把 openocd.cfg 第1行 interface 换成 `interface/jlink.cfg` 的最小改法; 附一句"也可用 SEGGER 原生 JLink 工具"但不展开。

### Must NOT have (guardrails, anti-slop, scope boundaries)
- 只动两个文件: `新手配置指南.md` 和 `.vscode/tasks.json`。不改工程源码/CMakeLists/工具链/keybindings/openocd.cfg 本身。
- flash 任务的 problemMatcher 不加 $gcc(openocd 输出非编译错误)。
- 不删文档第2-10章的结构, 只精简各章内部。
- 仍不写 printf 重定向、不写"别点 EIDE 编译按钮"告诫。
- 仅中文、仅 Windows。
- J-Link 只给 OpenOCD 最小改法, 不写成 SEGGER 工具大教程。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none(文档+一处 tasks 配置)+ 代理执行的 JSON 校验和内容审查。tasks.json 的"真验证"= 有效 JSON 且 build 的 problemMatcher 为 ["$gcc"]; 文档"真验证"= 第1章行数明显缩短、含 J-Link 节、仍无 printf/EIDE 告诫。
- Evidence: .omo/evidence/task-<N>-refine-guide-and-errorjump.txt

## Execution strategy
### Parallel execution waves
> T2(tasks.json 单行改)独立、极小; T1(文档精简+加 J-Link 节)独立。两者并行。Wave 2 = 审查。

- Wave 1: T1(文档), T2(tasks.json) — 并行
- Wave 2: 最终审查

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| T1 文档精简+J-Link | - | - | T2 |
| T2 tasks.json $gcc | - | - | T1 |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. 精简《新手配置指南.md》+ 新增 J-Link 小节 + 补报错可点击说明
  What to do / Must NOT do: 编辑 `D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111\新手配置指南.md`。
    (a) **第1章精简**: 现状每个工具有"它是什么/版本/官方下载/装到哪/为什么"五段(L24-69), 改成一份紧凑下载清单——每个工具 1-2 行: 名字 + 一句它干嘛(≤15字) + 官方网址。保留这四个网址:
        arm-none-eabi-gcc: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
        CMake: https://cmake.org/download/
        Ninja: https://github.com/ninja-build/ninja/releases (或装 STM32CubeCLT 自带)
        OpenOCD: https://github.com/xpack-dev-tools/openocd-xpack/releases
        并保留"ninja 可能在 STM32CubeCLT 的 bundle 目录 C:\Users\<用户名>\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin"这一句(避坑必需)。
    (b) **全文激进精简(用户定: 力度=激进)**: 通读第2-10章, 目标是把文档改成"速查手册"风格——**只留"做什么 + 命令 + 一句话结果", 原理解释几乎全删**。明确删掉: fpv4-sp-d16 寄存器解释、newlib-nano 学术描述、g++ 当链接器的原理展开、-lstdc++/-lsupc++ 的运行时长篇、每个 flag 的逐字解读等。**仅保留极少数避坑必需的一句话**"为什么": ① setx 会截断 PATH 危险 ② .c 含 class 要 LANGUAGE CXX ③ 优化 flag 要同时给 C 和 C++ 否则 C++ 没优化/调试。其余一律删成动作步骤。代码块/命令/配置片段保留(它们是操作), 但配套的大段讲解砍掉。语气口语、步骤化。
    (c) **新增 J-Link 小节**(放第5章 openocd.cfg 之后, 或作为 5.x): 说明若用 J-Link 而非 CMSIS-DAP, 把 openocd.cfg 第一行 `source [find interface/cmsis-dap.cfg]` 换成 `source [find interface/jlink.cfg]`, 其余不变, tasks/快捷键不用动; 附一句"也可用 SEGGER 自带的 JLink/JFlash 工具烧, 但那是另一套, 本文不展开"。
    (d) **补报错可点击说明**: 因为 T2 会给 build 加 $gcc, 在第6章(tasks)或第8章(看错误)补一句: 编译报错会进 VSCode"问题"面板(Ctrl+Shift+M), 终端里的 `文件:行:列` 可直接 Ctrl+点击 跳转到出错行, 且有红黄颜色。
  Must NOT: 不删第2-10章整章结构(只精简内部); 不写 printf 重定向/串口打印; 不写"别点 EIDE 编译按钮"告诫; 不改工程其它文件; 保持中文。
  Parallelization: Wave 1 | Blocked by: none | Blocks: none
  References: 新手配置指南.md(现 778 行), 第1章 L24-69。openocd.cfg 现内容(cmsis-dap 那 4 行)。J-Link 走 OpenOCD 只改 interface 行。$gcc 说明配合 T2。
  Acceptance criteria (agent-executable): 第1章行数明显缩短(四个工具变成紧凑清单, 四个官方网址仍在, ninja bundle 路径仍在); 文档含 "jlink" 或 "J-Link" 小节且包含 `interface/jlink.cfg`; 含"问题"面板/可点击/跳转 的说明; grep 仍为 0 的: "printf 重定向"/"_write"/"别点.*编译按钮"; 总行数应比 778 明显下降。
  QA scenarios: happy = 新手读第1章一眼知道下载啥去哪下; failure = grep 四个网址是否都在、grep jlink.cfg、grep 禁写内容为0、对比行数下降。Evidence .omo/evidence/task-1-refine-guide-and-errorjump.txt
  Commit: N | docs: 精简新手指南第1章与全文, 新增 J-Link 说明与报错跳转提示

- [x] 2. tasks.json: build 加 $gcc problemMatcher(报错可点击跳转+颜色)
  What to do / Must NOT do: 编辑 `.vscode/tasks.json`。把 build 任务(label "build")的 `"problemMatcher": []`(第12行)改成 `"problemMatcher": ["$gcc"]`。flash 任务的 problemMatcher 保持 `[]`(openocd 输出不是编译错误)。build and flash 任务(dependsOn, 无 command)的 problemMatcher 保持原样或删除均可, 不加 $gcc。保持文件为有效 JSON, 其余内容(command/label/group/dependsOn)一字不改。
  Parallelization: Wave 1 | Blocked by: none | Blocks: none
  References: .vscode/tasks.json:12(build 的空 problemMatcher)。$gcc 是 VSCode 内置 GCC 输出解析器, 让 arm-none-eabi-gcc/g++ 的 `文件:行:列: error:` 进问题面板并可点击。
  Acceptance criteria (agent-executable): tasks.json 是有效 JSON; build 任务 problemMatcher == ["$gcc"]; flash 任务 problemMatcher 仍为 []; command 行未变。
  QA scenarios: happy = ConvertFrom-Json 解析成功且 build.problemMatcher 为 $gcc; failure = JSON lint + grep 确认 flash 未被误加 $gcc。Evidence .omo/evidence/task-2-refine-guide-and-errorjump.txt
  Commit: N | build(vscode): build 任务加 $gcc problemMatcher 以支持报错跳转

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance — 第1章已精简为下载清单+网址; 全文去冗余; 含 J-Link 节; tasks.json build 加了 $gcc。
- [x] F2. Accuracy — 四个官方网址完整; ninja bundle 路径保留; jlink.cfg 改法正确; $gcc 拼写正确、flash 未误加。
- [x] F3. Real QA — tasks.json 有效 JSON 且能被解析; 文档行数较 778 下降; 报错跳转说明存在。
- [x] F4. Scope fidelity — 只改了 新手配置指南.md 和 tasks.json 两个文件; 未碰工程源码/CMakeLists/工具链/keybindings/openocd.cfg; 仍无 printf/EIDE 告诫; 中文; flash 未加 $gcc。

## Commit strategy
- 可选单次提交 `docs+build: 精简指南、加 J-Link 说明、build 报错可跳转`。除非用户要求否则不提交。

## Success criteria
- 第1章变成简洁下载清单(要下啥+网址), 新手一眼能懂。
- 全文去掉了看不懂/没必要的解释, 直接可操作。
- 文档有 J-Link 使用说明(OpenOCD 最小改法)。
- 按快捷键/任务编译时, 报错进"问题"面板、终端路径可点击跳转、有颜色。
- 只动两个文件, 其它一切不变; 仍不含 printf 重定向/EIDE 告诫。
