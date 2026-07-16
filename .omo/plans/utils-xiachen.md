# utils-xiachen - 工作计划(通用工具函数下沉 + float 联合体统一)

> 执行目标副本(实际):D:\Newcode\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动。
> 推荐顺序:在 lib-decpp(库去C++化)之后、my-main-shell(薄壳)之前做。
> 原因:本步把工具函数从 main.c 挪进公版库;若先做薄壳,函数已在 my_main.cpp,下沉起点会变。
>
> ⚠️ 计划修订(2026-07-16,执行前重核):原计划的下沉目标 my_math(.c/.h)已在 lib-decpp
> 执行中被证明为整模块死代码并**彻底删除**。因此下沉落脚点改为现存核心公版库 **RM_Lib(.cpp/.h)**
> ——它已含 PID/滤波/CAN 等通用能力,main.c 已 include RM_Lib.h,三函数放入零新增文件、复用性最好。
> 下文所有 "my_math" 均已改为 "RM_Lib"。行号亦按当前 main.c 重核(前几步删除死代码后整体上移)。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。计划最终交付前的最后一次 build 必须通过。烧录验证的前提永远是"已编译通过"。

## TL;DR (给人看的)

**你会得到什么**:把 main.c 里三个与机器人无关的通用"缓变/斜坡"工具函数(I16_slow / I_slow / I_slow_ease)下沉到 RM_Lib 公版库,成为公版可复用件。main.c 已 include RM_Lib.h,调用点零改动。(float 联合体统一见"备注",本步先只做工具下沉,联合体统一列为可选。)

**为什么这样做**:这三个函数是纯通用算法(增量缓变、缓出斜坡),任何车、任何轴都可能用,却写在英雄应用层 main.c。下沉后下一台车直接复用,不用复制。

**它绝不会做什么(红线)**:
- 不改这三个函数任何一行逻辑(逐字搬家)。
- 不改 4 个调用点的参数、行为。
- 不改函数签名(保持 I16_slow/I_slow/I_slow_ease 原名原参,调用点零改动)。
- 本步不做联合体统一的强制改动(列为可选 Todo,默认不做,避免牵动多处)。
- 不引入 CRC/库去C++/薄壳等其它步骤。

**工作量**:小(3 函数搬家 + 头文件加声明 + main.c 加 include)。
**风险**:低(纯搬家,函数是纯 C 逻辑,依赖 abs/fabsf)。

**我替你做的决定**:
- 下沉目标改为 RM_Lib(.cpp/.h)——原定的 my_math 已删。RM_Lib 是现存核心公版库,main.c 已 include RM_Lib.h,三函数是纯 C 逻辑,放 RM_Lib.cpp(当 C++ 编)合法且复用性最好,零新增文件。
- 保持函数名和签名不变,调用点只需能看到声明即可,无需改调用代码。
- float 联合体统一(dat/_angle/_data/FloatBytes_t/Shoot_speed_u...)牵涉多文件、多协议,单独列可选,默认不动。

## 已核实的事实(2026-07-16 执行前重核)
- 三函数定义:main.c I16_slow(:372)、I_slow(:401)、I_slow_ease(:432),函数体覆盖 :372-464。
- 调用点共 4 处,全在 main.c:582(I16_slow)、743(I_slow_ease)、751(I_slow)、2182(I_slow);均在 main.c 内。
- 库中无任何文件引用这三个函数。
- 依赖:abs()、fabsf()(math.h)。RM_Lib.h 已 include math.h(:15);RM_Lib.cpp 当 C++ 编,abs/fabsf 均可得(原 my_math.c 同样用 abs 且编译通过)。
- 原同类函数 RampFloat 曾在 my_math.c,已随 my_math 删除;本步三函数下沉到 RM_Lib 独立成"通用缓变工具"区。

## Scope

### In scope
- 将 I16_slow / I_slow / I_slow_ease 的定义从 main.c 剪切到 RM_Lib.cpp。
- 在 RM_Lib.h 增加三者声明(用 extern "C" 或与现有 C 可见声明同区,C/C++ 都可调)。
- main.c 已 include "RM_Lib.h",删除原三函数定义,调用点不变。

