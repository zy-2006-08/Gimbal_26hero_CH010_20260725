# lib-decpp - 工作计划(库去 C++ 化,方向A:让 .c 回归纯 C)

> 执行目标副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动。
> 推荐顺序:三份 A 类文档里最先做(最独立、零新增 .cpp)。与 CRC 合并互不冲突。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。计划最终交付前的最后一次 build 必须通过。烧录验证的前提永远是"已编译通过"。

## TL;DR (给人看的)

**你会得到什么**:`communication.c` 和 `my_math.c` 从"假 C 真 C++(被 -fpermissive + LANGUAGE CXX 强编)"回归为**真正的纯 C**。做法是删掉这两个 .c 里没用到的 `#include "RM_Lib.h"`(绕开那个 2009 行的 C++ 大头),再清掉头文件里几处真正的 C++ 语法(class Kf、两处结构体成员默认初始化、补 stdbool),Kf 的实现搬进已有的 RM_Lib.cpp。零新增 .cpp 文件。

> 注:本节曾误以为"只有 2 个 constexpr + class Kf"。经实测更正——见下方"已核实的事实"。旧的 constexpr 障碍已在 CRC 合并计划里被顺带删除,不复存在。

**为什么这样做**:公版库要给多台车复用,库文件"名字是 .c 却要当 C++ 编"很脏、易踩坑。清理后库文件类型名副其实。这也是后续"删掉 CMakeLists 里 LANGUAGE CXX / -fpermissive"的前提之一(另一半是 main.c 薄壳,见 my-main-shell 计划)。

**它绝不会做什么(红线)**:
- 不改变任何计算结果:Kf 类实现整体搬家逻辑不动;去掉的两处结构体成员默认初始化(head[2]、cnt_max)均已核实其结构体零使用或无代码依赖该默认值。
- 不改任何函数签名、调用点。
- 不改 RM_Lib.h(靠删两个 .c 的死 include 绕开)。
- 本步**暂不删** CMakeLists 的 LANGUAGE CXX / -fpermissive(main.c 仍需它,等 my-main-shell 做完再统一删)——本步只让 communication.c/my_math.c 从那两行的名单里**移除**。
- 不新增 .cpp 文件(Kf 实现并入已存在的 RM_Lib.cpp)。
- 不碰 CP_System.c(它本就是纯 C 编,不在 LANGUAGE CXX 名单)。

**工作量**:小。
**风险**:低(语法等价替换 + 一段实现搬家)。

**我替你做的决定**:
- Kf 类实现搬进现有 RM_Lib.cpp(不新建 my_math.cpp),避免多文件。
- my_math.h 的 class Kf 声明用 `#ifdef __cplusplus` 包裹,纯 C 的 .c 包含时自动跳过。
- 不去改 RM_Lib.h 让它 C 兼容(工程量巨大、风险高),而是删掉两个 .c 里对它的死 include。
- 去掉 head[2]/cnt_max 的默认初始化(纯 C 不支持),前提是先 grep 证明没有行为依赖。

## 已核实的事实(2026-07-16 用纯 C 编译器实测重核,推翻了旧版的错误假设)

> 旧版本节声称"C++ 障碍只有 2 个 constexpr + class Kf",经 `arm-none-eabi-gcc -x c -fsyntax-only` 实测,该假设错误且会导致按原计划执行必崩编译。以下为实测结论。

### 已自动消除的
- **communication.h 原 :141 `constexpr CRC8_INIT` / :161 `constexpr CRC16_INIT` 已不存在**:crc-gongban 计划(CRC 合并)执行时,这两个常量作为"死掉的重复表常量"已被删除。全库现零 `constexpr`。原 Todo 1 已自动满足,无需再做。

### 真正的、且旧计划漏掉的 C++ 障碍
1. **两个 .c 都 `#include "RM_Lib.h"`,而 RM_Lib.h 是 2009 行、含 27 个 class、约 60 处 C++ 成员默认初始化(`成员 = 值;`)的纯 C++ 头,且无 `#ifdef __cplusplus` 保护。** 这是纯 C 编译的头号障碍:一遇 `class USER_CAN` 即报 `unknown type name 'class'`。旧计划完全没查这条传递依赖。
   - **关键缓解**:实测(注释剥离后)**两个 .c 的正式代码都没用到 RM_Lib.h 的任何符号**——communication.c 里 BMI088/YK 等只出现在大段注释块里;my_math.c 正式代码只用到 my_math.h 自己的结构体(Vision_process_t、extKalman_t)和 class Kf。故 **`#include "RM_Lib.h"` 在这两个文件里是死 include,可直接删除**,无需改 RM_Lib.h。
2. **communication.h:150 `uint8_t head[2] = {'S','P'};`**(结构体 `GimbalToVision` 的 C++ 成员默认初始化)——纯 C 报错。
   - **关键事实**:`GimbalToVision` 结构体全工程**仅有定义、零实例化**(死结构体)。`GetReceive_SP()` 用的是宏 `SP_HEADER`(0x66)/`SP_TAIL`(0x11) 和 `buf[]` 下标,**并未使用该结构体**。
