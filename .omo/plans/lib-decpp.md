# lib-decpp - 工作计划(库去 C++ 化,方向A:让 .c 回归纯 C)

> 执行目标副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 不动。
> 推荐顺序:三份 A 类文档里最先做(最独立、零新增 .cpp)。与 CRC 合并互不冲突。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。计划最终交付前的最后一次 build 必须通过。烧录验证的前提永远是"已编译通过"。

## TL;DR (给人看的)

**你会得到什么**:`communication.c` 和 `my_math.c` 从"假 C 真 C++(被 -fpermissive + LANGUAGE CXX 强编)"回归为**真正的纯 C**。做法是把它们里/头文件里仅有的几处 C++ 语法(communication.h 的 2 个 constexpr、my_math 的 class Kf)清理掉或收进已有的 .cpp。零新增 .cpp 文件。

**为什么这样做**:公版库要给多台车复用,库文件"名字是 .c 却要当 C++ 编"很脏、易踩坑。清理后库文件类型名副其实。这也是后续"删掉 CMakeLists 里 LANGUAGE CXX / -fpermissive"的前提之一(另一半是 main.c 薄壳,见 my-main-shell 计划)。

**它绝不会做什么(红线)**:
- 不改变任何计算结果:constexpr→const 只是关键字换法,值不变;Kf 类实现整体搬家,逻辑不动。
- 不改任何函数签名、调用点。
- 本步**暂不删** CMakeLists 的 LANGUAGE CXX / -fpermissive(main.c 仍需它,等 my-main-shell 做完再统一删)——本步只让 communication.c/my_math.c 从那两行的名单里**移除**。
- 不新增 .cpp 文件(Kf 实现并入已存在的 RM_Lib.cpp)。
- 不碰 CP_System.c(它本就是纯 C 编,不在 LANGUAGE CXX 名单)。

**工作量**:小。
**风险**:低(语法等价替换 + 一段实现搬家)。

**我替你做的决定**:
- Kf 类实现搬进现有 RM_Lib.cpp(不新建 my_math.cpp),避免多文件。
- my_math.h 的 class Kf 声明用 `#ifdef __cplusplus` 包裹,纯 C 的 .c 包含时自动跳过。
- constexpr 改 `static const`(C11 也接受,值与链接性等价)。

## 已核实的事实
- communication.h:141 `constexpr uint8_t CRC8_INIT = 0xff;`、:161 `constexpr uint16_t CRC16_INIT = 0xffff;` —— 这两处 C++ 关键字污染了头,导致包含它的 communication.c 必须当 C++ 编。
- my_math.h:63 `class Kf{...}`;my_math.c:167 `double Kf::KalmanFilter(...)` —— 这个类成员实现是 my_math.c 必须当 C++ 编的唯一原因。
- CMakeLists.txt:47 LANGUAGE CXX 名单 = {main.c, stm32f4xx_it.c, communication.c, my_math.c};:50-56 -fpermissive 同名单。
- CP_System.c 不在名单(纯 C 编),本步不动。
- Kf 使用点:my_math.c:12 `extern Kf kalman_speedYaw1,...`;需确认 Kf 实例在哪定义(main.c PM 区 `Kf kalman_speedYaw1,...`)——实例定义在 .cpp/当C++编的文件里即可。

## Scope

### In scope
- communication.h:141,161 两个 constexpr → static const。
- my_math.h:63 class Kf 声明加 `#ifdef __cplusplus` 守卫。
- my_math.c:167 的 `Kf::KalmanFilter` 实现剪切到 RM_Lib.cpp。
- CMakeLists.txt:47 与 50-56 的名单里**移除** communication.c 和 my_math.c(保留 main.c、stm32f4xx_it.c 不动)。

### Must NOT have
- 不删 LANGUAGE CXX / -fpermissive 整行(main.c 仍需);只从名单移除两个库文件。
- 不改 Kf 算法、不改 constexpr 的值。
- 不新建 .cpp。
- 不碰 CP_System.c、不碰其它库函数。
- 不引入 CRC/工具下沉/薄壳等其它步骤。

## Verification strategy
以"能否纯 C 编译通过 + 整机行为不变"验证。每改一个文件重新 build。Kf 搬家后,卡尔曼相关功能(视觉/自瞄滤波若用到 Kf)整机确认无异常。communication.c 转纯 C 后,SuperPower/Mini_PC 收发正常。

