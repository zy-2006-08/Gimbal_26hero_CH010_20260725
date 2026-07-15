# tune-2006-lift-pid - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 一套能真正调好这个2006抬升机构的能力——先让电机能通过串口实时改参数、在 serialplot 上看到目标和实际曲线,然后按固定流程一步步把位置环和速度环调到位(带上重力补偿,机构不再往下掉)。

**Why this approach:** 关键发现是电机输出现在被写死成0根本没通电,且参数写死在代码里改一次要重烧,所以必须先"接通输出+能实时改参+能看曲线",才谈得上调参;抬升机构有重力负载,必须加积分项才能消掉往下掉的误差。

**What it will NOT do:** 不碰拨盘、摩擦轮、云台等其他部分;不做那种强迫机构剧烈振荡的激进自动整定(有甩坏风险);不会凭空给你一组"拍死"的最终参数,最终数值要在你的真机上跑出来。

**Effort:** Medium
**Risk:** Medium - 带负载机构调参有硬件风险,已用使能开关、行程限位、单次增益不超过2倍、发散即断电等护栏兜底。
**Decisions to sanity-check:** 速度环限幅改10000、行程限位[0,40000]、改参走串口1——这三个你已确认。

Your next move: 方案已写好。运行 `/start-work` 让工作会话开始执行(改代码+硬件调参循环)。本规划会话不改代码。完整执行细节见下。

---

> TL;DR (machine): Medium effort, Medium risk. Enable runtime PID tuning over huart1 + serialplot telemetry, connect disabled 2006 motor output, add safety clamps, then worker runs fixed inner-then-outer cascade auto-tuning for the lift mechanism.

## Scope
### Must have
- 接通被禁用的2006电机输出:main.c:2102 当前发 `CAN_2.Send_RM(0x200, BP_output, 0, 0, 0)`,Mini_Pitch 电流位写死为0,电机不转。必须改回发 `Mini_Pitch_output`,并加安全保护。
- 运行时 PID 改参:通过 huart1 收文本指令,实时修改位置环/速度环的 KP/KI/KD 及各限幅,无需重新编译烧录。
- 多通道 serialplot 遥测:输出位置目标、位置实际、速度目标(=内环给定,即位置环输出)、速度实际、电流输出,格式为 serialplot 可解析的 CSV 行。
- 速度环输出限幅由 16000 改为 10000(匹配 C610 电调最大电流指令)。
- 位置目标 clamp 到 [0, 40000] 安全行程(零点=堵转零点,向上为正)。
- 固定整定流程:先内环速度环,再外环位置环,每步有明确的加减规则与达标判据,由工作会话按规则自动迭代执行。
- 两个环都加入 I 项以消除抬升机构的重力稳态误差(droop)。

### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不做继电反馈/强推临界振荡类的激进自动整定 —— 带负载抬升机构在强迫振荡下有甩坏结构/电机风险。
- 不修改其他电机或控制环:BoPan(拨盘)、六个摩擦轮、yaw、云台 pitch 一律不动。
- 不修改 CAN / DMA / 中断 等底层配置。
- 不承诺拍死的"最终调好"数值 —— 交付起始值 + 自动调整规则;最终收敛在真实硬件上完成。
- 不改动 PID_class / PID_update 的算法本身(RM_Lib.cpp:1648),只通过公开成员改参数。
- 改参指令解析不得阻塞主控制循环或干扰现有 huart1 打印。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: tests-after(嵌入式无单元测试框架)。验证 = 编译通过 + 串口读回参数确认 + serialplot 阶跃响应曲线达标判据。
- 每个改代码的 todo:必须 `cmake` 构建成功(exit 0),lsp_diagnostics 零错误。
- 调参 todo:工作会话发指令后从 huart1 读回当前参数值确认写入成功;采集阶跃响应曲线并对照判据(超调%、稳态误差、有无持续振荡)。
- Evidence: .omo/evidence/task-<N>-tune-2006-lift-pid.<ext>(构建日志、串口读回记录、serialplot 曲线截图/数据)。

## Execution strategy
### Parallel execution waves
> Wave 1 (基础设施,可并行): T1 接通输出+安全clamp、T2 运行时改参解析、T3 多通道遥测。三者改 main.c 不同区域,但都改同一文件 -> 由工作会话顺序落笔避免冲突,视为逻辑并行批次。
> Wave 2 (整定,严格串行,依赖 Wave 1 全部完成 + 真实硬件): T4 内环速度环整定 -> T5 外环位置环整定。
> Wave 3 (收尾): T6 固化最终参数回构造函数 + 文档记录。

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| T1 接通输出+clamp | - | T4,T5 | T2,T3 (同文件,顺序落笔) |
| T2 运行时改参 | - | T4,T5 | T1,T3 |
| T3 多通道遥测 | - | T4,T5 | T1,T2 |
| T4 内环整定 | T1,T2,T3 | T5 | - (串行) |
| T5 外环整定 | T4 | T6 | - (串行) |
| T6 固化参数 | T5 | - | - |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->

