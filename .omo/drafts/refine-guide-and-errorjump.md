---
slug: refine-guide-and-errorjump
status: awaiting-approval
intent: clear
review_required: false
pending-action: write .omo/plans/refine-guide-and-errorjump.md
approach: 4 changes. (1) Simplify 新手配置指南.md 第1章 to just "download what + official URL", drop the verbose per-tool explanations. (2) De-verbose the whole doc — remove explanations a beginner can't follow, keep the operations. (3) Add $gcc problemMatcher to tasks.json build (and build-and-flash) so compile errors get color + clickable jump + Problems panel. (4) Add a J-Link section to the doc (OpenOCD route: swap interface/jlink.cfg — minimal change).
---

# Draft: refine-guide-and-errorjump

## Decisions (user-confirmed)
- 全部一起改: 文档简化(第1章+全文去冗余) + tasks.json 加 $gcc + 文档加 J-Link 说明。
- 第1章: 只写"要下载什么 + 官方网址", 删掉每个工具的长篇 what/why/装到哪。保留下载地址即可。
- 去冗余力度(用户定): **激进精简**。只留"做什么 + 命令 + 一句话结果", 原理解释几乎全删, 全文像速查手册。仅保留极少数避坑必需的一句话(setx 危险、.c 含 class 要 LANGUAGE CXX、优化 flag 要同时给 C 和 C++)。
- J-Link 详略(用户定): **只写 OpenOCD 改法 + 一句 SEGGER 可选**。即"把 openocd.cfg 第一行换成 interface/jlink.cfg", 再加一句"也可用 SEGGER 自带 JLink 工具, 本文不展开"。不写 SEGGER 工具安装/使用细节。

## Findings (cited)
- tasks.json:12 build 的 `"problemMatcher": []` 是空 → 编译错误不进 Problems 面板、终端 file:line:col 不可点击、无红黄色。改成 `["$gcc"]`(VSCode 内置 GCC 解析器) → 错误进面板、可点击跳转、有颜色。flash 任务保持 [](openocd 输出无需解析)。build and flash 是 dependsOn, 无直接 command, problemMatcher 可留空或去掉。
- ninja --version 输出 1.13.1 = 正常(ninja 一贯只打版本号)。用户当前确实在用 Ninja(CMakePresets generator=Ninja)。这些是答疑, 不涉及改文件。
- 新手配置指南.md 现状: 778 行, 第1章(L24-69)每个工具有 它是什么/版本/官方下载/装到哪/为什么 五段, 偏长。全文多处"为什么"较学术, 新手未必需要。
- J-Link over OpenOCD: openocd.cfg 第1行 `source [find interface/cmsis-dap.cfg]` 换成 `source [find interface/jlink.cfg]` 即可, 其余不变。

## Scope IN
- 精简 新手配置指南.md 第1章为"下载清单 + 网址"(每工具 1-2 行: 名字 + 官方链接, 顶多一句它干嘛)。
- 全文去冗余: 删掉新手看不懂或非必要的原理解释, 保留操作步骤和命令; 只留真正避坑必需的极简"为什么"。
- tasks.json: build 的 problemMatcher [] → ["$gcc"]; 相应地文档第6章/第8章补充"报错会进问题面板、可点击跳转"。
- 文档新增 J-Link 一节(第5章附近或独立小节): OpenOCD 路线改 interface 行, 附最小改法; 一句话提 SEGGER 原生工具可选。

## Scope OUT (Must NOT have)
- 不改任何工程源码/CMakeLists/工具链文件/keybindings(只动 tasks.json 一处 + 文档)。
- 不删文档必要章节(第2-10章结构保留, 只是各章内部去水)。
- flash 任务的 problemMatcher 不加 $gcc(openocd 输出不是编译错误)。
- 仍不写 printf 重定向、不写"别点 EIDE 编译按钮"。
- 仅中文、仅 Windows。
- 不把 J-Link 写成大段 SEGGER 工具教程, 只给 OpenOCD 最小改法 + 一句可选提示。

## Open questions
(none)

## Approval gate
status: awaiting-approval
pending-action: write plan then user runs /start-work
