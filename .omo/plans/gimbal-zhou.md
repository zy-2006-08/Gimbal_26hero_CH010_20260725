# gimbal-zhou - 工作计划(云台轴 Gimbal_Zhou 模块化)

> 执行目标副本:D:\Newcode\...(注意:用户的 PITCH 改动目前在原工程 D:\RMUC2026\...,副本尚未同步——见下"前置")
> 本计划基于原工程 D:\RMUC2026\... 改后的当前 PITCH/YAW 代码编写。
> 风险等级:高(改动实时控制环)。建议顺序:排在 CRC、工具下沉、库去C++化、my_main 薄壳之后。必须配实车调参验证。

## 前置(执行前必须先做,否则文档与代码对不上)
用户最新的 PITCH 改动(gyr 统一为 gyr[0]、符号统一为 −OUT_PID、重力补偿全部注释关闭)当前只在原工程 D:\RMUC2026\...。执行本计划前,必须先把原工程的 PITCH_PID_Calc / PITCH_Logic 最新版同步到副本 D:\Newcode\...,并编译上车确认副本行为与原工程一致。否则本计划的 1:1 等价前提不成立。

## 编译门禁(硬规则,高于一切)
每个 Todo、每次 commit 之前,必须先 `cmake --build --preset Debug` 编译通过(0 error;warning 不阻断)才能提交、才能进入下一步。任何一步编译不过:立即停下修到过;修不过就 `git revert` 回上一个能编译的 commit,绝不把"编译不过"的状态留给下一步。本计划动实时控制环,"编译通过"只是最低门槛,之后还必须实车逐模式验证。计划最终交付前的最后一次 build 必须通过。

## TL;DR (给人看的)

**你会得到什么**:一个公版类 `Gimbal_Zhou`,把 YAW / PITCH 两根轴的"双环 PID + 反馈源 + 双组PID(常规/自瞄)切换 + 输出符号"打包。应用层只需给 `mode` 和 `goal`,类负责算 `output`。两根轴变成同一个类的两个实例,只在配置(用哪组PID、反馈哪个陀螺轴、符号)上不同。

**为什么这样做**:你刚把 PITCH 改得和 YAW 高度对称(都是 gyr 反馈、−OUT_PID、无重力补偿),现在两轴逻辑只差"反馈陀螺轴"和"用哪组PID",正好可参数化。模块化后,下一台车加一根云台轴 = new 一个实例 + 填配置,不再复制粘贴两大段 PID 代码。

**它绝不会做什么(红线)**:
- 不改变 YAW / PITCH 任何一个模式下的计算结果(外内环、反馈轴、符号、清积分时机、LP系数、目标限幅逐项 1:1 保留)。
- 不改重力补偿现状(PITCH 现在是关闭/注释的——保持关闭;YAW 本就没有)。
- 不把"进哪个模式(MYmode 判定)"搬进类里——模式判定留在应用层 PITCH_Logic/YAW_Logic(用户已认可此边界)。
- 不动 PROTECT 模式的保护赋值(在 50Hz 任务里那部分)。
- 本步不引入 CRC、软定时表、电机注册表等其它任务。

**工作量**:中(新建 gimbal_zhou.h/.cpp + 改 YAW_PID_Calc/PITCH_PID_Calc 两个函数改为调用实例)。
**风险**:高(实时控制环),必须实车逐模式验证。

**我替你做的决定**:
- 反馈陀螺轴用指针配(YAW→&gyr[2],PITCH→&gyr[0]),不写死。
- 输出符号统一为类内 `-1`(两轴现状都是 −OUT_PID);仍保留 output_sign 字段以便别的车配 +1。
- 保留"双组PID(常规 pid_wai/pid_nei + 自瞄 pid_wai_zm/pid_nei_zm)+ 切换时清对方积分"的现有行为,做进类里。
- MANG 模式(角度/编码模式)YAW 与 PITCH 用的是各自独立的第三组 PID 且 goal 处理不同——本版不强行并入 Gimbal_Zhou 的 GYRO 双组结构,MANG 分支保留在应用层原样调用(见 Scope)。

## 已核实的事实(原工程改后现状,方案地基)

