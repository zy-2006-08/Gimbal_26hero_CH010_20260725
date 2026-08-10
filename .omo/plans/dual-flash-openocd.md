# dual-flash-openocd - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 一个烧录快捷键，插上 J-Link 就用 J-Link 烧、没插就用正点原子 DAP 烧，全自动识别，你什么都不用切。原来的 DAP 烧录和调试一切照旧。

**Why this approach:** 两个烧录器统一走 OpenOCD（你现在就在用），所以同一条命令、同一个快捷键就能通吃；用一个几十毫秒的 USB 检测脚本自动判断插的是哪个。不用 J-Flash，因为它是图形界面没法一键、也没法和 DAP 共用一个键。

**What it will NOT do:** 不碰你的固件代码和构建；不改调试配置（调试仍走 DAP）；不引入 J-Flash 到这个快捷键里。

**Effort:** Short
**Risk:** Low - 只新增两个文件 + 改一个 task 的一行命令，DAP 原流程零改动
**Decisions to sanity-check:** (1) J-Link 走 OpenOCD 的速度设 8000kHz（比 DAP 的 4000 高，V9 撑得住）；(2) 检测规则=认到 J-Link 就用 J-Link，否则默认 DAP；(3) 本次只做烧录自动切换，调试暂不做。

Your next move: 确认这个方案（或调整上面 3 个决策），我就开始实现。真机实烧要等你接好 SWD 线，但脚本和配置本次就能全部做好并验证。

---

> TL;DR (machine): Short effort, Low risk. Add openocd-jlink.cfg + scripts/flash.ps1 (VID_1366 detect) + rewire flash task. DAP path untouched.

## Scope
### Must have
- 保留现有 DAP 烧录流程，一个字不改：`openocd.cfg`（cmsis-dap 接口）继续可用。
- 新增 J-Link (V9) 走 OpenOCD 的接口配置文件 `openocd-jlink.cfg`（jlink 接口 + swd + stm32f4x + 合适的 adapter speed）。
- 新增一个检测脚本 `scripts/flash.ps1`：按下同一个 `flash` 快捷键时，自动扫 USB —— 认到 J-Link（VID 0x1366）就用 jlink 配置，否则用 DAP 配置，然后调用同一个 openocd 命令烧 `build/Debug/Gimbal_Demo.elf`。
- 修改 `.vscode/tasks.json` 的 `flash` 任务，让它调用检测脚本，而不是直接写死 openocd 命令。`build` 和 `build and flash` 行为不变。
- 检测脚本要能明确报告：用的是哪个烧录器、没插任何烧录器时给清晰错误、openocd 失败时透传退出码。

### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不引入 J-Flash / JLink.exe 作为快捷键后端（GUI 无法一键、命令行只认 J-Link，破坏单快捷键统一）。J-Flash 手动用不受影响，但不进这个流程。
- 不修改 `launch.json`（cortex-debug 调试保持走 DAP，本次不做调试自动切换）。
- 不修改 `openocd.cfg`（DAP 配置原样保留）。
- 不改任何固件源码、CMake 构建、时钟或定时器配置。
- 不加"粘性记忆/缓存上次烧录器"之类的复杂逻辑（USB 检测本身是毫秒级，无需优化）。
- 不硬编码 openocd 绝对路径到脚本里，除非 PATH 里找不到 openocd（此时回退到已知安装路径 `C:\Tools\xpack-openocd-0.12.0-7-win32-x64\xpack-openocd-0.12.0-7\bin`）。
- 不同时挂两个烧录器上总线（这是使用约束，脚本只选一个 interface，天然规避）。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none（这是构建/工具配置，非业务逻辑；用实跑脚本 + openocd dry-run 验证，不引入测试框架）
- 验证手段：
  1. `openocd -f openocd-jlink.cfg` 语法/加载校验（不接硬件也能验证 config 能被解析、interface driver 存在）。
  2. `scripts/flash.ps1` 的 USB 检测分支：在**当前插着 J-Link** 的环境下，实跑脚本确认它正确选中 jlink 分支并打印。
  3. 拔掉 J-Link（或模拟无 J-Link）时，脚本回退到 DAP 分支的逻辑走查 + 实跑。
  4. 全链路实烧留待用户接好 SWD 线后手动确认（硬件当前 SWD 未接，无法在本次自动完成）。
- Evidence: `.omo/evidence/task-<N>-dual-flash-openocd.txt`