## Execution strategy
先动 communication(仅头文件改 2 行 + CMake 移除),再动 my_math(搬 Kf + 头加守卫 + CMake 移除)。每个文件一 commit,失败单独 revert。

## Todos

- [ ] 1. **communication.h: 两个 constexpr 改为 static const - 期望:值不变,C 可编译**
  - :141 `constexpr uint8_t CRC8_INIT=0xff;`→`static const uint8_t CRC8_INIT=0xff;`;:161 同理 CRC16_INIT。
  - References: communication.h:141,161
  - QA(happy): 暂不改 CMake,先确认改后 C++ 编译仍通过(值/用法不变)。
  - QA(fail): 若某处依赖 constexpr 编译期特性报错 → 记录该用法,单独处理。
  - Commit: `refactor(comm): constexpr -> static const (value unchanged)`

- [ ] 2. **CMakeLists.txt: 从 LANGUAGE CXX 与 -fpermissive 名单移除 communication.c - 期望:communication.c 按 .c 纯 C 编译通过**
  - :47 与 :50-56 两处名单删掉 communication.c 一项(其余保留)。
  - References: CMakeLists.txt:47,50-56
  - QA(happy): build 通过,communication.c 以 C 编译无报错。
  - QA(fail): 若报 C 不认的语法 → 说明还有残留 C++ 特性,定位并转 C 写法或回退本步。
  - Commit: `build(comm): compile communication.c as pure C`

- [ ] 3. **my_math.c: Kf::KalmanFilter 实现剪切到 RM_Lib.cpp - 期望:实现整体搬家,逻辑不变,编译通过**
  - 把 my_math.c:167 起的 Kf::KalmanFilter 整个函数移入 RM_Lib.cpp(RM_Lib.cpp 已 include my_math.h,能看到 class Kf)。my_math.c 里删除该实现。
  - References: my_math.c:167
  - QA(happy): 编译通过,无 undefined reference to Kf::KalmanFilter。
  - QA(fail): 链接报未定义 → 确认 RM_Lib.cpp 确有该实现且 include 正确。
  - Commit: `refactor(math): move Kf impl into RM_Lib.cpp`

- [ ] 4. **my_math.h: class Kf 声明加 #ifdef __cplusplus 守卫 - 期望:纯 C 的 .c 包含此头不报错**
  - class Kf{...} 用 `#ifdef __cplusplus ... #endif` 包裹(注意该头已有 extern "C" 块,class 需放在 C++ 可见区)。
  - References: my_math.h:63
  - QA(happy): 一个纯 C 文件 include my_math.h 编译通过。
  - QA(fail): C 编译仍见到 class → 守卫位置不对,调整。
  - Commit: `refactor(math): guard class Kf for C compat`

- [ ] 5. **CMakeLists.txt: 从名单移除 my_math.c - 期望:my_math.c 按 .c 纯 C 编译通过**
  - :47 与 :50-56 删掉 my_math.c 一项。
  - References: CMakeLists.txt:47,50-56
  - QA(happy): build 通过,my_math.c 以 C 编译无报错;整机卡尔曼相关功能正常。
  - QA(fail): 报 C 不认语法 → 定位残留,回退。
  - Commit: `build(math): compile my_math.c as pure C`

## Final verification wave(全部完成后)
- [ ] F1. build 通过,且 communication.c/my_math.c 已不在 CMakeLists 的 LANGUAGE CXX / -fpermissive 名单
- [ ] F2. 整机:SuperPower 自瞄、Mini_PC、卡尔曼滤波相关功能正常
- [ ] F3. main.c、stm32f4xx_it.c 仍保留在名单中(本步不动它们)
- [ ] F4. 无新增 .cpp 文件;git diff 只涉及 communication.h、my_math.h/.c、RM_Lib.cpp、CMakeLists.txt

## Commit strategy
每文件一 commit,communication 与 my_math 两组独立,任一组失败单独 revert。

## Success criteria
- communication.c、my_math.c 以纯 C 编译通过并从 CXX 名单移除。
- 零新增 .cpp;Kf 实现在 RM_Lib.cpp。
- 整机相关功能行为不变。
- main.c 的 CXX 强编暂留(待 my-main-shell 完成后统一删)。
