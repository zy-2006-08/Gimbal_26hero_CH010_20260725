# my-main-shell - 工作计划(main.c 薄 C 壳 + 业务搬入 my_main.cpp + 删 -fpermissive/LANGUAGE CXX)

> 执行目标副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动。
> 推荐顺序:A 类最后做。前置:lib-decpp 与 utils-xiachen 建议先完成(否则搬运起点/内容会变)。
> 说明:注意用户的 PITCH 改动当前在原工程,若尚未同步到副本,搬运前需先同步(见 gimbal-zhou 计划的前置)。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。本计划搬运量大,尤其要做到"分批搬、每批都能编译通过"再进下一批,绝不一次剪空 main.c。计划最终交付前的最后一次 build 必须通过。

## TL;DR (给人看的)

**你会得到什么**:main.c 回归 CubeMX 生成的**纯 C**,只保留外设初始化 + 在 USER CODE 区调 My_Setup()/My_Loop()。所有 C++ 业务、CAN 中断回调、定时器中断回调搬进新建的 my_main.cpp(配 my_main.h)。最终删掉 CMakeLists 里针对 main.c/stm32f4xx_it.c 的 LANGUAGE CXX 与 -fpermissive。CubeMX 以后重新生成 main.c 不再打架。

**为什么这样做**:现在 main.c 是 .c 却塞满 C++,靠强编 hack 维持,且 CubeMX 一重生成就冲突。薄壳后业务在 .cpp 里,main.c 干净,hack 可全删,这是工程卫生的根治。

**它绝不会做什么(红线)**:
- 不改任何业务逻辑——所有函数、中断回调整体搬家,一行不改。
- 不改外设初始化顺序、时钟配置。
- 不改中断行为(HAL 弱回调在 .cpp 里定义同样覆盖,链接语义不变)。
- 不引入 CRC/工具下沉/云台轴等其它步骤的逻辑改动。
- 搬运前若副本 PITCH 未同步原工程最新版,先同步再搬(避免搬到旧逻辑)。

**工作量**:中(一次性把 main.c 约 2000 行业务搬进 my_main.cpp,但只搬一次)。
**风险**:中(搬运量大易漏;但逻辑不改,靠"搬完与改前二进制行为一致"验证)。

**我替你做的决定**:
- 文件名用 my_main.cpp / my_main.h(用户指定)。
- 入口用 My_Setup()(原 USER CODE 2 内容)+ My_Loop()(原 while(1) 内容),extern "C" 暴露给 main.c。
- 中断回调(HAL_CAN_RxFifo0/1MsgPendingCallback、HAL_TIM_PeriodElapsedCallback)与普通函数(UART3_IT_ZM、TIM12_Callback 等)全部搬入 my_main.cpp。
- main.c 顶部 `#include "my_main.h"` 放进 USER CODE Includes 区(防 CubeMX 冲掉)。
- stm32f4xx_it.c 若含 C++ 依赖,一并评估:能纯 C 化则移出名单,否则单独保留(见 Todo 6)。

## 已核实的事实
- main.c 需搬运的中断回调:HAL_CAN_RxFifo0MsgPendingCallback(:1962)、HAL_CAN_RxFifo1MsgPendingCallback(:2032)、HAL_TIM_PeriodElapsedCallback(:2157);普通函数 TIM12_Callback(:821)、UART3_IT_ZM(:1463) 及大量业务函数(MYMODE_*, MCL_Logic, BP_Logic, YAW/PITCH_*, ZM_* 等)与全局对象(CAN_1/2, 电机, PID, YK, TC, RGB_UI, BMI088 等)。
- CMakeLists.txt:47 LANGUAGE CXX 名单 = {main.c, stm32f4xx_it.c, communication.c, my_math.c};:50-56 -fpermissive 同。
- 前置计划 lib-decpp 会先移除 communication.c/my_math.c;本步负责 main.c(及评估 stm32f4xx_it.c)。
- HAL 回调为 __weak,用户在任意 .cpp/.c 中定义即覆盖官方空实现——搬进 my_main.cpp 后链接语义不变。

## Scope

### In scope
- 新建 my_main.h(extern "C" 声明 My_Setup/My_Loop)+ my_main.cpp(承载全部业务 + 中断回调 + 全局对象)。
- main.c:保留 HAL_Init/SystemClock_Config/MX_*_Init;USER CODE 2 → 调 My_Setup();while(1) → 调 My_Loop();USER CODE Includes → #include "my_main.h";删除已搬走的业务/回调/全局定义。
- CMakeLists.txt:target_sources 增加 my_main.cpp;从 LANGUAGE CXX / -fpermissive 名单移除 main.c(及 stm32f4xx_it.c,若已纯 C 化)。

### Must NOT have
- 不改任何业务逻辑、初始化顺序、中断内容。
- 不改 SystemClock_Config、MX_*_Init(CubeMX 生成部分)。
- 不删 stm32f4xx_it.c 的强编,除非确认它可纯 C 化(Todo 6 单独判断)。
- 不合并/重排搬运的函数。
- 不引入其它计划的逻辑改动。

## Verification strategy
"搬完与改前行为完全一致"为准。分阶段编译:先建 my_main.* 骨架搬入一部分、编译;再逐块搬、每搬一大块编译一次,最后 main.c 只剩壳。整机全功能回归(所有 MYmode、图传、裁判、自瞄、拨盘、摩擦轮、RGB)。删 flag 后再整机确认一次。

