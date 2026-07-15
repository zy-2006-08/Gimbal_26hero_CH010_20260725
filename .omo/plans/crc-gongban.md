# crc-gongban - 工作计划(CRC 公版化 / 合并)

> 执行目标工程副本:D:\Newcode\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111
> 原工程 D:\RMUC2026\... 保持不动,作为黄金兜底。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。计划最终交付前的最后一次 build 必须通过。烧录验证的前提永远是"已编译通过"。

## TL;DR (给人看的)

**你会得到什么**:一个统一的 `crc.h` / `crc.c` 模块,把工程里散落 4 处、共 4 种不同的 CRC 算法(A: CRC-ITU16 取反;B: DJI-CRC16;C: DJI-CRC8;D: Mini_PC 旧 CRC8)各保留唯一一份实现。原来各文件里的重复实现改成调用这个模块,消除重复的 CRC 表(省 flash),消除"改一处忘改另一处"的隐患。

**为什么这样做**:公版库要给多台车复用,CRC 是最典型的"每台车都一样"的通用能力,现在却抄了 4~5 份。合并后库更干净、更可信。

**它绝不会做什么(你的红线)**:
- 不改变任何一种 CRC 的计算结果(初值、查表、是否取反、返回值一字节不改,只是搬家)。
- 不改任何调用点的行为、参数、协议帧格式、字节偏移、大小端。
- 不动 A/B 两种算法"表相同但收尾不同"的区别——分别保留,各留各的常量。
- 不在本步顺手做其它重构(不碰 my_main 拆分、不碰 constexpr、不删 -fpermissive)。

**工作量**:小(新建 2 文件 + 改 3~4 处调用 + CMake 加 1 行)。
**风险**:低——每种算法都用真实数据帧逐字节比对新旧输出一致后才删旧实现;git 每步可回退。

**我替你做的决定**:
- 保留全部 4 种算法,不试图"统一成一种"(协议不同,合并会算错)。
- 新模块用纯 C 写(`crc.c`),`extern "C"` 包裹,C 和 C++ 都能调。
- 先"新增+改调用+验证",确认无误后才删旧代码,而不是一步到位。

## 已核实的事实(方案地基)

工程里有 4 种不同 CRC,不可混用:
- 算法A CRC-ITU16:RM_Lib.cpp `GetCrc16`/`IsCrc16Good`,表 `crctab16`,收尾 `~fcs` 取反、magic `0xf0b8`,用于图传整包。
- 算法B DJI-CRC16:CP_System.c `Get_CRC16_Check_Sum`、communication.c `get_crc16`,初值 `0xffff` 不取反,用于裁判系统/SuperPower。
- 算法C DJI-CRC8:CP_System.c `Get_CRC8_Check_Sum`、communication.c 侧,初值 `0xff`,DJI 帧头。
- 算法D Mini_PC-CRC8:communication.c `cal_crc_table`,表 `crc_table`,初值 `0x00`。
- **关键**:A 与 B 的 256 项表数值相同,但一个取反一个不取反 —— 这就是"看到表一样也绝不能合并"的原因。

## Scope

### In scope
- 新建 `RM2023_Lib_V1.2/crc.c` + `crc.h`,收纳 4 种算法各一份。
- CMakeLists.txt `target_sources` 增加 `crc.c`。
- 将现有实现改为调用新模块(逐个验证后):
  - A: RM_Lib.cpp `GetCrc16`/`IsCrc16Good`
  - B: CP_System.c `Get_CRC16_Check_Sum`;communication.c `get_crc16`/`check_crc16`
  - C: CP_System.c `Get_CRC8_Check_Sum`
  - D: communication.c `cal_crc_table`
- 删死代码:`RC::get_crc16_check_sum`/`RC::verify_crc16_check_sum`(注释已标"没用")。

### Must NOT have
- 不得改任何 CRC 表数值。
- 不得把 A 表与 B 表"合并成一张共用"——即使同值,各留常量名,避免误用收尾逻辑。
- 不得改调用点的字节偏移、帧长、seq、大小端。
- 不得在本步引入 my_main 拆分、constexpr 替换、软定时表。
- 删任何实现前必须 grep 确认零引用。

## Verification strategy
每种算法独立验证:`crc.c` 就位、调用点改一处后,**先不删旧实现**,在 `while(1)` 用一帧固定数据分别用【旧】和【新】各算一次,`INFO("old=%04X new=%04X\r\n",...)` 打印,肉眼确认逐字节相等 → 该算法通过 → 删旧实现并 commit。四种逐个过。测试数据优先用真实收到的一帧,拿不到就用固定字节数组(比的是新旧一致)。

## Execution strategy
一次一种算法,顺序:A(图传,单文件最独立)→ D(Mini_PC)→ C(CRC8)→ B(裁判系统,调用点最多最关键)。每种一个独立 commit,失败可单独 revert。

## Todos

### 批次1:搭骨架(不改调用,先让模块存在且能编译)

- [ ] 1. **RM2023_Lib_V1.2/crc.h: 新建头,声明4种算法接口 - 期望:.c 和 .cpp 都能 include 不报错**
  - `#ifdef __cplusplus extern "C" {` 包裹;声明 `crc_itu16`/`crc_itu16_verify`(A)、`crc_dji16`(B)、`crc_dji8`(C)、`crc_minipc8`(D)。
  - Ref: RM_Lib.cpp:70-96(A), CP_System.c:785-820(B), CP_System.c:749-777(C), communication.c:53-61(D)
  - QA happy: 空 crc.c 与一个 .cpp 引用此头编译通过。
  - QA fail: 漏 extern "C" → C++ 链接报错 → 加回。
  - Commit: `feat(crc): add crc.h with 4 algorithm declarations`