## Execution strategy
### Parallel execution waves
> Wave 1 可并行：T1（jlink cfg）+ T2（检测脚本）互不依赖。Wave 2：T3（接线 tasks.json）依赖 T1+T2。Wave 3：T4 验证。

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1. openocd-jlink.cfg | - | 3, 4 | 2 |
| 2. flash.ps1 检测脚本 | - | 3, 4 | 1 |
| 3. 接线 tasks.json | 1, 2 | 4 | - |
| 4. 验证 | 1, 2, 3 | - | - |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [ ] 1. 新建 `openocd-jlink.cfg`（J-Link 走 OpenOCD 的接口配置）
  What to do: 在项目根目录（与 `openocd.cfg` 同级）新建 `openocd-jlink.cfg`，内容对标现有 [openocd.cfg](openocd.cfg) 但把接口换成 jlink：
    ```
    adapter driver jlink
    transport select swd
    source [find target/stm32f4x.cfg]
    adapter speed 8000
    ```
    说明：现有 DAP 用 `adapter speed 4000`；J-Link 可拉高到 8000（V9 硬件支持，SWD 线质量好可更高）。选 8000 作为稳妥高速档。
  Must NOT do: 不要用旧语法 `interface jlink`（0.12 已弃用，用 `adapter driver jlink`）；不要改 `openocd.cfg`；不要写死设备序列号。
  Parallelization: Wave 1 | Blocked by: - | Blocks: 3, 4
  References: [openocd.cfg](openocd.cfg)（现有 DAP 配置，4 行，作为模板）；openocd 安装 scripts 目录 `C:\Tools\xpack-openocd-0.12.0-7-win32-x64\xpack-openocd-0.12.0-7\scripts`（含 interface/jlink.cfg、target/stm32f4x.cfg）
  Acceptance criteria (agent-executable): `openocd -f openocd-jlink.cfg -c "init; shutdown"` —— 在无硬件时应能加载 config 并报"找不到设备/无法连接"（证明 config 语法正确、jlink driver 存在）；不能报 config 解析错误或未知命令。
  QA scenarios: happy — 有 J-Link 且 SWD 接好时 `init` 成功打印 target 信息；failure — 无 J-Link 时报 `No J-Link device found`（预期，说明 config 本身没问题）。Evidence `.omo/evidence/task-1-dual-flash-openocd.txt`
  Commit: N（末尾统一提交）