YAW_PID_Calc:
- GYRO 自瞄:外 PID_Yaw_mang_zm、内 PID_Yaw_sp_zm(均 PID_update_LP,lp=kk_yaw_mang_lp/kk_yaw_sp_lp),反馈 gyr[2],输出 −OUT_PID;进入先清常规组 Integral/OUT_I。
- GYRO 非自瞄:外 PID_Yaw_mang、内 PID_Yaw_sp,反馈 gyr[2],输出 −OUT_PID;进入先清 zm 组。
- MANG:PID_YAW_Erro_IMU_MANG→PID_YAW_Erro_IMU_GYRO,反馈 gyr[2],输出 −OUT_PID(含掉头/单击偏移等英雄专属 goal 处理)。
- PROTECT:输出 0,goal=REAL_YAW_REF。

PITCH_PID_Calc(用户已改):
- GYRO 自瞄:外 PID_LK_Pitch_Mang_zm、内 PID_LK_Pitch_SP_zm,反馈 gyr[0],输出 −OUT_PID(重力补偿已注释关闭);进入先清常规组。
- GYRO 非自瞄:外 PID_LK_Pitch_Mang、内 PID_LK_Pitch_SP,反馈 gyr[0],输出 −OUT_PID(重力补偿注释);进入先清 zm 组。
- MANG:PID_LK_Erro_Pitch_IMU_MANG→PID_LK_Erro_Pitch_IMU_Gyro,反馈 gyr[0],输出 −OUT_PID(重力补偿注释)。
- PROTECT:输出 0,goal=REAL_PITCH_REF。

两轴 GYRO 模式结构完全同构,只差:反馈陀螺轴(gyr[2] vs gyr[0])、外环是否 LP(YAW 内环用 PID_update_LP 带 kk_yaw_sp_lp;PITCH 内环用 PID_update 不带LP)。→ 抽象需支持"内环是否用LP"这一差异。

## Scope

### In scope
- 新建 RM2023_Lib_V1.2/gimbal_zhou.h(类声明)+ 实现(并入 RM_Lib.cpp 或新 gimbal_zhou.cpp)。
- Gimbal_Zhou 覆盖 GYRO 模式的"自瞄/非自瞄双组PID + 清对方积分 + 反馈指针 + 符号 + 内环LP开关"。
- 改 YAW_PID_Calc / PITCH_PID_Calc 的 GYRO 分支为调用实例;MANG 与 PROTECT 分支保留应用层原样。

### Must NOT have
- 不并入 MANG 模式(YAW/PITCH 的 MANG 各有专属 goal 逻辑,本版不动)。
- 不改重力补偿现状(保持关闭)。
- 不把 MYmode 判定移入类。
- 不改 LP 系数、目标限幅、清积分时机。
- 不引入其它步骤的改动。

## Verification strategy
逐模式、逐轴上车对比。改一根轴的 GYRO 分支后,在 while(1) 同时算"旧内联算法"与"新实例",INFO 打印两者 PITCH_PID_OUT/YAW_PID_OUT,肉眼确认逐帧一致(自瞄开、自瞄关两种都测)。一致后删旧内联。两根轴分别独立验证、独立 commit。整机每个 MYmode(战斗/双重/单云台/小陀螺等)都跑一遍。

## Execution strategy
先 YAW(结构更规整、内环带LP)再 PITCH。每根轴:先加类实例并行比对→确认一致→切换→删旧内联→commit。MANG/PROTECT 分支完全不动。

## Todos