- [ ] 1. main.c: 接通2006电机输出 + 位置目标行程clamp + 上电保护
  What to do:
    - main.c:2102 当前是 `CAN_2.Send_RM(0x200, BP_output, 0, 0, 0);`,把 Mini_Pitch 电流位(第2个参数)从 0 改为 `Mini_Pitch_output`,即 `CAN_2.Send_RM(0x200, BP_output, Mini_Pitch_output, 0, 0);`(main.c:2101 是已注释的正确版本,可参考)。
    - 加一个全局使能开关(如 `uint8_t Mini_Pitch_tune_enable = 0;`),默认 0(不输出),只有收到运行时指令 `en 1` 才输出电流。上电默认不动,防止参数未设好就乱冲。当 enable=0 时 `Mini_Pitch_output` 强制为 0。
    - 在 main.c:2035 计算 `PID_Mini_Pitch_2006_mang.PID_update(Mini_Pitch_targe, ...)` 之前,把 `Mini_Pitch_targe` 用 `LIMIT(Mini_Pitch_targe, 0, 40000)` 夹紧(LIMIT 宏见 RM_Lib.h:417)。
    - 定义行程宏:`#define MINI_PITCH_MIN 0` / `#define MINI_PITCH_MAX 40000`,放在 main.c:284 附近的宏区。
  Must NOT do: 不动 BoPan 的第1位参数;不改 DUZHUAN_MODE / PROTECT_MODE 分支逻辑;不动 CAN 发送时序。
  Parallelization: Wave 1 | Blocked by: - | Blocks: T4,T5
  References: main.c:2035-2051(PID计算与Mini_Pitch_output赋值), main.c:2101-2102(发送), main.c:233(Mini_Pitch_targe int32), main.c:1982(Mini_Pitch_output int16), main.c:284-286(宏区), RM_Lib.h:417(LIMIT宏)
  Acceptance criteria: `cmake --build build/Debug` exit 0;lsp_diagnostics 零错误;代码审查确认 2102 行发 Mini_Pitch_output、enable 默认 0、targe 被 clamp。
  QA scenarios: happy - 硬件上发 `en 1` 后电机响应位置目标;failure - enable=0 时电机无电流输出(读 Mini_Pitch_output==0)。Evidence .omo/evidence/task-1-tune-2006-lift-pid.md
  Commit: Y | fix(mini-pitch): connect 2006 current output with travel clamp and enable guard

- [ ] 2. main.c + communication: huart1 运行时 PID 改参指令解析
  What to do:
    - 在 huart1(PRINTF_USART_HANDLE, RM_Lib.h:28)上增加接收:优先用 IDLE + DMA 不定长接收一行 ASCII 文本(参考 communication.c 里 Mini_PC 的 DMA 接收模式与 stm32f4xx_it.c 的 RxEvent 处理)。若 huart1 当前无接收配置,则在其 IRQ/RxEvent 里累积到换行符 `\n` 触发解析。
    - 指令格式(空格分隔,回车结束),解析后直接写 PID 对象公开成员:
      - `pos kp <f>` / `pos ki <f>` / `pos kd <f>` -> PID_Mini_Pitch_2006_mang.KP/KI/KD
      - `pos lp <f>` / `pos li <f>` / `pos lpid <f>` -> LIMIT_P / LIMIT_I / LIMIT_PID
      - `sp kp <f>` / `sp ki <f>` / `sp kd <f>` -> PID_Mini_Pitch_2006_sp.KP/KI/KD
      - `sp lp/li/lpid <f>` -> 速度环限幅
      - `tgt <int>` -> 设置 Mini_Pitch_targe(仍受 T1 的 clamp)
      - `en <0|1>` -> 使能电流输出(T1 的开关)
      - `show` -> 通过 INFO 打印两环当前全部参数,便于读回确认
      - `rst` -> 清零两环 OUT_I(积分),防止调参切换时积分残留
    - 改 KI 时同步注意:PID_update 用 `OUT_I += KI*error`,KI 为 0 时积分项不再增长但 OUT_I 保留旧值 -> `rst` 用于手动清零。
    - 解析放在主循环或低优先级,不得在高优先级中断里做重活;写成员是原子的单次赋值,无需加锁。
  Must NOT do: 不改 PID_update 算法;不占用其他串口;不阻塞控制循环;不破坏现有 main.c:1807 的打印。
  Parallelization: Wave 1 | Blocked by: - | Blocks: T4,T5
  References: RM_Lib.h:28-30(huart1/INFO), RM_Lib.h:425-454(PID_class成员KP/KI/KD/LIMIT_*), main.c:90-93(PID对象名), communication.c:69/495(Mini_PC DMA接收范式), stm32f4xx_it.c:82(RxEvent范式), RM_Lib.cpp:1648-1665(PID_update确认成员语义)
  Acceptance criteria: `cmake --build build/Debug` exit 0;lsp_diagnostics 零错误;硬件上发 `pos kp 1.5` 后 `show` 回读显示 KP=1.50。
  QA scenarios: happy - 发 `sp kp 0.8` + `show` 读回 0.80;failure - 发非法指令(如 `pos kx 1`)被忽略不崩溃。Evidence .omo/evidence/task-2-tune-2006-lift-pid.md
  Commit: Y | feat(mini-pitch): add runtime PID param tuning over huart1

