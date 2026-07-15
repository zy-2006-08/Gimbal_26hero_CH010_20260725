# soft-timer-table - 工作计划(软定时任务表 / 定时器派发公版化)

> 执行目标副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动,黄金兜底。
> 前置:建议在 CRC 合并(crc-gongban)之后做,但两者不冲突,也可独立进行。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。提取任务函数时"剪一个编译一次"。计划最终交付前的最后一次 build 必须通过。

## TL;DR (给人看的)

**你会得到什么**:把 `HAL_TIM_PeriodElapsedCallback` 里那一长串 `if(htim==&htimX) else if...` 的手写派发,换成一张"定时器→任务函数"的表 + 一个通用的遍历派发。以后加/删一个周期任务,只改表一行,派发框架对所有车通用。

**为什么这样做**:现在的中断回调把"派发逻辑"和"每个定时器干什么"糊在一起,又长又难读(你原话:有点丑)。表驱动后,派发是公版通用件,各任务内容是车型专属件,清晰分层。

**它绝不会做什么(红线)**:
- 不改变任何一个定时器任务的执行内容、执行频率、执行顺序。
- 不拆 htim1 的 `scan_tim` 内部半分频(你已定:不拆)——它连同 htim7 的 `Send_flag`、htim9 的 `can_scan` 一起,原样保留在各自任务函数体内部。
- 不改任何 `static` 局部变量的语义(它们随函数体一起搬,仍是函数内 static)。
- 不动定时器的 Prescaler/Period 配置(tim.c 不碰)。
- 本步不引入 CRC、my_main 拆分、云台轴等其它任务。

**工作量**:小(新建若干任务函数是"剪切原分支体"+ 一张表 + 重写回调,单文件 main.c 内完成)。
**风险**:低——纯派发方式替换,任务体逐字照搬;可与改前行为逐项对比。

**我替你做的决定**:
- 不引入 fenpin/xiangwei 相位机制(排查后发现不需要,见下)。最终是最简单的"一表一函数"。
- 任务函数命名按频率+用途,不含"英雄"字样,便于公版复用。

## 已核实的事实(方案地基)

排查全部已启用定时器(tim.c 配置 + main.c 回调):
- htim12 = 1kHz:纯 IMU 读取(TIM12_Callback),无内部分频。
- htim9  = 2kHz:内部 `can_scan` 两半交替(PID计算+发送 / 自瞄+拨盘+通信),各等效 1kHz。→ 内部 toggle 保留。
- htim7  = 160Hz:内部 `Send_flag` 交替设 can2_bjtx010/011,其余每次都跑。→ 内部 toggle 保留。
- htim1  = 100Hz:内部 `scan_tim` 仅 TX_VD_Deal 减半,其余每次跑。→ 你已定:不拆,原样保留。
- htim8  = 20Hz:置 can2bjtx013/014,无内部分频。(注释写40Hz与配置对不上,不在本步处理)
- htim6  = 10Hz:喂狗 + YK_ctrl,无内部分频。
- htim5  = 800Hz:填四元数/视觉发送,无内部分频。
- **结论**:所有"中断内再分频"都是"大部分每次跑、一小段 toggle"的同一性质,既然 htim1 不拆,其余同理全部保留在任务函数内部。因此不需要相位机制,表里一个定时器对一个任务函数即可。

## Scope

### In scope(仅 main.c 内)
- 为每个已启用且在回调里有分支的定时器(htim12/9/7/1/8/6/5)各提取一个任务函数,函数体 = 原 `if(htim==...)` 分支体逐字剪切。
- 新增 `SoftTask` 结构 + `task_table[]` + 重写 `HAL_TIM_PeriodElapsedCallback` 为遍历派发。

### Must NOT have
- 不得改任务体任何一行逻辑(含 static 变量、内部 toggle)。
- 不得改任务触发频率、顺序。
- 不得动 tim.c、不得动 CubeMX 生成区。
- 不得顺手"修复"htim8 频率注释、不得合并任何任务。
- 不得引入相位/分频参数(已确认不需要)。