- [ ] 1. **RM2023_Lib_V1.2/gimbal_zhou.h: 新建 Gimbal_Zhou 类声明 - 期望:.cpp 能编译,字段覆盖双组PID/反馈指针/符号/内环LP开关**
  - 字段:`PID_class *pid_wai,*pid_nei,*pid_wai_zm,*pid_nei_zm; float *fankui_angle,*fankui_gyro; float output_sign; bool nei_use_lp; float lp_wai,lp_nei;`
  - 状态:`float goal,output; enum{ZHOU_PROTECT,ZHOU_GYRO} mode;`(MANG 不进类)
  - 方法:`void gyro_update(bool zimiao);`(内部:选组→清对方积分→外环 PID_update_LP(goal,*fankui_angle,lp_wai)→内环按 nei_use_lp 选 PID_update_LP 或 PID_update→output=output_sign*内环.OUT_PID)
  - References: 原工程 main.c YAW_PID_Calc / PITCH_PID_Calc 的 GYRO 分支(去查当前行)。
  - QA(happy): 空 .cpp 引用编译通过。
  - QA(fail): PID_class 未包含 → include RM_Lib.h。
  - Commit: `feat(gimbal): add Gimbal_Zhou class declaration`

- [ ] 2. **gimbal_zhou 实现: gyro_update 逐行对应现有 GYRO 分支(清积分→双环→符号) - 期望:与两轴现有内联算法逐字节等价**
  - 清积分:自瞄时清常规组,非自瞄时清 zm 组(与现状一致)。
  - 内环:nei_use_lp=true 用 PID_update_LP(...,lp_nei),false 用 PID_update(...)。
  - References: 同上。
  - QA(happy): 编译通过。
  - QA(fail): 行为不一致在 T3/T4 比对时暴露。
  - Commit: `feat(gimbal): implement gyro_update matching current inline logic`

- [ ] 3. **YAW_PID_Calc GYRO 分支改为调用 Yaw 实例 - 期望:自瞄/非自瞄两种下 YAW_PID_OUT 与改前逐帧一致**
  - 配置实例:pid_wai/nei=Yaw_mang/Yaw_sp,zm=Yaw_mang_zm/Yaw_sp_zm,fankui_gyro=&gyr[2],output_sign=-1,nei_use_lp=true,lp=kk_yaw_*。
  - MANG/PROTECT 分支不动。先并行比对打印,一致后切换删旧内联。
  - References: YAW_PID_Calc GYRO 分支。
  - QA(happy): 上车,自瞄开/关 YAW 表现与改前一致;整机各 MYmode 正常。
  - QA(fail): 不一致 git revert。
  - Commit: `refactor(gimbal): route YAW gyro through Gimbal_Zhou (verified)`

- [ ] 4. **PITCH_PID_Calc GYRO 分支改为调用 Pitch 实例 - 期望:自瞄/非自瞄两种下 PITCH_PID_OUT 与改前逐帧一致**
  - 配置:pid=LK_Pitch_Mang/SP,zm=..._zm,fankui_gyro=&gyr[0],output_sign=-1,nei_use_lp=false(PITCH 内环现用 PID_update 不带LP),lp_wai=kk_pitch_mang_lp。
  - 重力补偿保持关闭(不加回)。MANG/PROTECT 不动。先并行比对,一致后切换删旧。
  - References: PITCH_PID_Calc GYRO 分支。
  - QA(happy): 上车,自瞄开/关 PITCH 表现与改前一致。
  - QA(fail): git revert。
  - Commit: `refactor(gimbal): route PITCH gyro through Gimbal_Zhou (verified)`

## Final verification wave(全部完成后)
- [ ] F1. 留有 YAW/PITCH 各自"新旧 PID_OUT 逐帧一致"的烧录记录(自瞄开+关)
- [ ] F2. 整机逐 MYmode 测试:战斗/双重/单云台/小陀螺/吊射/上下,YAW+PITCH 表现与改前无差异
- [ ] F3. 确认 MANG 与 PROTECT 分支未被改动,重力补偿仍关闭
- [ ] F4. git diff 只涉及 gimbal_zhou.* 与 YAW_PID_Calc/PITCH_PID_Calc 的 GYRO 分支

## Commit strategy
声明+实现各一 commit;每根轴切换各一"已验证"commit。任一轴不一致单独 revert,不影响另一轴。

## Success criteria
- Gimbal_Zhou 类存在,YAW/PITCH 的 GYRO 模式经它计算,结果与改前逐帧一致(实车验证)。
- MANG/PROTECT/重力补偿现状不变。
- 两轴差异仅由配置(反馈轴指针、nei_use_lp)体现,无重复的 GYRO 分支代码。
- 未触碰其它功能模块。