- [ ] 3. main.c: 多通道 serialplot 遥测输出
  What to do:
    - 把 main.c:1807 的 `INFO("%d,%d\r\n", Mini_Pitch_targe, Mini_Pitch_2006.mang_inf);` 改为多通道 CSV,通道顺序固定(serialplot 按列):
      `INFO("%d,%d,%.1f,%d,%d\r\n", (int)Mini_Pitch_targe, Mini_Pitch_2006.mang_inf, PID_Mini_Pitch_2006_mang.OUT_PID, Mini_Pitch_2006.sp, Mini_Pitch_output);`
      即:通道1=位置目标, 2=位置实际(mang_inf), 3=速度目标(=位置环输出), 4=速度实际(sp), 5=电流输出。
    - 保持 `\r\n` 结尾与现有波特率不变(serialplot 已在用 huart1)。
    - 打印频率沿用现有主循环 HAL_Delay(10)(约100Hz),足够看阶跃;若曲线太疏,可在 CAN RX 处另开计数降采样打印(可选,不强制)。
  Must NOT do: 不改波特率;不删其它被注释的调试打印(保留);不把打印放进中断。
  Parallelization: Wave 1 | Blocked by: - | Blocks: T4,T5
  References: main.c:1806-1808(当前打印), main.c:2035-2037(OUT_PID/sp/output来源), RM_Lib.h:30(INFO宏), main.c:1814(HAL_Delay(10))
  Acceptance criteria: `cmake --build build/Debug` exit 0;serialplot 能解析出5个通道且数值随电机动作变化。
  QA scenarios: happy - serialplot 显示5条曲线,手动 `tgt 10000` 后位置实际趋向10000;failure - 无电机数据时打印仍不崩溃(输出0)。Evidence .omo/evidence/task-3-tune-2006-lift-pid.md
  Commit: Y | feat(mini-pitch): multi-channel serialplot telemetry

- [ ] 4. 整定内环速度环(硬件在环,工作会话自动迭代)
  What to do(工作会话按此规则循环执行:发指令 -> 采曲线 -> 判断 -> 调):
    - 前置:`en 1` 使能;先把外环 KP 设 0(`pos kp 0`)使内环可独立测试;速度环限幅 `sp lpid 10000`、`sp li` 设约 3000 起步。
    - 用 `tgt` 制造位置阶跃从而给内环一个速度给定(或临时直接给速度环固定给定,视 T2 实现)。
    - 规则:
      1. 从 `sp kp 0.5` 起,每次 ×1.5 增大,直到速度实际出现持续高频振荡/电流抖动,然后回退到该值的 ~0.6 倍。
      2. 加积分 `sp ki`:从 0.005 起,每次 ×2,直到稳态速度误差基本消除且无明显积分超调;若出现积分超调则减半。
      3. 若高频噪声大可选小量 `sp kd`(从 0.1 起),否则保持 0。
      4. 每次改参前发 `rst` 清积分,`show` 读回确认。
    - 达标判据:速度阶跃响应上升快、超调 <20%、无持续振荡、稳态误差接近0。
  Must NOT do: 不强推到发散;不在外环仍激活时调内环;单次增益跳变不超过 ×2(防甩)。
  Parallelization: Wave 2 | Blocked by: T1,T2,T3 | Blocks: T5
  References: main.c:2036(内环update:goal=位置环输出,now=sp), RM_Lib.cpp:1648-1665(积分累加语义), 本plan"整定流程"
  Acceptance criteria: 记录收敛后的 sp KP/KI/KD 值 + 阶跃曲线达标(超调<20%、无持续振荡)。
  QA scenarios: happy - 速度阶跃曲线快速跟随无振荡;failure - 若发散,enable 立即置0并回退上一组参数。Evidence .omo/evidence/task-4-tune-2006-lift-pid.md(曲线+最终参数)
  Commit: N(参数固化在 T6 统一提交)