- [ ] 2. **RM2023_Lib_V1.2/crc.c: 新建实现,4种表和逻辑逐字节从原处照抄 - 期望:编译通过,与原实现逐字节等价**
  - A表 crctab16 + GetCrc16 逻辑→crc_itu16/verify;B表 wCRC_Table→crc_dji16;C表 CRC8_TAB→crc_dji8;D表 crc_table→crc_minipc8。
  - 表数值一个不改;A/B 表虽同值仍各建常量不共用。
  - QA happy: 加入 CMake 后 build 通过。
  - QA fail: multiple definition → 新表加 crc_ 前缀。
  - Commit: `feat(crc): add crc.c with 4 algorithms copied verbatim`

- [ ] 3. **CMakeLists.txt: target_sources 增加 RM2023_Lib_V1.2/crc.c - 期望:crc.c 参与编译**
  - 第69行 CP_System.c 下方加一行。Ref: CMakeLists.txt:64-74
  - QA: 重新 configure+build,产物含 crc.c.obj。
  - Commit: `build(crc): compile crc.c`

### 批次2:逐算法切换+验证+删旧(每算法一 commit)

- [ ] 4. **算法A(图传): RM_Lib.cpp GetCrc16/IsCrc16Good 改为调用 crc_itu16 - 期望:图传收发正常,新旧逐字节一致**
  - 先在 while(1) 加临时比对打印,烧录确认相等;再把函数体改成 `return crc_itu16(...)`,保留函数名(调用点不动)。
  - Ref: RM_Lib.cpp:70-96;调用点 RM_Lib.h:1178
  - QA happy: 烧录后 rx_crc_ok_cnt 正常增长、err 不涨。
  - QA fail: err 涨立即 git revert。
  - Commit: `refactor(crc): route ITU16 through crc module (verified equal)`

- [ ] 5. **算法D(Mini_PC): communication.c cal_crc_table 改为调用 crc_minipc8 - 期望:帧尾校验字节不变**
  - 比对打印后切换。Ref: communication.c:53-61,283,312
  - QA happy: 上位机正常解析 Mini_PC 帧。 QA fail: revert。
  - Commit: `refactor(crc): route MiniPC crc8 through crc module (verified equal)`

- [ ] 6. **算法C(DJI-CRC8): CP_System.c Get_CRC8_Check_Sum 改为调用 crc_dji8 - 期望:帧头CRC8不变**
  - Get_CRC8 内部 `return crc_dji8(...)`,Append 不动。比对打印后切换。Ref: CP_System.c:749-784
  - QA happy: 裁判系统数据正常收发。 QA fail: revert。
  - Commit: `refactor(crc): route DJI crc8 through crc module (verified equal)`

- [ ] 7. **算法B第一处: CP_System.c Get_CRC16_Check_Sum 改为调用 crc_dji16 - 期望:裁判系统整包校验不变**
  - Get_CRC16 内部 `return crc_dji16(...)`,Append 不动。比对打印后切换。Ref: CP_System.c:785-828
  - QA happy: 裁判系统 UI 绘制/接收正常。 QA fail: revert。
  - Commit: `refactor(crc): route CP_System DJI16 through crc module (verified equal)`

- [ ] 8. **算法B第二处: communication.c get_crc16/check_crc16 改为调用 crc_dji16 - 期望:SuperPower 校验不变**
  - get_crc16 内部 `return crc_dji16(...)`;check_crc16 的大小端组装不动。比对打印后切换。Ref: communication.c:29-50,164,208
  - QA happy: SuperPower 自瞄 CRC 通过率不变。 QA fail: revert。
  - Commit: `refactor(crc): route SuperPower DJI16 through crc module (verified equal)`

### 批次3:清理死代码

- [ ] 9. **删除 RC::get_crc16_check_sum/verify_crc16_check_sum 死代码 - 期望:零引用,删后仍能编译**
  - 先 grep 确认全工程零调用;删 RM_Lib.cpp:2387-2412 + RM_Lib.h:944-945 声明。
  - QA happy: 编译通过无 undefined reference。 QA fail: 有隐藏调用则不删恢复。
  - Commit: `chore(crc): remove dead RC crc methods`

- [ ] 10. **(可选)评估 RM_Lib.h 内联 CRC16_class 是否有引用 - 期望:确认后决定删或留**
  - grep `CRC16_class`;有引用保留不动,无引用删声明。Ref: RM_Lib.h:514-557
  - Commit: `chore(crc): drop unused CRC16_class`(仅确认无引用时)

## Final verification wave(全部完成后)
- [ ] F1. 核对第4-8项每种算法都留有"新旧比对一致"的烧录记录
- [ ] F2. 全工程 grep crctab16/wCRC_Table/CRC8_TAB/crc_table,确认除 crc.c 外无重复表定义
- [ ] F3. 实车整机:图传、裁判系统、SuperPower、Mini_PC 四条链路全部正常
- [ ] F4. git diff 只涉及 crc.*/CMakeLists/4个调用文件,未碰其它

## Commit strategy
每个 Todo 一个 commit;批次2 每个都是"已验证一致"后才提交,任一步出问题 `git revert` 单步回退。

## Success criteria
- crc.c/.h 含 4 种算法各一份,表数值与原实现逐字节相同。
- 4 条通信链路行为与改动前完全一致(实车验证)。
- 除 crc.c 外无重复 CRC 表;死代码已删。
- 未触碰任何非 CRC 功能;-fpermissive/LANGUAGE CXX 本步不动。