3. **communication.h:189 `bool check_crc16(...)`** 在纯 C 下需 `#include <stdbool.h>`(该头未包含 stdbool)。
4. **my_math.h(约 :109)`uint32_t cnt_max = 300;`**(结构体 `Anti_top_Data` 的 C++ 成员默认初始化)——已用最小样例实测:C11 报 `expected ... before '=' token`,C++ 通过。确系 C++ 语法,是 my_math.c 转纯 C 的真障碍。
5. **my_math.h:63 `class Kf{...}`;my_math.c 内 `double Kf::KalmanFilter(...)`**——旧计划已知的这条依然成立。

### 其它事实
- CMakeLists.txt:47 LANGUAGE CXX 名单 = {main.c, stm32f4xx_it.c, communication.c, my_math.c};:50-56 -fpermissive 同名单。
- CP_System.c 不在名单(纯 C 编),本步不动。
- Kf 实例定义在 main.c(当 C++ 编)的 PM 区,不影响本步。

## Scope

### In scope
communication 组:
- communication.c:3 删除死 `#include "RM_Lib.h"`(正式代码零引用)。
- communication.h:150 `uint8_t head[2] = {'S','P'};` 去掉默认初始化(该结构体零实例化;改成 `uint8_t head[2];`,不影响任何行为)。
- communication.h 顶部补 `#include <stdbool.h>`(为 :189 的 `bool` 提供 C 定义)。
- my_math.h:63 class Kf 声明加 `#ifdef __cplusplus` 守卫(communication.h 传递包含 my_math.h,不守卫则纯 C 见到 class 报错)。
- CMakeLists.txt:47 与 50-56 名单里**移除** communication.c。

my_math 组:
- my_math.c 删除死 `#include "RM_Lib.h"`(正式代码零引用)。
- my_math.h(约 :109)`uint32_t cnt_max = 300;` 去掉结构体内默认初始化 → `uint32_t cnt_max;`,并在实例定义处(main.c:1701 `Anti_top_Data TOP_Data;`)用指定初始化器 `Anti_top_Data TOP_Data = {.cnt_max = 300};` 显式赋 300。**已实测**:当前 C++ 靠自动生成的构造器把 cnt_max 设为 300(汇编里 `mov r2,#300`),全工程仅 main.c 定义此实例、my_math.c:448 读它、无别处赋值;若只删不补会退化为 0,改变行为,故必须在外面补 300。
- my_math.h:63 class Kf 加 `#ifdef __cplusplus` 守卫(与 communication 组同一处改动,先做者完成即可)。
- my_math.c 内 `Kf::KalmanFilter` 实现剪切到 RM_Lib.cpp。
- CMakeLists.txt:47 与 50-56 名单里**移除** my_math.c。

### Must NOT have
- 不删 LANGUAGE CXX / -fpermissive 整行(main.c 仍需);只从名单移除两个库文件。
- 不改 Kf 算法、不改任何计算逻辑。
- 不新建 .cpp(Kf 实现并入已存在的 RM_Lib.cpp)。
- **不改 RM_Lib.h**(靠删两个 .c 里的死 include 绕开它,而非改这个 2009 行大头)。
- 不碰 CP_System.c、不碰其它库函数。
- 不引入 CRC/工具下沉/薄壳等其它步骤。
- 去掉 `head[2]`/`cnt_max` 默认初始化前,必须先 grep 确认无任何代码依赖这两个默认值;若有依赖,改为在使用处显式赋值,绝不静默改变行为。

## Verification strategy
以"能否纯 C 编译通过 + 整机行为不变"验证。每改一个文件重新 build。Kf 搬家后,卡尔曼相关功能(视觉/自瞄滤波若用到 Kf)整机确认无异常。communication.c 转纯 C 后,SuperPower/Mini_PC 收发正常。

## Execution strategy
先做共用前置(class Kf 守卫),再清 communication 的头障碍与死 include,再清 my_math(死 include + cnt_max + Kf 搬家),最后逐个从 CMake 名单移除并以纯 C 验证。每步一 commit,失败单独 revert。communication 组(Todo 2/3/5)与 my_math 组(Todo 4/6)相对独立,任一组失败可单独回退,不影响另一组。

## Todos

> 顺序:先把两个 .c 都能纯 C 编译的"头文件障碍"清干净(Todo 1-4),再逐个从 CMake 名单移除并验证(Todo 5-6)。class Kf 守卫是两组共用的前置项,放在最前。

- [ ] 1. **my_math.h: class Kf 声明加 #ifdef __cplusplus 守卫 - 期望:纯 C include 此头不报错,C++ 仍见得到 Kf**
  - my_math.h 已有 `extern "C" { ... }` 块(:5 开始,:176 结束)。class Kf 目前在这个 extern "C" 块内且无守卫。用 `#ifdef __cplusplus ... #endif` 单独包裹 class Kf 声明。
  - References: my_math.h:63-71(class Kf),:4-6 与 :176-178(extern "C" 边界)
  - QA(happy): 一个纯 C 文件 include my_math.h 编 C 通过;现有 C++ 文件仍能用 Kf。
  - QA(fail): C 仍见到 class → 守卫位置/嵌套不对,调整。
  - Commit: `refactor(math): guard class Kf for C compat`