- [ ] 5. 整定外环位置环(硬件在环,工作会话自动迭代)
  What to do:
    - 前置:内环参数已定(T4);恢复外环 `pos lpid 30000`。
    - 规则:
      1. `pos kp` 从 0.6 起,每次 ×1.3 增大,直到位置阶跃响应快但开始轻微过冲/振荡,回退到 ~0.7 倍。
      2. 加 `pos ki` 从 0.002 起 ×2 递增,专门消除抬升机构的重力稳态误差(droop);出现低频摆动则减半;配合 `pos li` 限幅防积分饱和(从 5000 起)。
      3. 加 `pos kd` 从 0.5 起抑制过冲;噪声放大则减小。
      4. 每次改参前 `rst` + `show` 读回。
    - 达标判据:位置阶跃超调 <10%、稳态误差 <±50 编码器值(重力 droop 被 I 消除)、静止无爬行/摆动。
    - 全行程验证:`tgt 0`、`tgt 20000`、`tgt 40000` 各测一次,确认全程稳定且不撞限位。
  Must NOT do: 不越过 [0,40000] 行程;单次增益跳变不超过 ×2;发散立即 `en 0`。
  Parallelization: Wave 2 | Blocked by: T4 | Blocks: T6
  References: main.c:2035(外环update:goal=Mini_Pitch_targe,now=mang_inf), main.c:284(行程参考36700), 本plan"整定流程"
  Acceptance criteria: 记录收敛后的 pos KP/KI/KD/LIMIT_I 值 + 全行程(0/20000/40000)阶跃曲线达标。
  QA scenarios: happy - 位置阶跃超调<10%、稳态误差<±50、无重力droop;failure - 发散/撞限位时 en 0 回退。Evidence .omo/evidence/task-5-tune-2006-lift-pid.md(全行程曲线+最终参数)
  Commit: N(参数固化在 T6 统一提交)

- [ ] 6. main.c: 固化最终参数 + 记录调参结果
  What to do:
    - 把 T4/T5 收敛的最终 KP/KI/KD/限幅 写回 main.c:90-93 的构造函数(PID_Mini_Pitch_2006_mang 和 _sp),更新行尾注释里的历史值。
    - 速度环限幅参数确认为 10000。
    - 保留运行时改参与遥测功能(T2/T3)以便日后微调,但把 `Mini_Pitch_tune_enable` 默认值按实际使用需求确认(调试期 0,量产可视情况)。
    - 在 .omo/evidence/ 追加最终参数表与曲线,供复现。
  Must NOT do: 不删运行时改参/遥测代码;不改其他电机参数。
  Parallelization: Wave 3 | Blocked by: T5 | Blocks: -
  References: main.c:90-93(构造函数参数), T4/T5 evidence
  Acceptance criteria: `cmake --build build/Debug` exit 0;构造函数数值 == 硬件验证收敛值;lsp_diagnostics 零错误。
  QA scenarios: happy - 重新烧录后上电即用固化参数,阶跃达标;failure - 数值笔误导致构建失败或行为回退。Evidence .omo/evidence/task-6-tune-2006-lift-pid.md
  Commit: Y | tune(mini-pitch): finalize 2006 lift cascade PID gains

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [ ] F1. Plan compliance audit - 每个 todo 的验收判据都有 evidence;固化参数==硬件收敛值。
- [ ] F2. Code quality review - 改参解析不阻塞控制循环、不破坏现有打印;LIMIT/clamp 正确;enable 保护有效;仅改 Mini_Pitch 相关代码。
- [ ] F3. Real manual QA - 硬件上全行程(0/20000/40000)阶跃:超调达标、无重力droop、无振荡、不撞限位;enable=0 时电机不动。
- [ ] F4. Scope fidelity - 未触碰 BoPan/摩擦轮/yaw/云台pitch/CAN/DMA;未做激进自动整定;未改 PID_update 算法。

## Commit strategy
- T1/T2/T3 各自独立 commit(基础设施)。
- T4/T5 不 commit(纯硬件调参,参数在 T6 固化)。
- T6 单独 commit 固化最终增益。
- 全部改动仅在 main.c、communication.*、stm32f4xx_it.c(如需 huart1 接收)范围内。不新建分支/不推送,除非用户明确要求。

## Success criteria
- `cmake --build build/Debug` exit 0,lsp_diagnostics 零错误。
- 2006 电机实际接收电流指令(2102 行发 Mini_Pitch_output),enable 保护生效。
- huart1 可实时改两环 PID 参数并 `show` 读回;serialplot 显示5通道曲线。
- 位置环阶跃:全行程超调 <10%、稳态误差 <±50 编码器值、无持续振荡、静止无重力droop。
- 速度环阶跃:超调 <20%、无持续振荡、稳态误差接近0。
- 最终增益固化回 main.c:90-93 构造函数,速度环限幅=10000,行程 clamp [0,40000]。
- 注:最终 PID 数值在真实硬件上迭代收敛,本方案交付起始值 + 自动调整规则,不预设拍死数值。
