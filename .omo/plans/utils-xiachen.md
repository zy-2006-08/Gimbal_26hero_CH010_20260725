# utils-xiachen - 工作计划(通用工具函数下沉 + float 联合体统一)

> 执行目标副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动。
> 推荐顺序:在 lib-decpp(库去C++化)之后、my-main-shell(薄壳)之前做。
> 原因:本步把工具函数从 main.c 挪进 my_math;若先做薄壳,函数已在 my_main.cpp,下沉起点会变。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。计划最终交付前的最后一次 build 必须通过。烧录验证的前提永远是"已编译通过"。

## TL;DR (给人看的)

**你会得到什么**:把 main.c 里三个与机器人无关的通用"缓变/斜坡"工具函数(I16_slow / I_slow / I_slow_ease)下沉到 my_math 模块,和已有的 RampFloat 归为一类,成为公版可复用件。main.c 改为 include 调用。(float 联合体统一见"备注",本步先只做工具下沉,联合体统一列为可选。)

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
- 下沉目标选 my_math(.c/.h),与 RampFloat 同类。三函数是纯 C,放 my_math.c 合适(前提:my_math 已按 lib-decpp 回归纯 C;若 lib-decpp 未做,放这里也能编,因为 my_math 当时仍当 C++ 编,纯 C 函数在 C++ 下同样合法)。
- 保持函数名和签名不变,调用点只需能看到声明即可,无需改调用代码。
- float 联合体统一(dat/_angle/_data/FloatBytes_t/Shoot_speed_u...)牵涉多文件、多协议,单独列可选,默认不动。

## 已核实的事实
- 三函数定义:main.c I16_slow(:373)、I_slow(:402)、I_slow_ease(:433)。
- 调用点共 4 处,全在 main.c:583(I16_slow)、744(I_slow_ease)、752(I_slow)、2188(I_slow);均在 main.c 内。
- 库中无任何文件引用这三个函数。
- 依赖:abs()(stdlib)、fabsf()(math.h)——my_math.h 已 include math.h。
- my_math 已有同类函数 RampFloat(my_math.c:14),下沉后归类自然。

## Scope

### In scope
- 将 I16_slow / I_slow / I_slow_ease 的定义从 main.c 剪切到 my_math.c。
- 在 my_math.h 增加三者声明(在 extern "C" 区内,C/C++ 都可调)。
- main.c 确认已 include "my_math.h"(已 include),删除原三函数定义,调用点不变。

### Must NOT have
- 不改函数体、签名、调用点。
- 不强制做 float 联合体统一(可选,默认跳过)。
- 不动其它工具函数或库。

## Verification strategy
下沉后编译通过 + 整机所有用到缓变的功能(摩擦轮 MCL 缓变、Mini_Pitch 缓出、拨盘 BP_targe 缓变)表现与改前一致。因是逐字搬家,行为必然一致,重点验证"链接正确、无重复定义"。

## Execution strategy
一次性把三函数搬到 my_math + 头加声明 + main.c 删定义,单 commit。可选联合体统一另起 commit(默认不做)。

## Todos

- [ ] 1. **my_math.h: 声明 I16_slow/I_slow/I_slow_ease - 期望:main.c include 后可调用,C/C++ 均可**
  - 在 extern "C" 区内加三行声明(签名与 main.c 现定义完全一致)。
  - References: main.c:373,402,433 的函数签名
  - QA(happy): my_math.h 编译通过。
  - QA(fail): 签名笔误 → 与 main.c 定义逐字比对。
  - Commit: 与下条合并。

- [ ] 2. **my_math.c: 从 main.c 剪切三函数定义到此,main.c 删除原定义 - 期望:逐字搬家,编译通过,4 调用点不变**
  - 三函数体逐字移入 my_math.c(放 RampFloat 附近);main.c 删除 :373/:402/:433 三处定义;调用点 :583/:744/:752/:2188 不动。
  - References: main.c:373-464(三函数体), my_math.c:14(RampFloat 位置参考)
  - QA(happy): build 通过,无 multiple definition / undefined reference;整机 MCL缓变、Mini_Pitch缓出、拨盘缓变正常。
  - QA(fail): 链接报错 → 确认 main.c 已删原定义且 my_math.h 声明正确;行为异常 → git revert。
  - Commit: `refactor(math): sink I_slow/I16_slow/I_slow_ease into my_math`

- [ ] 3. **(可选,默认跳过)float 联合体统一评估 - 期望:仅评估,不强制改**
  - grep dat/_angle/_data/FloatBytes_t/Shoot_speed_u/Robot_pos_u 的定义与用法;评估统一成一个 f32_bytes_t 的收益与牵动范围。牵动多协议,若风险 > 收益则记录结论并跳过。
  - References: RM_Lib.h:718(dat), communication.h:64/70/185, main.c 的 Shoot_speed_u/Robot_pos_u
  - QA: 若不改,编译不受影响;若改,每处联合体替换后 build+整机验证。
  - Commit: `refactor(comm): unify float-byte unions`(仅在决定做时)

## Final verification wave(全部完成后)
- [ ] F1. build 通过,三函数在 my_math,main.c 无重复定义
- [ ] F2. 整机:摩擦轮缓变、Mini_Pitch 缓出到位、拨盘 BP_targe 缓变行为与改前一致
- [ ] F3. git diff 只涉及 my_math.h/.c 与 main.c 的三函数定义处(调用点未变)
- [ ] F4. 若做了联合体统一,单独确认各协议收发正常;若跳过,记录跳过结论

## Commit strategy
工具下沉一个 commit;联合体统一(若做)独立 commit。

## Success criteria
- I16_slow/I_slow/I_slow_ease 在 my_math,可被复用,调用点零改动。
- 整机缓变相关行为不变。
- 未触碰其它模块;联合体统一默认未做(或已单独验证)。