### Must NOT have
- 不改函数体、签名、调用点。
- 不强制做 float 联合体统一(可选,默认跳过)。
- 不动其它工具函数或库。

## Verification strategy
下沉后编译通过 + 整机所有用到缓变的功能(摩擦轮 MCL 缓变、Mini_Pitch 缓出、拨盘 BP_targe 缓变)表现与改前一致。因是逐字搬家,行为必然一致,重点验证"链接正确、无重复定义"。

## Execution strategy
一次性把三函数搬到 my_math + 头加声明 + main.c 删定义,单 commit。可选联合体统一另起 commit(默认不做)。

## Todos

- [ ] 1. **RM_Lib.h: 声明 I16_slow/I_slow/I_slow_ease - 期望:main.c include 后可调用,C/C++ 均可**
  - 加三行声明(签名与 main.c 现定义完全一致)。放在文件作用域的函数声明区(如 CRC/LIMIT 附近),用 `#ifdef __cplusplus extern "C"` 或直接放 C 可见处;由于 RM_Lib.h 无统一 extern "C" 包裹,声明为普通 C 链接即可(main.c 当 C++ 编、RM_Lib.cpp 当 C++ 编,C++ 名字修饰一致,无需 extern "C")。
  - References: main.c:372,401,432 的函数签名
  - QA(happy): RM_Lib.h 编译通过。
  - QA(fail): 签名笔误 → 与 main.c 定义逐字比对。
  - Commit: 与下条合并。

- [ ] 2. **RM_Lib.cpp: 从 main.c 剪切三函数定义到此,main.c 删除原定义 - 期望:逐字搬家,编译通过,4 调用点不变**
  - 三函数体逐字移入 RM_Lib.cpp(放一个新的"通用缓变工具"区,如 CRC 区附近);main.c 删除 :372/:401/:432 三处定义;调用点 :582/:743/:751/:2182 不动。
  - References: main.c:372-464(三函数体)
  - QA(happy): build 通过,无 multiple definition / undefined reference;整机 MCL缓变、Mini_Pitch缓出、拨盘缓变正常。
  - QA(fail): 链接报错 → 确认 main.c 已删原定义且 RM_Lib.h 声明正确;行为异常 → git revert。
  - Commit: `refactor(lib): sink I_slow/I16_slow/I_slow_ease into RM_Lib`

- [ ] 3. **(可选,默认跳过)float 联合体统一评估 - 期望:仅评估,不强制改**
  - grep dat/_angle/_data/FloatBytes_t/Shoot_speed_u/Robot_pos_u 的定义与用法;评估统一成一个 f32_bytes_t 的收益与牵动范围。牵动多协议,若风险 > 收益则记录结论并跳过。
  - References: RM_Lib.h:718(dat), communication.h:64/70/185, main.c 的 Shoot_speed_u/Robot_pos_u
  - QA: 若不改,编译不受影响;若改,每处联合体替换后 build+整机验证。
  - Commit: `refactor(comm): unify float-byte unions`(仅在决定做时)

## Final verification wave(全部完成后)
- [ ] F1. build 通过,三函数在 RM_Lib,main.c 无重复定义
- [ ] F2. 整机:摩擦轮缓变、Mini_Pitch 缓出到位、拨盘 BP_targe 缓变行为与改前一致
- [ ] F3. git diff 只涉及 my_math.h/.c 与 main.c 的三函数定义处(调用点未变)
- [ ] F4. 若做了联合体统一,单独确认各协议收发正常;若跳过,记录跳过结论

## Commit strategy
工具下沉一个 commit;联合体统一(若做)独立 commit。

## Success criteria
- I16_slow/I_slow/I_slow_ease 在 RM_Lib,可被复用,调用点零改动。
- 整机缓变相关行为不变。
- 未触碰其它模块;联合体统一默认未做(或已单独验证)。
