# chassis-safe-deploy-guard - draft

intent: clear
review_required: false
status: awaiting-approval
pending_action: write .omo/plans/chassis-safe-deploy-guard.md

## Request (确认后的需求)
一层可选的"底盘保险":双下(键鼠)按R进入部署高转速(5250)后,如果底盘4个电机全部掉线,
持续监测;等4个电机全部恢复上线时,自动 deploy_flag=0 退出部署 -> 摩擦轮自然回落到 Near(3700)。
平常不生效,只有调试变量 chassis_safe_enable 手动置1时才跑。

## 已确认的决策
1. 开启方式:源码级编译开关 `#define CHASSIS_SAFE_ENABLE 1` + `#if` 包裹整段功能。
   改成0重编译=整段代码不进固件(等同注释掉,零开销)。持久设置,与重启无关。
   (用户明确:不要运行时变量,要像注释/取消注释一样的源码开关。)
2. 下线判定:必须 M1&&M2&&M3&&M4 全部==0 才算"下线";恢复=4个全部==1。
3. 恢复动作:直接 deploy_flag=0(退出部署),摩擦轮经 MCL_Logic 自然回 Near。
4. 触发后不自动重新部署,需手动再按R;保险只在"下线->上线"沿触发一次,不永久禁止部署。

## Grounding (verified)
- 底盘状态标志: my_main.cpp:150-153 (Chassis_Motor_M1_OK..M4_OK, uint8_t 全局)。
- 状态实时更新: my_main.cpp:1746-1755, CAN2 0x120 帧, 由底盘板发送在线标志位。
- deploy_flag: my_main.cpp:310, 由R键在 my_main.cpp:1068 切换。
- 摩擦轮速度选择: my_main.cpp:423 (deploy_flag||... -> Far 5250), else Near(=3700 已改)。
- 现成的同类保护函数样板: Deploy_Timeout_Check() my_main.cpp:1129-1145 (static局部计时/状态,主循环调用)。
- 主循环 My_Loop(): my_main.cpp:2010-2103, 各Logic依次调用(10ms节拍)。
  MCL_Logic 在 2073, Keyboard_Special_Func 在 2085。

## Design (recommended)
宏开关 (放在文件顶部 #define 区,和 CHASSIS_M1_OK_FLAG 等宏一起, ~my_main.cpp:166 区域):
```
#define CHASSIS_SAFE_ENABLE 1   // 1=启用底盘保险  0=关闭(整段功能不编译)
```
新增函数 `Chassis_Safe_Guard()`, 用 #if 包裹, 内部 static 状态机 (类似 Deploy_Timeout_Check):
```
#if CHASSIS_SAFE_ENABLE
void Chassis_Safe_Guard(void) {
  static uint8_t chassis_was_offline = 0;
  if (!deploy_flag) { chassis_was_offline = 0; return; }   // 非部署:不监测
  uint8_t all_online = Chassis_Motor_M1_OK && Chassis_Motor_M2_OK
                    && Chassis_Motor_M3_OK && Chassis_Motor_M4_OK;
  if (!all_online) {
    chassis_was_offline = 1;              // 4个全掉 -> 记住下线过
  } else if (chassis_was_offline) {       // 曾下线 且 现已全上线 -> 上升沿
    deploy_flag = 0;                      // 退出部署 -> 摩擦轮回 Near(3700)
    chassis_was_offline = 0;
  }
}
#endif
```
- 宏定义放文件顶部 #define 区 (~my_main.cpp:166,和 CHASSIS_*_OK_FLAG 一起)。
- 函数定义 (含 #if/#endif) 放在 Deploy_Timeout_Check 之后 (~my_main.cpp:1145)。
- 调用点:My_Loop 里 MCL_Logic 之前(2072 行前),同样用 #if 包裹:
  ```
  #if CHASSIS_SAFE_ENABLE
      Chassis_Safe_Guard();
  #endif
  ```
  确保退出部署当拍就生效。放主循环10ms节拍足够,底盘状态本就慢速更新。

## Verification
- 编译两遍验证:CHASSIS_SAFE_ENABLE=1 和 =0 都要能干净编译(项目工具链 CMake preset 或 MDK-ARM)。
- 静态确认:=0 时函数与调用都不进固件(等同注释掉,零开销);
  =1 且部署时才监测;仅"4全掉->4全上"沿触发一次 deploy_flag=0;之后需手动按R重进。
- 无硬件在环,agent-executed QA = 双配置 build + 逻辑断言。

## Adopted defaults
- 宏默认值定为 1 (启用);用户改 0 重编译即关闭。
- 监测节拍走主循环10ms,不进中断。
- 不动 Deploy_Timeout_Check 的 deploy_flag==2 超时逻辑;二者独立共存。