## Verification strategy
改前先记录一段基准:在原回调各分支入口加临时计数器(如 `t9_cnt++`),烧录 10 秒记下各定时器命中次数与频率。改成表驱动后,同样计数,确认每个任务的命中频率与改前**完全一致**。再整机跑一遍确认行为无变化。确认后删临时计数器。

## Execution strategy
一次性在 main.c 内完成结构替换(因为是同一个回调函数,无法半改)。但提取任务函数时逐个剪切、剪一个编译一次,确保搬运无误后再重写回调。全部完成一个 commit(可与基准计数验证记录一起)。

## Todos

- [ ] 1. **main.c: 提取 htim12/9/7/1/8/6/5 的分支体为独立任务函数(逐字剪切) - 期望:每个函数体与原分支一字不差,含内部 toggle 和 static**
  - 命名建议:Task_1kHz_IMU、Task_2kHz_Main(含can_scan)、Task_160Hz(含Send_flag)、Task_100Hz(含scan_tim,不拆)、Task_20Hz、Task_10Hz_Watchdog、Task_800Hz_Vision。
  - 做法:把 `if(htim==&htimX){...}` 大括号内的内容整体移入 `void Task_X(void){...}`,static 变量随之移入函数内(语义不变)。
  - References: main.c 的 HAL_TIM_PeriodElapsedCallback(去查当前行)。
  - QA(happy): 每提取一个,编译通过。
  - QA(fail): 若 static 变量被别处引用而报错,则该变量提升为文件作用域(仅此情况),记录之。
  - Commit: 与下条合并提交。

- [ ] 2. **main.c: 新增 SoftTask 结构 + task_table[] + 重写 HAL_TIM_PeriodElapsedCallback 为遍历派发 - 期望:派发行为与原 if-else 链完全等价**
  - 结构:`struct SoftTask { TIM_HandleTypeDef *tim; void(*fn)(void); };`
  - 表:每个定时器一行,指向对应任务函数。
  - 回调:`for` 下标循环(不用 auto/范围for),`if(table[i].tim==htim) table[i].fn();`。
  - References: 同上。
  - QA(happy): 加临时命中计数,烧录确认每定时器频率与改前一致;整机图传/裁判/云台/拨盘/自瞄行为无变化。
  - QA(fail): 某任务频率变了或不跑 → git revert,回到原 if-else。
  - Commit: `refactor(timer): table-driven timer dispatch (behavior verified identical)`

- [ ] 3. **main.c: 删除验证用的临时命中计数器 - 期望:恢复干净,仍能编译上车**
  - References: 上一步加的计数器。
  - QA: 编译通过,整机正常。
  - Commit: `chore(timer): remove temp dispatch counters`

## Final verification wave(全部完成后)
- [ ] F1. 逐个定时器核对改前/改后命中频率一致的烧录记录
- [ ] F2. 整机跑:图传、裁判、云台YAW/PITCH、拨盘、摩擦轮、自瞄全部正常
- [ ] F3. git diff 只涉及 main.c 的回调区与新增任务函数,未碰 tim.c 与其它模块
- [ ] F4. 确认 htim1/7/9 的内部 toggle 原样保留、未被拆分

## Commit strategy
提取+重写合为一个已验证 commit;删临时计数器一个 commit。出问题单步 git revert。

## Success criteria
- HAL_TIM_PeriodElapsedCallback 变为表驱动遍历派发。
- 7 个定时器任务的频率、顺序、内容与改前完全一致(烧录验证)。
- htim1/7/9 内部 toggle 保留未拆。
- 未触碰 tim.c、CubeMX 生成区、其它功能模块。

## 备注:后续步骤的未定案记录(不在本计划执行范围)
- 云台轴 Gimbal_Zhou(#7):用户倾向"PITCH 去掉重力补偿"。已核实:重力补偿当前在 PITCH 自瞄分支实际生效,去掉属功能修改,需实车验证;且 PITCH 自瞄/非自瞄还存在 gyr[1] vs gyr[2]、正号 vs 负号 的差异,去重力补偿不能单独消除不对称。此步风险最高,排最后,做时作为一次功能变更配实车调参,届时单独立文档。
