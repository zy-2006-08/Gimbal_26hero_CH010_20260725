# diaoshe-pitch-assist - draft

intent: clear
review_required: false
status: awaiting-approval
pending_action: write .omo/plans/diaoshe-pitch-assist.md

## Request (final, simplified by user)
辅助吊射，pitch 一键到位（yaw 放弃自动对准，操作手手动瞄）。
- 吊射模式下按 R（复用现有 deploy_flag 键），pitch 平滑移动到雷达目标 -vision_pitch。
- 触发语义：按 R 上升沿触发一次（不是按住持续追）；到位后操作手仍可用 ch3 / W-S 微调，不被拉回。
- 用缓动避免机械冲击。因 Pitch_goal 是 float 度数(-43~16)，需新增 float 版缓动函数（照 I_slow_ease 比例缓出逻辑，不取整），不复用 int 版。
- 距离数据不参与控制；yaw 不改。

## Grounded facts (paths)
- 雷达数据: my_main.cpp:2200 VD_2rx() CT帧解析 -> vision_pitch(off4)/vision_yaw(off8)/vision_distance(off12), 小端float32(度)。
- 吊射模式: DIAO_SHE_MODE = 拨杆 s1中 s2下。此模式 pitch 走 MANG_MODE。
- 现有 pitch 触发(要替换): PITCH_Logic() my_main.cpp:976-979, 条件 UD_TURN_P (ch0>500 && ch1<-500) 上升沿 -> Pitch_goal = -vision_pitch (瞬间赋值, 无缓动)。
- R 键: Keyboard_Special_Func() my_main.cpp:1081, 在 jianshu_ctrl_flag==ENABLE_MODE 生效; 吊射模式该标志=ENABLE(my_main.cpp:1047)。R 现切 deploy_flag。
- pitch 闭环: Pitch_calc.Mang_calc(Pitch_goal), Pitch_goal float度; 反馈 hi91.pitch。my_main.cpp:1004-1009。
- 缓动库: RM_Lib.h:40-41 只有 int 版 I_slow / I_slow_ease, 无 float 版。小pitch缓动参照 my_main.cpp:675。
- Pitch_goal 限幅 MANG 分支: LIMIT(Pitch_goal, -43, 16) my_main.cpp:1008。

## Decisions
- Q pitch缓动接口: 新增 float 版缓动函数(F_slow_ease)。
- Q R键语义: 上升沿触发一次, 之后可自由微调。
- Q R键复用: 复用现有 R(deploy_flag)。
- Q 目标pitch含弹道: 已含弹道, 直接用 -vision_pitch。
- Q 距离: 仅信息/调试, 不参与控制。
- Q yaw: 放弃自动对准, 手动瞄, 不改 yaw 代码。

## Open design note
- R 现在同时是 deploy_flag 展开切换键。吊射模式下按 R 既要触发 pitch 到位缓动, 又不能破坏 deploy_flag 原逻辑 -> 方案: 在吊射模式(DIAO_SHE_MODE)专门捕获 R 上升沿做 pitch 缓动触发, deploy_flag 逻辑保持。需在 plan 里明确 R 在吊射模式的行为边界。
