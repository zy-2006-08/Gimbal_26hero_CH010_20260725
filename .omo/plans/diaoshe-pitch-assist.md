# diaoshe-pitch-assist - Work Plan

## TL;DR (For humans)
- **What you'll get**: 吊射模式下按 R 键，云台 pitch 平滑（缓动，不冲击机械）移动到雷达目标角 `-vision_pitch`（已含弹道补偿）。到位后仍可用 ch3/W-S 微调；操作手一动摇杆/键盘立刻中断缓动交给手动。再按 R 重新对准。yaw 不变、手动瞄。
- **Why this approach**: 复用现有吊射 MANG pitch 闭环与 R 键(deploy_flag)，最小侵入。新增 float 版缓动函数 `F_slow_ease`（照搬 `I_slow_ease` 比例缓出逻辑，不取整），解决 `Pitch_goal` 是 float 度数、int 缓动 1 度一跳的问题。
- **What it will NOT do**: 不改 yaw、不改雷达/CT 协议、不动摩擦轮/拨盘、距离数据不参与控制、不新增按键。
- **Effort**: 小。2 文件（RM_Lib.cpp/.h、my_main.cpp），6 个 todo。
- **Risk**: R 键在吊射模式双重职责（deploy_flag + pitch 触发）需明确边界；缓动参数需真车调。无硬件在环，验证靠编译+走查+真车实测。
- **Decisions**: float版缓动函数；R上升沿触发一次；复用R;缓动中动摇杆立刻中断；限幅保留。

## Scope
### In
1. RM_Lib.cpp/.h 新增 `F_slow_ease(float *in, float target, float max_step, float min_step, float k, float stop_err, float *inc_buf)`。
2. my_main.cpp 新增吊射 pitch 到位状态变量与 R 上升沿触发逻辑。
3. my_main.cpp 改吊射 pitch 控制：用 `F_slow_ease` 缓动 `Pitch_goal` 到 `-vision_pitch`，缓到位或操作手动摇杆则交出控制。
4. 保留 `LIMIT(Pitch_goal, -43, 16)`。

### Out (Must-NOT-Have)
- 不改 yaw 任何代码（YAW_Logic/YAW_PID_Calc/坐标标定）。
- 不改 CT 帧解析（VD_2rx 那三行 memcpy）。
- 不动 deploy_flag 原有展开/旋转逻辑。
- 距离 vision_distance 不接入控制。
- 不改摩擦轮、拨盘、舵机逻辑。
- 不新增遥控器按键映射。

## Verification strategy
- 构建: `cmake --build --preset Debug`（VSCode Alt+, 等价）。exit 0，无 error。
- LSP: lsp_diagnostics 对改动文件 0 error。
- 代码走查: 编排者逐行读改动，确认逻辑符合方案、无 stub、限幅在、R 双职责边界正确。
- 真车实测（交用户）: 吊射模式按 R → pitch 缓动到位无冲击；到位后 ch3 可微调；缓动中动摇杆立刻接管；再按 R 重新对准。

## Execution strategy
Todo 1（缓动函数）与 Todo 2/3（my_main 接入）有依赖：my_main 要调用 `F_slow_ease`，需先声明存在。Todo 1 先做，Todo 2/3/4 依赖它。故 Todo1 → 然后 my_main 改动串行。单文件冲突（my_main.cpp 多处改）→ 合并为一个委派任务顺序改，避免同文件并行冲突。

## Todos

- [x] 1. RM_Lib.cpp/.h 新增 float 版缓出缓动函数 `F_slow_ease` — 期望：编译通过，签名 `void F_slow_ease(float *in, float target, float max_step, float min_step, float k, float stop_err, float *inc_buf)`
  - 实现照搬 RM_Lib.cpp:111-143 的 `I_slow_ease` 比例缓出逻辑，但全程 float、不取整：`remaining=target-*in`；`fabsf(remaining)<=stop_err` 则 `*in=target;*inc_buf=0;return`；`step=remaining*k`，`fabsf` 限幅到 [min_step,max_step]，带符号；`*inc_buf` 机制可省略（float 直接累加即可）或保留一致风格——推荐直接 `*in += step`（float 无需亚整数累积），并防越过目标 `if(fabsf(step)>fabsf(remaining)) step=remaining`。
  - 头文件 RM_Lib.h:41 后加 extern 声明，紧邻 I_slow_ease，注释风格一致。
  - References: RM_Lib.cpp:108-143（I_slow_ease 实现）、RM_Lib.h:37-42（缓变工具声明区）。
  - Acceptance: 构建 exit 0；lsp_diagnostics RM_Lib.cpp/.h 0 error；函数存在且签名正确。
  - QA happy: 编译通过。QA fail: 若签名/取整错误 → 编译报错或走查发现取整，修正。
  - Commit: `feat(rmlib): add float ramp F_slow_ease for smooth pitch easing`