- [ ] 2. 新建 `scripts/flash.ps1`（自动检测烧录器并调用 openocd）
  What to do: 新建 `scripts/flash.ps1`，逻辑：
    1. 定位 openocd：先 `Get-Command openocd`，找不到则回退 `C:\Tools\xpack-openocd-0.12.0-7-win32-x64\xpack-openocd-0.12.0-7\bin\openocd.exe`。
    2. 扫 USB 判断 J-Link 是否在线：`Get-PnpDevice` 过滤 `InstanceId -match "VID_1366"` 且 Status 为 OK/正常的 BULK/Composite 接口。
    3. 认到 J-Link → `$cfg = "openocd-jlink.cfg"`，打印 `[flash] Detected J-Link (V9), using OpenOCD + jlink`；否则 `$cfg = "openocd.cfg"`，打印 `[flash] No J-Link found, using CMSIS-DAP`。
    4. 执行 `& $openocd -f $cfg -c "program build/Debug/Gimbal_Demo.elf verify reset exit"`，用 `$LASTEXITCODE` 作为脚本退出码（`exit $LASTEXITCODE`）。
    5. 接收可选参数 `-Elf <path>` 覆盖默认 elf 路径，默认 `build/Debug/Gimbal_Demo.elf`。
  Must NOT do: 不加粘性缓存；不同时选两个 interface；不吞掉 openocd 退出码；不 `--no-verify` 之类跳过校验；工作目录假定为项目根（tasks.json 会保证 cwd）。
  Parallelization: Wave 1 | Blocked by: - | Blocks: 3, 4
  References: 之前检测证据 —— J-Link VID `1366` PID `0105`，正常时 `USB Composite Device` Status=OK + `BULK interface` MI_02；DAP 是 CMSIS-DAP HID 免驱。现有 flash 命令见 [.vscode/tasks.json](.vscode/tasks.json#L15-L19)。
  Acceptance criteria (agent-executable): 当前插着 J-Link 时 `powershell -File scripts/flash.ps1` 首行打印应为 `[flash] Detected J-Link`；把检测函数抽出单独 dry-run（不真烧）确认分支选择正确。
  QA scenarios: happy — 插 J-Link 跑脚本 → 选中 jlink 分支；failure — J-Link 拔掉/未识别 → 回退 DAP 分支且打印清晰提示。Evidence `.omo/evidence/task-2-dual-flash-openocd.txt`
  Commit: N

- [ ] 3. 修改 `.vscode/tasks.json` 的 `flash` 任务调用检测脚本
  What to do: 把 [.vscode/tasks.json](.vscode/tasks.json#L14-L19) 的 `flash` 任务 command 从写死的 openocd 命令改为：
    `"command": "powershell -ExecutionPolicy Bypass -File ${workspaceFolder}/scripts/flash.ps1"`
    保留 `label: flash`、`problemMatcher: []`。`build` 和 `build and flash` 任务保持不变（`build and flash` 仍 dependsOn [build, flash]，自动继承新行为）。
  Must NOT do: 不动 `build` 任务；不动 `build and flash` 的 dependsOn 结构；不改 `launch.json`；保持 tasks.json JSON 合法。
  Parallelization: Wave 2 | Blocked by: 1, 2 | Blocks: 4
  References: [.vscode/tasks.json](.vscode/tasks.json)（全 30 行，flash 在 L14-19，build and flash 在 L20-28）
  Acceptance criteria (agent-executable): tasks.json 通过 JSON 解析（`Get-Content tasks.json | ConvertFrom-Json` 不报错）；flash.command 含 `flash.ps1`。
  QA scenarios: happy — VS Code 运行 `flash` 任务触发脚本；failure — JSON 语法错误会被 ConvertFrom-Json 捕获。Evidence `.omo/evidence/task-3-dual-flash-openocd.txt`
  Commit: N

- [ ] 4. 端到端验证 + 提交
  What to do:
    1. 无硬件下 `openocd -f openocd-jlink.cfg -c "init; shutdown"` 验证 jlink config 可加载（预期报无设备，非语法错）。
    2. 当前插着 J-Link → 跑 `scripts/flash.ps1` 的检测部分，确认打印 `Detected J-Link`。
    3. `ConvertFrom-Json` 校验 tasks.json。
    4. 全部通过后，`git add openocd-jlink.cfg scripts/flash.ps1 .vscode/tasks.json` 并提交（仅当用户已确认要提交）。
    5. 输出给用户的说明：接好 SWD 线后如何做真机全链路烧录测试（拔掉 DAP，只留 J-Link，运行 flash 任务）。
  Must NOT do: 不提交固件改动或无关文件；SWD 未接时不谎报"烧录成功"，如实说明真机烧录待用户接线后验证。
  Parallelization: Wave 3 | Blocked by: 1, 2, 3 | Blocks: -
  References: 全部上述文件 + 之前 JLink.exe 确诊结果（硬件+驱动 OK，SWD 线未接故芯片 attach 失败）
  Acceptance criteria (agent-executable): 三项自动验证（config 加载 / 脚本检测分支 / tasks.json JSON 合法）全绿并留证据文件。
  QA scenarios: happy — 三项全过；failure — 任一项失败则修复后重跑。Evidence `.omo/evidence/task-4-dual-flash-openocd.txt`
  Commit: Y（用户确认后）| chore(flash): add J-Link OpenOCD config + auto-detect flash script

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [ ] F1. Plan compliance audit
- [ ] F2. Code quality review
- [ ] F3. Real manual QA
- [ ] F4. Scope fidelity

## Commit strategy
- 单次提交（用户确认后）：`chore(flash): add J-Link OpenOCD config + auto-detect flash script`
- 只暂存 3 个文件：`openocd-jlink.cfg`、`scripts/flash.ps1`、`.vscode/tasks.json`。
- 不提交任何固件/构建/无关改动。

## Success criteria
- 一个 `flash` 快捷键：插着 J-Link 时自动用 J-Link 烧，没插时自动用 DAP 烧，无需改任何配置或手动切换。
- 现有 DAP 流程（`openocd.cfg`、`build`、`build and flash`、`launch.json` 调试）行为完全不变。
- `openocd-jlink.cfg` 可被 OpenOCD 正确加载（config 无语法错）。
- `scripts/flash.ps1` 在当前插着 J-Link 的环境下正确识别并选中 jlink 分支；拔掉时回退 DAP。
- `.vscode/tasks.json` 为合法 JSON，`flash` 任务指向检测脚本。
- 真机全链路烧录（实际写入芯片）待用户接好 SWD 线后验证——本次不阻塞交付，已在说明中告知步骤。
