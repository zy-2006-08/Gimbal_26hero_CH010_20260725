#ifndef GIMBAL_ZHOU_H
#define GIMBAL_ZHOU_H

/*
 * Gimbal_Zhou - 云台单轴 GYRO 模式双环 PID 公版类
 *
 * 把 YAW / PITCH 两根轴在 GYRO 模式下的结构打包:
 *   外环(角度环) -> 内环(角速度环) -> 输出符号
 * 支持"自瞄 / 非自瞄"双组 PID 切换,并在切换时清对方组积分(与原内联逻辑一致)。
 *
 * 仅覆盖 GYRO 模式。MANG / PROTECT 分支仍留在应用层原样处理(不进本类)。
 *
 * 两轴差异全部由配置体现:
 *   - 外环角度反馈来源指针 fankui_wai_angle (YAW=&continuous_yaw, PITCH=&hi91.pitch)
 *   - 内环角速度反馈来源指针 fankui_gyro       (YAW=&gyr[2],        PITCH=&gyr[0])
 *   - 内环是否用低通 nei_use_lp                (YAW=true 带LP,     PITCH=false 不带LP)
 *   - 外环/内环 LP 系数 lp_wai / lp_nei
 *   - 输出符号 output_sign                     (两轴现状均 -1)
 *
 * 逻辑与原 YAW_PID_Calc / PITCH_PID_Calc 的 GYRO 分支逐行等价,不改任何计算。
 */

#ifdef __cplusplus

/* 前向声明:本头只用 PID_class 指针,无需完整定义,避免重复 include RM_Lib.h。
 * 成员函数实现在 RM_Lib.cpp(那里已 include RM_Lib.h,可见完整 PID_class)。 */
class PID_class;

class Gimbal_Zhou
{
  public:
    /* 双组 PID(外/内 常规 + 外/内 自瞄),由应用层指向已存在的全局 PID 对象 */
    PID_class *pid_wai;      // 常规 外环(角度)
    PID_class *pid_nei;      // 常规 内环(角速度)
    PID_class *pid_wai_zm;   // 自瞄 外环
    PID_class *pid_nei_zm;   // 自瞄 内环

    /* 反馈来源(指向实时更新的全局量) */
    float *fankui_wai_angle; // 外环角度反馈 (YAW=continuous_yaw, PITCH=hi91.pitch)
    float *fankui_gyro;      // 内环角速度反馈 (YAW=gyr[2], PITCH=gyr[0])

    /* 配置 */
    float output_sign;       // 输出符号,两轴现状均 -1
    bool  nei_use_lp;        // 内环是否用 PID_update_LP(YAW=true) 或 PID_update(PITCH=false)
    float lp_wai;            // 外环 LP 系数
    float lp_nei;            // 内环 LP 系数(仅 nei_use_lp=true 时使用)

    /* 状态 */
    float goal;              // 目标(角度),由应用层每帧写入
    float output;            // 计算结果,应用层读取

    Gimbal_Zhou() : pid_wai(0), pid_nei(0), pid_wai_zm(0), pid_nei_zm(0),
                    fankui_wai_angle(0), fankui_gyro(0),
                    output_sign(-1.0f), nei_use_lp(true), lp_wai(1.0f), lp_nei(1.0f),
                    goal(0.0f), output(0.0f) {}

    /* GYRO 模式一帧计算:zimiao=true 走自瞄组,否则走常规组。
     * 与原内联逻辑一致:进入某组前先清对方组的 Integral/OUT_I。 */
    void gyro_update(bool zimiao);
};

#endif /* __cplusplus */

#endif /* GIMBAL_ZHOU_H */