- [x] 2. my_main.cpp 新增吊射 pitch 到位状态变量 + F_slow_ease inc_buf — 期望：新增文件作用域变量，编译通过
  - 在 PITCH 变量区（my_main.cpp:296-311 附近）新增：`uint8_t diaoshe_pitch_arr_flag = 0;`（1=缓动进行中）、`float diaoshe_pitch_incbuf = 0;`（F_slow_ease 缓冲）。
  - References: my_main.cpp:296-311（PITCH 变量区）。
  - Acceptance: 构建 exit 0；变量定义存在。
  - Commit: `feat(gimbal): add diaoshe pitch arrival state vars`

- [x] 3. my_main.cpp 吊射模式捕获 R 上升沿触发 pitch 到位（不破坏 deploy_flag） — 期望：吊射模式按 R 置位 diaoshe_pitch_arr_flag，deploy_flag 原逻辑不受影响
  - 在 Keyboard_Special_Func()（my_main.cpp:1081 现有 UD_R 处）或 PITCH_Logic()：当 `MYmode==DIAO_SHE_MODE` 且 R 上升沿时，`diaoshe_pitch_arr_flag=1; diaoshe_pitch_incbuf=0;`。用独立 UpDown_check 实例避免与现有 UD_R 争用边沿。
  - 明确边界：deploy_flag 的 R 切换逻辑保持原样（吊射模式本就不依赖 deploy_flag 展开；确认吊射模式下切 deploy_flag 无副作用，若有则在吊射模式屏蔽 deploy_flag 的 R 切换，仅触发 pitch）。走查时确认。
  - References: my_main.cpp:1081-1087（UD_R/deploy_flag）、my_main.cpp:960-986（PITCH_Logic）、UpDown_check_class 用法 my_main.cpp:963。
  - Acceptance: 构建 exit 0；走查确认 R 在吊射模式触发 flag 且 deploy_flag 未被误改。
  - Commit: `feat(gimbal): trigger diaoshe pitch arrival on R rising edge`

- [x] 4. my_main.cpp 吊射 pitch 控制改用 F_slow_ease 缓动到 -vision_pitch — 期望：R 触发后 Pitch_goal 缓动到位，操作手动摇杆立刻接管，限幅保留
  - 在 PITCH_PID_Calc() 的 MANG 分支（my_main.cpp:1004-1010）或 PITCH_Logic() 中：
    - 若 `diaoshe_pitch_arr_flag`：调 `F_slow_ease(&Pitch_goal, -vision_pitch, <max_step>, <min_step>, <k>, <stop_err>, &diaoshe_pitch_incbuf)`；到位（fabsf(Pitch_goal-(-vision_pitch))<=stop_err）清 flag。
    - 若操作手动了 ch3（`abs(YK.yaogan.ch3)>阈值`）或 W/S 键：`diaoshe_pitch_arr_flag=0`，交给现有手动微调。
    - 删除现有瞬间赋值触发 my_main.cpp:976-979（UD_TURN_P 那段 Pitch_goal=-vision_pitch），由新缓动逻辑替代。
    - 保留 `Pitch_goal = LIMIT(Pitch_goal, -43, 16)`。
  - 缓动初始参数建议（真车调）：max_step≈0.3（度/2kHz周期）、min_step≈0.02、k≈0.02、stop_err≈0.2，附注释「真车调整」。
  - References: my_main.cpp:996-1016（PITCH_PID_Calc）、my_main.cpp:960-986（PITCH_Logic）、my_main.cpp:675（小pitch F/I_slow_ease 参数参照）、my_main.cpp:1008（限幅）。
  - Acceptance: 构建 exit 0；lsp 0 error；走查确认缓动/中断/限幅/删除旧触发都正确。
  - QA happy: 编译通过+走查逻辑闭合。QA fail: 逻辑漏洞（如手动阈值误触、未清 flag）→ 修正。
  - Commit: `feat(gimbal): smooth-ease diaoshe pitch to radar target, manual override`

- [x] 5. 构建全量验证 — 期望：`cmake --build --preset Debug` exit 0，无 error
  - References: 新手配置指南.md 第6章 build。
  - Acceptance: 构建 exit 0；固件大小正常打印。
  - Commit: 无（验证步）

## Final verification wave
- [x] F1. 计划符合度审计：所有 todo 完成，改动只在 scope 内，Must-NOT 未被触碰。
- [x] F2. 代码质量走查：逐行读 4 个改动点，无 stub/TODO/魔数无注释；F_slow_ease 逻辑与 I_slow_ease 等价（float 化）；R 双职责边界正确。（编排者亲自走查，两个评审子代理因网络证书错误中止）
- [x] F3. 构建/静态验证：build exit 0（Gimbal_Demo.elf 链接成功，仅遗留无关警告）；clangd 未安装，LSP 不可用，以编译为准。
- [x] F4. scope 保真：yaw/CT协议/摩擦轮/拨盘/距离/deploy_flag原逻辑均未被改动（git diff 确认）。

## Commit strategy
每个 todo 一个 commit（见各 todo Commit 行）。仅在用户明确要求时才 commit；默认只改代码不提交。

## Success criteria
- `cmake --build --preset Debug` exit 0，无 error。
- lsp_diagnostics 改动文件 0 error。
- 代码走查：R 触发缓动、操作手中断、限幅、删除旧瞬间赋值、F_slow_ease float 化，全部正确。
- yaw/CT/摩擦轮/拨盘/距离/deploy_flag 原逻辑零改动。