- [ ] 2. **communication.h: 去掉 head[2] 默认初始化 + 补 stdbool - 期望:纯 C 可编译,行为不变**
  - 先 grep 确认 `GimbalToVision` 零实例化(已核实:仅定义无使用)。:150 `uint8_t head[2] = {'S','P'};` → `uint8_t head[2];`。
  - communication.h 顶部(其它 include 附近)补 `#include <stdbool.h>`,供 :189 `bool` 使用。
  - References: communication.h:148-156(结构体),:189(bool),:8-11(include 区)
  - QA(happy): 暂不改 CMake,C++ 编译仍通过(值/用法不变)。
  - QA(fail): 若发现有代码实例化 GimbalToVision 并依赖默认 head → 改为使用处显式赋值,不静默改行为。
  - Commit: `refactor(comm): drop C++ member-init in unused struct + add stdbool`

- [ ] 3. **communication.c: 删除死 #include "RM_Lib.h" - 期望:编译通过(正式代码零引用该头)**
  - :3 `#include "RM_Lib.h"` 删除(已核实:正式代码未用其任何符号,BMI088/YK 仅在注释块)。
  - References: communication.c:3
  - QA(happy): 当前(仍 C++ 编)build 通过,无 undefined。
  - QA(fail): 报缺符号 → 说明有活跃引用,恢复 include 并记录具体符号。
  - Commit: `refactor(comm): drop dead RM_Lib.h include`

- [ ] 4. **my_math.c: 删死 include + 去 cnt_max 默认初始化 + Kf 实现搬到 RM_Lib.cpp - 期望:纯 C 障碍清零,逻辑不变**
  - my_math.c 删除 `#include "RM_Lib.h"`(已核实正式代码零引用)。
  - my_math.h(约 :109)`uint32_t cnt_max = 300;` → `uint32_t cnt_max;`;先 grep 确认无代码依赖默认值 300,若有则在初始化处显式赋 300。
  - 把 my_math.c 内 `Kf::KalmanFilter` 整个实现剪切进 RM_Lib.cpp(该文件已 include my_math.h,能看到 class Kf);my_math.c 删除该实现。
  - References: my_math.c(include 区、Kf::KalmanFilter 实现),my_math.h:98-111(Anti_top_Data)
  - QA(happy): 当前(仍 C++ 编)build 通过,无 undefined reference to Kf::KalmanFilter。
  - QA(fail): 链接未定义 → 确认实现已在 RM_Lib.cpp 且 include 正确。
  - Commit: `refactor(math): move Kf impl to RM_Lib.cpp, drop dead include + C++ member-init`

- [ ] 5. **CMakeLists.txt: 从 LANGUAGE CXX 与 -fpermissive 名单移除 communication.c - 期望:按纯 C 编译通过**
  - :47 与 :50-56 两处名单删掉 communication.c(其余保留)。
  - References: CMakeLists.txt:47,50-56
  - QA(happy): build 通过,communication.c 以 C 编译无报错。
  - QA(fail): 报 C 不认语法 → 定位残留 C++ 特性,补到 Todo 2/3 或回退本步。
  - Commit: `build(comm): compile communication.c as pure C`

- [ ] 6. **CMakeLists.txt: 从名单移除 my_math.c - 期望:按纯 C 编译通过,整机卡尔曼相关功能正常**
  - :47 与 :50-56 删掉 my_math.c。
  - References: CMakeLists.txt:47,50-56
  - QA(happy): build 通过,my_math.c 以 C 编译无报错。
  - QA(fail): 报 C 不认语法 → 定位残留,补到 Todo 1/4 或回退。
  - Commit: `build(math): compile my_math.c as pure C`

## Final verification wave(全部完成后)
- [ ] F1. clean rebuild 通过,且 communication.c/my_math.c 已不在 CMakeLists 的 LANGUAGE CXX / -fpermissive 名单,确以 C(非 C++)编译(可查 compile_commands.json 里对应条目为 arm-none-eabi-gcc)。
- [ ] F2. 整机:SuperPower 自瞄、Mini_PC、卡尔曼滤波相关功能正常。
- [ ] F3. main.c、stm32f4xx_it.c 仍保留在名单中(本步不动它们)。
- [ ] F4. 无新增 .cpp 文件;git diff 只涉及 communication.h/.c、my_math.h/.c、RM_Lib.cpp、CMakeLists.txt。
- [ ] F5. 确认删除的两处默认初始化(head[2]、cnt_max)无行为依赖(grep 佐证已在对应 Todo 记录)。

## Commit strategy
每文件一 commit,communication 与 my_math 两组独立,任一组失败单独 revert。

## Success criteria
- communication.c、my_math.c 以纯 C 编译通过并从 CXX 名单移除。
- 零新增 .cpp;Kf 实现在 RM_Lib.cpp;RM_Lib.h 未被改动。
- 整机相关功能行为不变。
- main.c 的 CXX 强编暂留(待 my-main-shell 完成后统一删)。