## Execution strategy
分阶段、可编译地搬,不要一次性剪空 main.c 再粘贴(易全崩)。顺序:①建 my_main.h/.cpp 骨架 + My_Setup/My_Loop 空实现,main.c 调用它们(此时业务仍在 main.c,先让链接跑通)②把全局对象与业务函数整体搬入 my_main.cpp ③把三个中断回调搬入 ④main.c 清理为纯壳 ⑤CMake 移除 main.c 强编 ⑥评估 stm32f4xx_it.c。每阶段 commit。

## Todos

- [ ] 1. **my_main.h: 新建,extern "C" 声明 My_Setup/My_Loop - 期望:main.c(纯C)可 include 调用**
  - `#ifdef __cplusplus extern "C" { #endif void My_Setup(void); void My_Loop(void); #ifdef __cplusplus } #endif`
  - QA(happy): main.c include 后编译通过。
  - Commit: `feat(mymain): add my_main.h entry declarations`

- [ ] 2. **my_main.cpp: 新建骨架,My_Setup/My_Loop 空壳 + CMake 加入 - 期望:能编译链接**
  - CMakeLists target_sources 加 RM2023_Lib_V1.2/my_main.cpp(或 Core/Src/,位置任选一致即可)。
  - References: CMakeLists.txt:64-74
  - QA(happy): build 通过(My_Setup/My_Loop 暂空)。
  - Commit: `feat(mymain): add my_main.cpp skeleton`

- [ ] 3. **搬运:全局对象定义 + 业务函数 从 main.c 移入 my_main.cpp - 期望:逐块搬,每块编译通过,逻辑一字不改**
  - 把 CAN/电机/PID/YK/TC/RGB_UI/BMI088 等全局定义,及 MYMODE_*/MCL_Logic/BP_Logic/YAW_*/PITCH_*/ZM_*/Servo_*/Keyboard_* 等函数整体移入 my_main.cpp;main.c 删除对应定义。
  - 分几批搬,每批 build 一次。
  - References: main.c 的 PTD/PV 区与各业务函数(去查当前行)。
  - QA(happy): 每批编译通过,无重复定义/未定义。
  - QA(fail): 漏搬依赖 → 补搬;revert 单批。
  - Commit: 每批一 commit `refactor(mymain): move <块> into my_main.cpp`

- [ ] 4. **搬运:三个中断回调 + TIM12_Callback + UART3_IT_ZM 移入 my_main.cpp - 期望:中断行为不变**
  - HAL_CAN_RxFifo0/1MsgPendingCallback、HAL_TIM_PeriodElapsedCallback、TIM12_Callback、UART3_IT_ZM 整体移入。
  - References: main.c:821,1463,1962,2032,2157
  - QA(happy): 整机中断驱动功能(电机接收、定时任务、自瞄串口)正常。
  - QA(fail): 若某回调未生效(弱符号未覆盖)→ 确认签名完全一致;revert。
  - Commit: `refactor(mymain): move ISR callbacks into my_main.cpp`

- [ ] 5. **main.c 填入 My_Setup/My_Loop 实体内容并清成纯壳 - 期望:main.c 内无 C++ 语法**
  - My_Setup() = 原 USER CODE 2 内容;My_Loop() = 原 while(1) 体(这两个实体在 my_main.cpp)。main.c 的 USER CODE 2 改为 `My_Setup();`,while(1) 改为 `My_Loop();`,Includes 区加 `#include "my_main.h"`。
  - QA(happy): 整机完整回归(所有 MYmode + 各链路)与改前一致。
  - QA(fail): revert 到上一步。
  - Commit: `refactor(mymain): main.c reduced to pure-C shell`

- [ ] 6. **CMakeLists.txt: 从 LANGUAGE CXX / -fpermissive 名单移除 main.c;评估 stm32f4xx_it.c - 期望:main.c 纯 C 编译通过,能删则删 flag**
  - 移除 main.c。检查 stm32f4xx_it.c 是否含 C++ 依赖:若无,一并移除并使其纯 C 编;若有(如调用了 C++ 对象),保留其强编或将相关内容也搬入 .cpp。
  - 若 communication.c/my_math.c(lib-decpp)已移除,则此步后 LANGUAGE CXX / -fpermissive 两段可能整段删空 → 删空。
  - References: CMakeLists.txt:47,50-56
  - QA(happy): build 通过;整机正常。
  - QA(fail): 报 C 不认语法 → 定位残留 C++,搬入 .cpp 或回退。
  - Commit: `build(mymain): drop LANGUAGE CXX / -fpermissive for main.c`

## Final verification wave(全部完成后)
- [ ] F1. main.c 内无 class/::/构造函数等 C++ 语法,纯 C 编译通过
- [ ] F2. 整机全功能回归:所有 MYmode、图传、裁判、自瞄、拨盘、摩擦轮、YAW/PITCH、RGB 与改前一致
- [ ] F3. CMakeLists 的 LANGUAGE CXX / -fpermissive 名单已不含 main.c(理想情况整段删空)
- [ ] F4. CubeMX 重新生成 main.c 的模拟测试:USER CODE 区的 include 与调用得以保留(或确认放对了区)
- [ ] F5. git diff 涉及 main.c、my_main.*、CMakeLists;业务逻辑无实质改动(仅位置变化)

## Commit strategy
骨架/搬运/清壳/删flag 分多 commit,每阶段可编译可回退。搬运量大,宁可多分几个小 commit。

## Success criteria
- main.c 为纯 C 薄壳,业务全在 my_main.cpp。
- LANGUAGE CXX / -fpermissive 对 main.c 已删(配合 lib-decpp 后理想为整段删空)。
- 整机全功能与改前完全一致(实车回归)。
- CubeMX 重生成不再冲突。
