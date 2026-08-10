/*
 * my_main.cpp - 应用层业务实现(薄壳化)
 *
 * 承载原 main.c 里的全部 C++ 业务:全局对象、业务函数、中断回调,
 * 并对外提供 My_Setup()/My_Loop() 两个 extern "C" 入口给 main.c 调用。
 *
 * 分阶段搬运中:当前为骨架阶段,My_Setup/My_Loop 暂为空壳,
 * 业务仍在 main.c,先让链接跑通、行为零变化。后续逐批把业务搬入本文件。
 */
#include "my_main.h"
#include "main.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "RM_Lib.h"
#include "communication.h"
#include "hipnuc_dec.h"
#include "CP_System.h"
#include "rgb_debug.h"
#include "stm32f4xx_it.h"
extern Yaw_Continuous_t yaw_cont;
extern float Yaw_Continuous_Update(float raw_yaw);
extern void Yaw_Continuous_Reset(void);
extern float Get_Continuous_Yaw(void);
#define MCL_KP 20.0F

/********   控制模式   **********/
#define PROTECT_MODE 0
#define GYRO_MODE 2
#define MANG_MODE 1
#define SPEED_MODE 3
#define DUZHUAN_MODE 5 // 2006上电堵转记位置
#define DISABLE_MODE 0
#define ENABLE_MODE 4
#define ZM_MODE 5
#define ZM_MODE_AUTO 6

#define DAN_YUN_TAI_MODE 1
#define SHANG_XIA_MODE 2
#define DI_PAN_L_MODE 3
#define SHUANG_ZHONG_MODE 4
#define DIAO_SHE_MODE 5
#define DI_PAN_H_MODE 6
#define XIAO_TUO_LUO_MODE 7
#define ZHAN_DOU_MODE 8

uint8_t MYmode = PROTECT_MODE;

/********   电机   **********/
USER_CAN CAN_1(&hcan1, 0), CAN_2(&hcan2, 1);
MOTOR_RM  BoPan(0x201, &CAN_2);
MOTOR_RM  Mini_Pitch_2006(0x207, &CAN_1); 

MOTOR_RM  Motor_MCL_UP_up(0x202, &CAN_1), 
					Motor_MCL_R(0x203, &CAN_1),         
					Motor_MCL_L(0x201, &CAN_1),       
					Motor_MCL_RR(0x206, &CAN_1),  
					Motor_MCL_UP(0x205, &CAN_1),        
					Motor_MCL_LL(0x204, &CAN_1);       

MOTOR_LK Motor_LK6010_Pitch(0X141, &CAN_2), Motor_LK6010_Yaw(0x142, &CAN_2);

PID_class PID_Mini_Pitch_2006_mang(0.6, 0, 0, 30000, 0, 30000, 16000, 0,0), // 0.6, 0, 0, 30000, 0, 30000, 16000, 0,0                       
          PID_Mini_Pitch_2006_sp(0.6, 0, 0, 30000, 0, 0, 10000, 0, 0); // 0.6, 0, 0, 30000, 0, 0, 10000, 0, 0

PID_class PID_BP_mang(0.6f, 0, 0, 30000, 0, 0, 30000, 0,0), // 0.6f, 0, 0, 30000, 0, 0, 30000, 0,0
          PID_BP_sp(20, 0, 0, 16000, 0, 0, 16000, 0, 0); // 20, 0, 0, 16000, 0, 0, 16000, 0, 0
          
PID_class PID_MCL_UP_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0), 
          PID_MCL_R_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0),
          PID_MCL_L_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0,0), 
          PID_MCL_RR_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0),  
          PID_MCL_UPUP_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0), 
          PID_MCL_LL_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0);  
    
PID_class PID_LK_Pitch_Mang(12, 0, 0, 240, 1, 1, 240, 250, 50), //18, 0, 0, 240, 1, 1, 240, 250, 50
          PID_LK_Pitch_SP(4.5, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
          PID_LK_Pitch_Mang_zm(13, 0.03, 0, 240, 100, 1, 240, 0.12, 50), //13, 0.03, 0, 240, 100, 1, 240, 0.12, 50
          PID_LK_Pitch_SP_zm(8, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
          PID_LK_Erro_Pitch_IMU_MANG(12, 0.12, 0, 240, 500, 1, 240, 0.1, 50), //25, 0.15, 0, 240, 500, 1, 240, 0.1, 50
          PID_LK_Erro_Pitch_IMU_Gyro(5, 0, 0, 600, 0, 0, 600, 0, 0); //8, 0, 0, 600, 0, 0, 600, 0, 0

PID_class PID_Yaw_mang(12, 0, 0, 500, 100, 0, 500, 50, 0), //7, 0, 0, 500, 100, 0, 500, 50, 0
          PID_Yaw_sp(4, 0, 0, 400, 0, 0, 500, 0,0),   //3.5, 0, 0, 400, 0, 0, 500, 0,0
          PID_Yaw_mang_zm(13.5, 0.05, 0, 500, 100, 0, 500, 0.12, 50), //13.5, 0.05, 0, 500, 100, 0, 500, 0.12, 50
          PID_Yaw_sp_zm(7, 0, 0, 400, 0, 0, 500, 0,0),               // 7, 0, 0, 400, 0, 0, 500, 0,0                                                          
          PID_YAW_Erro_IMU_MANG(10, 0.11, 0, 500, 100, 0, 500, 0.1,50), //13, 0.07, 200, 500, 100, 0, 500, 0.1,50
          PID_YAW_Erro_IMU_GYRO(3.5, 0, 0, 400, 0, 0, 500, 0,0); // 9, 0, 0, 400, 0, 0, 500, 0,0

// 云台单轴双环 PID(Gimbal_Zhou):配置一行到位,风格同 MOTOR_RM。
//   参数序: GYRO常规组(外,内) GYRO自瞄组(外,内) MANG组(外,内) 外环角度反馈 内环角速度反馈 符号 GYRO内环是否LP
//   YAW  : GYRO内环带LP(true), 外环反馈 continuous_yaw, 内环反馈 gyr[2], 符号 -1
//   PITCH: GYRO内环不带LP(false),外环反馈 hi91.pitch,   内环反馈 gyr[0], 符号 -1 (重力补偿保持关闭)
Gimbal_Zhou Yaw_calc(&PID_Yaw_mang, &PID_Yaw_sp, &PID_Yaw_mang_zm, &PID_Yaw_sp_zm,
                     &PID_YAW_Erro_IMU_MANG, &PID_YAW_Erro_IMU_GYRO,
                     &yaw_cont.continuous_yaw, &hipnuc_raw.hi91.gyr[2], -1.0f, true,
                     1.0f, 1.0f, 1.0f, &hipnuc_raw.hi91.gyr[2]); // MANG 内环用差分角速度(带死区滤波)

Gimbal_Zhou Pitch_calc(&PID_LK_Pitch_Mang, &PID_LK_Pitch_SP, &PID_LK_Pitch_Mang_zm, &PID_LK_Pitch_SP_zm,
                       &PID_LK_Erro_Pitch_IMU_MANG, &PID_LK_Erro_Pitch_IMU_Gyro,
                       &hipnuc_raw.hi91.pitch, &hipnuc_raw.hi91.gyr[0], -1.0f, false);

BMI088 Pitch_088(&hspi1, &htim12, GPIOC, GPIO_PIN_4, GPIOA, GPIO_PIN_4, 800, 0.0f, BMI088_GYRO_RANGE_2000, BMI088_ACC_RANGE_3, "put the Update function in 400Hz interrupt", 1);
RC YK(&huart5, &huart6);
extern DMA_HandleTypeDef hdma_usart6_rx; 
TuChuan TC(&huart6, &hdma_usart6_rx);
RGB_UI RGB_UI(&htim3,TIM_CHANNEL_3,"put the Update function in 800kHz interrupt");

//RGB
#define RGB_goal 225  // WS2812亮度值
// 超时计数器（用于设备故障检测，50Hz下10次=200ms）
uint8_t timeout_pitch = 0;
uint8_t timeout_yaw = 0;
uint8_t timeout_bopan = 0;
uint8_t timeout_mcl_up = 0;   
uint8_t timeout_mcl_r = 0;     
uint8_t timeout_mcl_l = 0;      
uint8_t timeout_mcl_rr = 0;    
uint8_t timeout_mcl_ll = 0;    
uint8_t timeout_mcl_upup = 0;  
uint8_t timeout_imu = 0;     
uint8_t timeout_mini_pitch = 0; 

// 底盘设备状态标志位（从底盘板通过CAN接收）
uint8_t Chassis_Motor_M1_OK = 0;   // 底盘电机M1状态（0x201）
uint8_t Chassis_Motor_M2_OK = 0;   // 底盘电机M2状态（0x202）
uint8_t Chassis_Motor_M3_OK = 0;   // 底盘电机M3状态（0x203）
uint8_t Chassis_Motor_M4_OK = 0;   // 底盘电机M4状态（0x204）
uint8_t Chassis_IMU_OK = 0;        // 底盘IMU状态
uint8_t Chassis_REF_OK = 0;        // 裁判系统连接状态

// RGB Debug API 配置变量
RGB_Debug_Config_t rgb_debug_config;
LED_Mapping_t led_mappings[5];

// 超电电压相关
float V_Cap_Real = 0.0f;           // 超电电压实际值
uint8_t V_Cap_OK_flag = 0;         // 超电电压检查标志（0=故障，非0=正常）

// 底盘设备状态标志位定义(扩填进 DP_Tx_static_Flag, 用于 CAN 通信)
#define CHASSIS_M1_OK_FLAG    ((uint16_t)0x0001 << 5)   // 底盘电机 0x201
#define CHASSIS_M2_OK_FLAG    ((uint16_t)0x0001 << 6)   // 底盘电机 0x202
#define CHASSIS_M3_OK_FLAG    ((uint16_t)0x0001 << 7)   // 底盘电机 0x203
#define CHASSIS_M4_OK_FLAG    ((uint16_t)0x0001 << 8)   // 底盘电机 0x204
#define CHASSIS_IMU_OK_FLAG   ((uint16_t)0x0001 << 9)   // 底盘 IMU
#define CHASSIS_CAP_OK_FLAG   ((uint16_t)0x0001 << 10)  // 超级电容
#define CHASSIS_REF_OK_FLAG   ((uint16_t)0x0001 << 11)  // 裁判系统

// 底盘保险开关: 1=启用 0=关闭(整段功能不编译, 等同注释掉)
// 生效时: 部署模式(按R,5250)下若底盘4个电机全部掉线, 待其全部恢复上线自动退出部署 -> 摩擦轮回落 Near(3700)
#define CHASSIS_SAFE_ENABLE 1

/********  通信标志  **********/
uint16_t YT_Tx_static_Flag = 0, // 云台发送静态标志位 bool
         DP_Tx_static_Flag = 0; // 底盘发送静态标志位 bool  0=关闭  1=开启底盘跟随
bool turn_flag = 0;
#define TURN_FLAG ((uint16_t)0x0001 << 0)
uint16_t mine_flag = 0;
#define MINE_FLAG ((uint16_t)0x0001 << 1)
bool dp_follow_flag = 0;
#define HEAT_FLAG ((uint16_t)0x0001 << 2)
bool heat_flag = 0;
#define BOOST_FLAG ((uint16_t)0x0001 << 3)
uint8_t boost_flag = 0;
#define SP_TURN_FLAG ((uint16_t)0x0001 << 4)
#define SHOOT_FLAG ((uint16_t)0x0001 << 5)
bool shoot_once_flag = 0;

uint8_t jianshu_ctrl_flag = DISABLE_MODE;

//BMI088状态
uint8_t Pitch_088_state = BMI088_ERROR;

// 用于判断电机是否都接收到数据，用于控制发送频率
uint8_t MCL_4_motorflag = 0, MCL_2_motorflag = 0;
uint8_t can2_bjtx010 = 0, can2_bjtx011 = 0, can2bjtx013 = 0, can2bjtx014 = 0;
uint8_t BP_protect_cansend_flag = 0, MCL_protect_cansend_200_flag = 0, MCL_protect_cansend_1FF_flag = 0;

// 拨盘相关
union
{
  float f_speed;
  uint16_t u16[2];
  uint8_t c[4];
} Shoot_speed_u;
float shoot_sp = 11.9;
int16_t L_targe_sp, R_targe_sp, UP_targe_sp, RR_targe_sp, LL_targe_sp, UPUP_targe_sp;
uint8_t MCL_Start_flag = 0;
int16_t MCL_MID;                                                                       // 摩擦轮平均值
int16_t MCL_MAX_Speed_Near = 3685, MCL_MAX_Speed_Far = 5070, MCL_MAX_Speed_Now = 3700; // 5170，3750,3700，5200，5170，5070，5090
//3650，5070
uint8_t MCL_ON_flag = 0;

// 拨盘
#define int_shot 26219
uint8_t BP_MODE = PROTECT_MODE;
int32_t BP_targe = 0, BP_calc_targe = 0;
uint8_t BP_ON_flag = 0;
bool Shoot_flag = 0; // 射击标志，用于控制拨盘转动为0
uint16_t Shoot_time = 0;
float BP_Islow_incbuf = 0;
int32_t targe_inc;
UpDown_check_class UD_BoPan_ON(0), UD_BP_1(0);
int16_t BP_output = 0;

// Mini_Pitch_2006控制变量
uint8_t Mini_Pitch_MODE = PROTECT_MODE;
int32_t Mini_Pitch_targe = 0;              // 当前目标位置
int32_t Mini_Pitch_calc_targe = 0;         // 计算目标位置（用于平滑过渡）
uint8_t Mini_Pitch_preset_flag = 0;        // 预置触发标志
float Mini_Pitch_pwm_to_mang_ratio = 1.0f; // PWM到角度的映射比例（可调）
float Mini_Pitch_manual_speed = 5.0f;     // 手动控制速度（用于调参模式）
float Mini_Pitch_Islow_incbuf = 0;      
uint8_t Mini_Pitch_first_enter_diaoshe = 0; // 首次进入吊射模式标志
uint16_t Mini_Pitch_duzhuan_cnt = 0;
uint8_t Mini_Pitch_E_key_flag = 0;

// YAW
#define REAL_YAW_REF yaw_cont.continuous_yaw
// #define REAL_YAW_REF Pitch_088.realAngle.yaw
const int YT_Erro = 44480;
#define Yaw_Mouse_Speed 700.0f // Yaw鼠标速度
#define Yaw_YK_GYRO_Speed 6000.0f
#define Yaw_YK_MANG_Speed 28000.0f
#define YAW_LEVEL_MANG 28660
#define YAW_LEVEL_MANG_2 28600 + 32768
float YAW_PID_OUT = 0;                   // PID输出，用于发送CAN数据
float Yaw_goal = 0;
uint8_t yaw_control_mode = PROTECT_MODE; // 控制模式，单位度
float vision_yaw = -2.9;

// PITCH
#define REAL_PITCH_REF hipnuc_raw.hi91.pitch
// #define REAL_PITCH_REF Pitch_088.realAngle.roll
#define HIGH_PITCH_Mang 43075
#define LOW_PITCH_Mang 53945
// #define PITCH_LEVEL_MANG 50630
#define Pitch_Mouse_Speed 1500
#define Pitch_YK_Speed 2500.0f
#define Pitch_YK_MANG_Speed 8000.0f
#define PITCH_LEVEL_MANG 50630
#define DIAO_SHE_ENCODER_OFFSET  29000   // 吊射模式目标编码器值（相对堵转零点，可调）
#define DUZHUAN_DIANLIU      (-3000)
#define DUZHUAN_SUDU_YUZHI   30
#define DUZHUAN_QUEREN_CISHU 400

float PITCH_PID_OUT = 0;                   // PID 输出, 用于发送 CAN 数据
float Pitch_goal = 0;                    
uint8_t pitch_control_mode = PROTECT_MODE;         
float vision_pitch = 34.9;
float vision_distance = 0;
// 辅助吊射 pitch 一键到位(按R触发缓动到 -vision_pitch, 操作手动摇杆则中断交手动)
uint8_t diaoshe_pitch_arr_flag = 0;   // 1=缓动进行中
float   diaoshe_pitch_incbuf = 0.0f;

// 坐标变换
float theta_angle = 0, theta_rad = 0;
float sin_theta = 0, cos_theta = 0;

// 吊射
uint8_t deploy_flag = 0; // 0=关闭 1=展开模式 2=展开旋转
int32_t mini_pitch_deploy_offset = 0;

// 上台阶标志位（来自底盘板 0x012 bit5），用于上台阶模式下让2006电机保持不动
uint8_t Shangtaijie_flag = 0;

// 射速改变
uint8_t SP_Turn_Flag = 0;

// 自瞄
int16_t ZM_Fire_delay = 834;
float Yaw_ZM,Pitch_ZM;
uint8_t mouse_press_type = 0;          // 0=什么不做 1=单击长按 2=双击
uint16_t mouse_press_counter = 0;
uint8_t aim_mode = 0x00;
uint8_t is_mouse_single_clicked = 0;

// 望远镜
uint8_t telescope_ON = 0;
// 舵机确认动作相关变量
uint8_t servo_confirm_flag = 0;
uint16_t servo_confirm_timer = 0;
uint16_t servo_original_ccr = 0;

//杆子舵机
#define DEPLOY_SERVO_CCR_PARK  3450 
#define DEPLOY_SERVO_CCR_INIT  3450  
#define DEPLOY_SERVO_CCR_MIN   3315  
#define DEPLOY_SERVO_CCR_MAX   3615   
#define DEPLOY_SERVO_STEP      1                                                           
// 当前部署舵机脉宽; 持久保存, 退出部署时不复位，下次进入部署记住上一次位置
uint16_t deploy_servo_ccr = DEPLOY_SERVO_CCR_INIT;
uint16_t servo2_ccr = DEPLOY_SERVO_CCR_PARK;

// 拨盘相关
union
{
  float f_pos;
  uint16_t u16[2];
  uint8_t c[4];
} Robot_pos_u;

// 自瞄相关定义
uint8_t PRESS_TYPE = 0;
#define NO_ZM_MODE 0
#define OWN_FIRE_MODE 1
#define ZM_FIRE_MODE 2

//2006 
int16_t Mini_Pitch_output = 0; 

//调试
uint32_t counter;
uint8_t XTL_flag = 0;
uint8_t p_cnt, y_cnt, bp_cnt, mcl_cnt, zm_cnt, js_cnt,bj,imu_cnt,zm_press1,zm_press2,m2006_cnt,duoji;

// 模式
void MYMODE_while_application_layer(void)
{
  if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_MID) // 单云台，底盘跟随(不动)
  {
    MYmode = DAN_YUN_TAI_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_DOWN) // 上下模式
  {
    MYmode = SHANG_XIA_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_UP) //单底盘
  {
    MYmode = DI_PAN_L_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_MID) // 双中，底盘跟随
  {
    MYmode = SHUANG_ZHONG_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_DOWN) // 吊射（上台阶）
  {
    MYmode = DIAO_SHE_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_UP) // 单底盘
  {
    MYmode = DI_PAN_H_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_MID) // 小陀螺
  {
    MYmode = XIAO_TUO_LUO_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_DOWN) // 键鼠战斗
  {
    MYmode = ZHAN_DOU_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_UP) // 双上保护
  {
    MYmode = PROTECT_MODE;
  }
  else 
  {
    MYmode = PROTECT_MODE;
  }
}
// 摩擦轮逻辑
uint8_t MCL_Logic()
{
  static int16_t MCL_slow_speed = 0;
  static int16_t Buf_MCL_Speed = 0;
  static float MCL_I16slow_incbuf = 0;
  static UpDown_check_class UD_MCL(0), UD_C(0), UD_X(0), UD_TURN(0);
  static uint8_t now_state;
  static uint8_t last_mymode = 0xFF;
  static uint8_t last_deploy = 0xFF;
  //重置射速
  if (MYmode != last_mymode || deploy_flag != last_deploy)
  {
    if (MYmode == DIAO_SHE_MODE || deploy_flag || MYmode == SHANG_XIA_MODE)
      MCL_MAX_Speed_Now = MCL_MAX_Speed_Far;   // 吊射/上下/双下已部署: 默认远距
    else if (MYmode != PROTECT_MODE)
      MCL_MAX_Speed_Now = MCL_MAX_Speed_Near;  // 其它模式(含双下未部署): 默认近距
    last_mymode = MYmode;
    last_deploy = deploy_flag;
  }
  // 遥控变速: 仅摩擦轮未开(MCL_ON_flag != 1)时, 摇杆手势上升沿在 Far/Near 间切换
  if (MYmode == PROTECT_MODE)
  {
    now_state = (YK.yaogan.ch2 < -500 && YK.yaogan.ch3 > 500);
  }
  else if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
  {
    MCL_ON_flag = (YK.yaogan.ch1 > 600);
    now_state = (YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500);
  }
  else if (MYmode == ZHAN_DOU_MODE)
  {
    now_state = (YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500);
  }
  else
  {
    now_state = 0;
  }
  if (UD_TURN.updata(now_state) == UpDown_check_rising && MCL_ON_flag != 1)
  {
    MCL_MAX_Speed_Now = (MCL_MAX_Speed_Now == MCL_MAX_Speed_Far) ? MCL_MAX_Speed_Near : MCL_MAX_Speed_Far;
  }
  // 键盘控制摩擦轮
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    if (UD_MCL.updata(YK.Pressed_Check(KEY_PRESSED_B)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      MCL_ON_flag = !MCL_ON_flag;
    if (UD_C.updata(YK.Pressed_Check(KEY_PRESSED_C)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed += 50;
    else if (UD_X.updata(YK.Pressed_Check(KEY_PRESSED_X)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed -= 50;
  }
  //摩擦轮保护
  if (MYmode == PROTECT_MODE)
  {
    MCL_protect_cansend_200_flag = 1;
    MCL_4_motorflag = 0; 
    MCL_2_motorflag = 0;
    MCL_Start_flag = 0;
    MCL_ON_flag = 0; 
    MCL_MID = 0;

    L_targe_sp = 0;
    R_targe_sp = 0;
    UP_targe_sp = 0;
    RR_targe_sp = 0;
    LL_targe_sp = 0;
    UPUP_targe_sp = 0;
    MCL_slow_speed = 0;  
    MCL_I16slow_incbuf = 0;  
  }
  else
  {
    MCL_Start_flag = 1;
  }
  // 计算目标速度
  int16_t MCL_targe_sp = MCL_ON_flag ? (MCL_MAX_Speed_Now + Buf_MCL_Speed) : 0;
  I16_slow(&MCL_slow_speed, MCL_targe_sp, 200, 200, 300, &MCL_I16slow_incbuf);

  // 设置各电机目标
  if (MCL_ON_flag)   
  {
    if (MCL_MAX_Speed_Now == MCL_MAX_Speed_Far)
    {
      // 远距: 差速(一级比二级低1150)
      L_targe_sp = MCL_slow_speed - 1150;//一级
      R_targe_sp = MCL_slow_speed - 1150;//一级
      UP_targe_sp = MCL_slow_speed;//二级
      RR_targe_sp = MCL_slow_speed;//二级
      LL_targe_sp = MCL_slow_speed;//二级
      UPUP_targe_sp = MCL_slow_speed - 1150;//一级
    }
    else
    {
      // 近距: 不差速, 六轮同转速
      L_targe_sp = MCL_slow_speed;
      R_targe_sp = MCL_slow_speed;
      UP_targe_sp = MCL_slow_speed;
      RR_targe_sp = MCL_slow_speed;
      LL_targe_sp = MCL_slow_speed;
      UPUP_targe_sp = MCL_slow_speed;
    }
  }
  else // 摩擦轮未开(含保护模式): 六个目标全部归零, 缓变速度清零
  {
    L_targe_sp = 0;
    R_targe_sp = 0;
    UP_targe_sp = 0;
    RR_targe_sp = 0;
    LL_targe_sp = 0;
    UPUP_targe_sp = 0;
    MCL_slow_speed = 0;
  }

  return MCL_ON_flag;
}
// 拨盘逻辑
void BP_Logic(uint8_t MCL_ON_flag)
{
  static int32_t BP_adjest_mang = 0;
  // 模式切换
  if (MCL_ON_flag)
  {
    BP_MODE = MANG_MODE;
  }
  else if (MYmode == ZHAN_DOU_MODE || MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
  {
    BP_MODE = SPEED_MODE;
    BP_targe = BoPan.mang_inf;
    BP_calc_targe = BoPan.mang_inf;
    BP_ON_flag = 0;
  }
  else
  {
    BP_MODE = PROTECT_MODE;
    BP_targe = BoPan.mang_inf;
    BP_calc_targe = BoPan.mang_inf;
    BP_ON_flag = 0;
  }
  // 吊射模式拨盘归零
  if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
  {
    if (YK.yaogan.ch1 < -500 && YK.yaogan.ch0 < -500 && BP_MODE != MANG_MODE)
    {
      BP_adjest_mang = BoPan.mang;
      BoPan.first = 0;
      BP_targe = 0;
      BP_calc_targe = 0;
      BoPan.mang_inf = 0;
    }
    if (YK.yaogan.ch0 > 600)
    {
      BP_ON_flag = 1;
    }
    else
    {
      BP_ON_flag = 0;
    }
  }
  // 鼠标射击
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE)
  {
    UpDown_check_state UD_BoPan_ON_buf;
    UD_BoPan_ON_buf = UD_BoPan_ON.updata(YK.shubiao.press_l);
    if (UD_BoPan_ON_buf == UpDown_check_rising && MCL_ON_flag && mouse_press_type != 2) // 鼠标左键按下且摩擦轮开
    {
      Shoot_flag = 1;
      Shoot_time = 0; 
    }
    else if (UD_BoPan_ON_buf == UpDown_check_falling) // 松开
    {
      Shoot_time = 0; 
    }
  }
  if (BP_MODE == MANG_MODE)
  {
    if (UD_BP_1.updata(BP_ON_flag) == UpDown_check_rising)
    {

      Shoot_flag = 1;
    }
    int32_t BP_inc = BoPan.mang_inf % int_shot;
    int bp_err_test = BP_calc_targe - BoPan.mang_inf;

    if (Shoot_flag && (abs(bp_err_test) < 2500) && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL)))
  // if (Shoot_flag && (abs(bp_err_test) < 2500))
    {
      targe_inc = (BP_inc < -20000) ? -((int_shot + BP_inc) + int_shot) : -(int_shot + BP_inc);
      BP_calc_targe = BoPan.mang_inf + targe_inc;
      shoot_once_flag = 1;
    }
    Shoot_flag = 0;
  }
}
// 拨盘卡弹
void BP_KD_TIM(void)
{
  static uint16_t kadan_tui_mang = 6000;
  static int16_t Kadan_cnt = 0; 
  static uint16_t kadan_tim = 0;

  if (BP_MODE == MANG_MODE)
  {
    if (PID_BP_sp.OUT_PID < -14000)
    {
      kadan_tim++;
      if (kadan_tim > 500)
      {
        BP_calc_targe = BoPan.mang_inf;
        BP_targe = BoPan.mang_inf;
        BP_calc_targe += kadan_tui_mang;
        Kadan_cnt++;
        kadan_tim = 0;
      }
    }
    else
    {
      kadan_tim = 0;
    }
  }
}
// Mini_Pitch_2006控制逻辑
void Mini_Pitch_2006_Logic()
{
  static UpDown_check_class UD_E(0);
  static uint8_t shangtaijie_latched = 0;  // 上台阶锁位标志：进入时锁定一次目标
  if (UD_E.updata(YK.Pressed_Check(KEY_PRESSED_E)) == UpDown_check_rising)
    Mini_Pitch_E_key_flag = !Mini_Pitch_E_key_flag;

  if (Mini_Pitch_MODE == DUZHUAN_MODE)
  {
    if (Mini_Pitch_duzhuan_cnt >= DUZHUAN_QUEREN_CISHU)
    {
      Mini_Pitch_2006.first = 0;
      Mini_Pitch_targe = 0;
      Mini_Pitch_calc_targe = 0;
      Mini_Pitch_duzhuan_cnt = 0;
      Mini_Pitch_MODE = MANG_MODE;
    }
    return;
  }
  //上台阶锁2006
  if (Shangtaijie_flag)
  {
    if (!shangtaijie_latched)
    {
      Mini_Pitch_targe = Mini_Pitch_2006.mang_inf; 
      Mini_Pitch_calc_targe = Mini_Pitch_targe;
      shangtaijie_latched = 1;
    }
    Mini_Pitch_MODE = MANG_MODE; 
    return;
  }
  shangtaijie_latched = 0;  // 非上台阶：清除锁定，下次进入时重新锁位

  if (MYmode == DI_PAN_H_MODE)
  {
    Mini_Pitch_MODE = MANG_MODE;
    // m2006_cnt++;
    Mini_Pitch_targe += (float)(YK.yaogan.ch3) / Mini_Pitch_manual_speed;
  }
  else if (MYmode == DIAO_SHE_MODE || deploy_flag || MYmode == SHANG_XIA_MODE)  
  {
    Mini_Pitch_MODE = MANG_MODE;
    Mini_Pitch_calc_targe = DIAO_SHE_ENCODER_OFFSET;
    if (Mini_Pitch_first_enter_diaoshe == 0)
      Mini_Pitch_first_enter_diaoshe = 1;
    I_slow_ease(&Mini_Pitch_targe, Mini_Pitch_calc_targe, 900, 20, 0.15f, 5, &Mini_Pitch_Islow_incbuf);
  }
  else if (MYmode == SHUANG_ZHONG_MODE || MYmode == ZHAN_DOU_MODE || MYmode == XIAO_TUO_LUO_MODE || MYmode == DAN_YUN_TAI_MODE )
  {
    Mini_Pitch_MODE = MANG_MODE;
    Mini_Pitch_calc_targe = 0;
    Mini_Pitch_first_enter_diaoshe = 0; 
    I_slow(&Mini_Pitch_targe, Mini_Pitch_calc_targe, 900, 900, 50, &Mini_Pitch_Islow_incbuf);
  }
  else
  {
    // 预置触发：保护模式 + 右摇杆右下（手动）
    if (YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500 && Mini_Pitch_MODE != MANG_MODE)
    {
      Mini_Pitch_2006.first = 0;
      Mini_Pitch_2006.mang_inf = 0;
      Mini_Pitch_targe = 0;
      Mini_Pitch_calc_targe = 0;
      Mini_Pitch_preset_flag = 1;
    }

    Mini_Pitch_MODE = PROTECT_MODE;
    Mini_Pitch_first_enter_diaoshe = 0;
    Mini_Pitch_targe = Mini_Pitch_2006.mang_inf;
  }
}
// 板间通信处理 1khz调用
void Communication_boards(void)
{
  /*CAN2双板通信，底盘板通过can接收遥控器数�?,CAN1CAN2发�?�摩擦轮*/
  if (can2_bjtx010) // 发遥控
  {
    CAN_2.Send_RM(0x010, YK.yaogan.ch0, YK.yaogan.ch1, YK.yaogan.ch2, YK.yaogan.ch3);
    can2_bjtx010 = 0;
  }
  else if (can2_bjtx011) // 发键鼠
  {
    CAN_2.Send_RM(0x011, YK.shubiao.x, YK.shubiao.y, YK.jianpan, (uint16_t)(((YK.yaogan.s1 << 4) | (YK.yaogan.s2 << 2) | (YK.shubiao.press_l << 1) | (YK.shubiao.press_r)) & 0x003f));
    can2_bjtx011 = 0;
  }
  else if (can2bjtx013)
  {
    uint16_t yt_flag_with_shoot = YT_Tx_static_Flag;
    if (shoot_once_flag) { yt_flag_with_shoot |= SHOOT_FLAG; shoot_once_flag = 0; }
    CAN_2.Send_RM(0x013, MCL_MID, (int16_t)(hipnuc_raw.hi91.pitch * 100.0f), (int16_t)(hipnuc_raw.hi91.yaw * 100.0f), yt_flag_with_shoot);
    can2bjtx013 = 0;
  }
  else if (can2bjtx014)
  {
    js_cnt++;
    int16_t pitch_data = (int16_t)(vision_pitch * 100.0f);
    int16_t yaw_data = (int16_t)(vision_yaw * 100.0f);
    int16_t distance_data = (int16_t)(vision_distance * 100.0f);
    CAN_2.Send_RM(0x014, pitch_data, yaw_data, distance_data, 0);
    can2bjtx014 = 0;
  }
  else if (BP_protect_cansend_flag)
  {
    CAN_2.Send_RM(0x200, 0, 0, 0, 0); // 保护pitch电机
    BP_protect_cansend_flag = 0;
  }
  /* CAN1摩擦轮发�?*/
  if (MCL_protect_cansend_200_flag)
  {
    CAN_1.Send_RM(0x200, 0, 0, 0, 0); // 保护6个摩擦轮
    MCL_protect_cansend_200_flag = 0;
  }
}
// 定时器12回调
void TIM12_Callback(void)
{
 Pitch_088.analyse();
 timeout_imu = 0;  // BMI088数据更新，清零超时计数
 if (abs(Pitch_088.realAngle.pitch) < 2.0f)
  {
   Pitch_088.realAngle.pitch = 0;
  }
}
// Mini PITCH舵机获取PWM
uint16_t Mini_PITCH_Get_PWM()
{
  static uint8_t mini_pitch_ctrl_flag = 0;
  static uint8_t mini_pitch_anjian_flag = 0;
  static float mini_pitch_buf_ccr = 0;
  static uint16_t mini_pitch_ccr = 2700;
  static const uint16_t mini_pitch_downmang = 1800;

  // 控制模式处理
  if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE || deploy_flag)
  {
    mini_pitch_ctrl_flag = 1;
  }
  else if (mini_pitch_anjian_flag)
  {
    mini_pitch_ctrl_flag = 2;
  }
  else
  {
    mini_pitch_ctrl_flag = 0;
  }
  //保护模式切换摩擦轮�?�度
  if (MYmode == PROTECT_MODE)
  {
    mini_pitch_ccr = 2700;
  }
  else if (mini_pitch_ctrl_flag == 1)
  {
    if (Motor_LK6010_Pitch.mang > PITCH_LEVEL_MANG)
    {
      mini_pitch_ccr = 1800;
    }
    else
    {
      float mini_pitch_angle = (PITCH_LEVEL_MANG - Motor_LK6010_Pitch.mang) * 360.0f / 65536.0f;
      uint16_t mini_pitch_auto_ccr = (uint16_t)(mini_pitch_angle * mini_pitch_angle * -0.7781f + mini_pitch_angle * 80.7714f + 130.9925) + 1800;
      mini_pitch_ccr = mini_pitch_auto_ccr + (int16_t)mini_pitch_buf_ccr;
    }
  }
  else if (mini_pitch_ctrl_flag == 2)
  {
    mini_pitch_ccr = 3000;
  }
  else
  {
    mini_pitch_ccr = mini_pitch_downmang;
  }

  return LIMIT(mini_pitch_ccr, 1800, 4200);
}
// 望远镜舵机获取PWM
uint16_t Telescope_Get_PWM()
{
  static uint16_t telescope_ccr = 0;
  static const uint16_t bj_en_ccr = 3700;
  static UpDown_check_class UD_E(0);

  //保护模式
  if (MYmode == PROTECT_MODE)
  {
    telescope_ccr = 2500;
    telescope_ON = 0;
  }
  else if (telescope_ON)
  {
    duoji++;
    telescope_ccr = bj_en_ccr;
  }
  else
  {
    telescope_ccr = 1300;
  }
  // E键控制望远镜
  if (UD_E.updata(YK.Pressed_Check(KEY_PRESSED_E)) == UpDown_check_rising)
  {
    telescope_ON = !telescope_ON;
  }
  return LIMIT(telescope_ccr, 1150, 4900);
}
//YAW逻辑
void YAW_Logic(void)
{
  static uint8_t now_state_yaw;
  static UpDown_check_class UD_SET2_Y(0),UD_SET_Y(0);

  uint8_t hipnuc_ok = (hipnuc_raw.hi91.tag != 0);
  
  if (hipnuc_ok && (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == XIAO_TUO_LUO_MODE) && deploy_flag != 1)
  // if (hipnuc_ok && (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == XIAO_TUO_LUO_MODE))  
  {
    yaw_control_mode = GYRO_MODE;
  }
  else if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE || (!hipnuc_ok && MYmode == ZHAN_DOU_MODE) || deploy_flag == 1)
  // else if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE || (!hipnuc_ok && MYmode == ZHAN_DOU_MODE))  
  {
    yaw_control_mode = MANG_MODE;
  }
  else
  {
    yaw_control_mode = PROTECT_MODE;
    Yaw_goal = REAL_YAW_REF;  
    YAW_PID_OUT = 0;
  }
}
//YAW计算
void YAW_PID_Calc(void)
{
  static UpDown_check_class UD_TURN_Y(0),UD_SET_Y(0);
  if (yaw_control_mode == GYRO_MODE)
  {
    Yaw_goal -= ((float)YK.yaogan.ch2 / Yaw_YK_GYRO_Speed + (float)LIMIT(YK.shubiao.x, -1000, 1000) / Yaw_Mouse_Speed);
    YAW_PID_OUT = Yaw_calc.Gyro_calc(Yaw_goal, request.zimiao_status);  // GYRO 双环,一行调用
  }
  else if (yaw_control_mode == MANG_MODE)
  {
				Yaw_goal -= ((float)YK.yaogan.ch2 / Yaw_YK_MANG_Speed) + ((YK.Pressed_Check(KEY_PRESSED_D) - YK.Pressed_Check(KEY_PRESSED_A)) * 0.002f);
        YAW_PID_OUT = Yaw_calc.Mang_calc(Yaw_goal);
  }  
  else
  {
    Yaw_goal = yaw_cont.continuous_yaw;
    YAW_PID_OUT = 0;
  }
}
// YAW角度更新
void YAW_Angle_Update(void)
{
  static int yaw_relative_mang = 0;
  Motor_LK6010_Yaw.update_65535mang_inf_basic_zeromang();
  yaw_relative_mang = (int)(Motor_LK6010_Yaw.mang - YT_Erro);
  theta_angle = (float)yaw_relative_mang * 360.0f / 65536.0f;
  theta_rad = theta_angle * 3.14159265f / 180.0f;
  sin_theta = sinf(theta_rad);
  cos_theta = cosf(theta_rad);
}
//PITCH逻辑
void PITCH_Logic(void)
{
  static uint8_t now_state_pitch;
  
  uint8_t hipnuc_ok = (hipnuc_raw.hi91.tag != 0);
  
  if (hipnuc_ok && (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == XIAO_TUO_LUO_MODE) && deploy_flag != 1)
  // if (hipnuc_ok && (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == XIAO_TUO_LUO_MODE))  
  {
    pitch_control_mode = GYRO_MODE;
  }
  else if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE || (!hipnuc_ok && MYmode == ZHAN_DOU_MODE) || deploy_flag == 1)
  // else if (MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE || (!hipnuc_ok && MYmode == ZHAN_DOU_MODE))  
  {
    pitch_control_mode = MANG_MODE;
  }
  else
  {
    pitch_control_mode = PROTECT_MODE;
    Pitch_goal = hipnuc_raw.hi91.pitch;
  }
}
//PITCH计算
void PITCH_PID_Calc(void)
{
  if (pitch_control_mode == GYRO_MODE)
  {
    Pitch_goal -= (float)YK.yaogan.ch3 / Pitch_YK_Speed + ((float)LIMIT(YK.shubiao.y, -500, 500) / Pitch_Mouse_Speed);
    Pitch_goal = LIMIT(Pitch_goal, -44, 16);
    PITCH_PID_OUT = Pitch_calc.Gyro_calc(Pitch_goal, request.zimiao_status);  // GYRO 双环,一行调用
  }
  else if (pitch_control_mode == MANG_MODE)
  {
    //微调
    float ws_pitch = YK.Pressed_Check(KEY_PRESSED_CTRL) ? 0.0f : (YK.Pressed_Check(KEY_PRESSED_W) - YK.Pressed_Check(KEY_PRESSED_S)) * 0.002f;
    uint8_t manual_intervene = (abs(YK.yaogan.ch3) > 3) || YK.Pressed_Check(KEY_PRESSED_W) || YK.Pressed_Check(KEY_PRESSED_S);
    float diaoshe_pitch_target = (vision_pitch >= 33.0f && vision_pitch <= 38.0f) ? -vision_pitch : -35.0f;
    if (diaoshe_pitch_arr_flag && !manual_intervene)
    {
      F_slow_ease(&Pitch_goal, diaoshe_pitch_target, 0.15f, 0.02f, 0.02f, 0.1f, &diaoshe_pitch_incbuf);
      if (fabsf(Pitch_goal - diaoshe_pitch_target) <= 0.1f)
        {diaoshe_pitch_arr_flag = 0;} 
    }
    else
    {
      if (diaoshe_pitch_arr_flag)
        {diaoshe_pitch_arr_flag = 0;} 
      Pitch_goal -= ((float)(YK.yaogan.ch3 / Pitch_YK_MANG_Speed) + ws_pitch);
    }
    Pitch_goal = LIMIT(Pitch_goal, -43, 16);
    PITCH_PID_OUT = Pitch_calc.Mang_calc(Pitch_goal);  // MANG 单组,一行调用
  }
  else
  {
    Pitch_goal = REAL_PITCH_REF;
    PITCH_PID_OUT = 0;
  }
}
// PITCH角度更新
void PITCH_Angle_Update(void) { Motor_LK6010_Pitch.update_65535mang_inf_basic_zeromang(); }
// 掉头处理函数
void YAW_Turn_Handle(void)
{
  static UpDown_check_class UD_DiaoTou_No(0);
  if (UD_DiaoTou_No.updata(turn_flag) == UpDown_check_rising && MYmode == ZHAN_DOU_MODE && !deploy_flag)
  {
    Yaw_goal -= 180.0f;
  }
}
// 舵机PWM输出
void Servo_PWM_Output()
{
  uint16_t telescope_ccr = Telescope_Get_PWM();
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, telescope_ccr);
  // deploy_servo_ccr 退出部署不复位, 下次按R进入恢复上次调好的位置
  if (MYmode == DIAO_SHE_MODE || (MYmode == ZHAN_DOU_MODE && deploy_flag))
    servo2_ccr = deploy_servo_ccr;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, servo2_ccr);
}
// 键盘特殊功能处理
void Keyboard_Special_Func()
{
  static UpDown_check_class UD_XTL(0), UD_R(0), UD_Deploy_Turn(0), UD_Arr_PY(0), UD_Boost(0), UD_MCL_Speed_Turn(0),UD_Z(0),UD_X(0),UD_C(0);
  static UpDown_check_class UD_Diaoshe_Pitch(0);  // 辅助吊射 pitch 一键到位 R 键专用边沿
  static int targe_f = YT_Erro;
  static uint8_t mini_pitch_anjian_flag = 0;

  // 键盘开关
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    jianshu_ctrl_flag = ENABLE_MODE;
  }
  else
  {
    jianshu_ctrl_flag = DISABLE_MODE;
  }
  if (jianshu_ctrl_flag == ENABLE_MODE)
  {
    //防止吊射时误触
    const uint8_t deploy_lock = deploy_flag;
    // Q小陀螺
    if (UD_XTL.updata(YK.Pressed_Check(KEY_PRESSED_Q)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL) && !deploy_lock)
    {
      XTL_flag = !XTL_flag;
      if (XTL_flag)
      {
        telescope_ON = 0;
        mini_pitch_anjian_flag = 0;
        if (deploy_flag)
        {
          Pitch_goal = 0;
          deploy_flag = 0;
        }
      }
    }
    // G爆发
    if (UD_Boost.updata(boost_flag) == UpDown_check_rising && !deploy_lock)
    {
      mini_pitch_anjian_flag = 0;
      Pitch_goal = 0;
      telescope_ON = 0;
    }
    // R吊射
    if (UD_R.updata(YK.Pressed_Check(KEY_PRESSED_R)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
    {
      deploy_flag = !deploy_flag;
      // 进入吊射部署模式时清零小陀螺状态; 退出部署后 XTL_flag 保持为 0(不自动恢复),
      // 因此不会因先前处于小陀螺而在退出部署时重新进入小陀螺。
      if (deploy_flag)
        XTL_flag = 0;
      mini_pitch_anjian_flag = 0;
      diaoshe_pitch_arr_flag = 1;
      diaoshe_pitch_incbuf = 0.0f;      
    }
    // F一键掉头（吊射不触发）
    if (UD_Deploy_Turn.updata(YK.Pressed_Check(KEY_PRESSED_F)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL) && deploy_flag && !deploy_lock)
    {
      if (YT_Erro > 32767)
      {
        targe_f = (cos_theta > 0) ? (YT_Erro - 32768) : YT_Erro;
      }
      else
      {
        targe_f = (cos_theta > 0) ? (YT_Erro + 32768) : YT_Erro;
      }
      Yaw_goal -= 180.0f;
      Pitch_goal = 0;
      deploy_flag = 2;
      mini_pitch_anjian_flag = 0;
      telescope_ON = 0;
    }
    // 检测到位
    if (deploy_flag == 2 && abs(targe_f - Motor_LK6010_Yaw.mang) < 2500)
    {
      deploy_flag = 0;
    }
    if (deploy_flag == 1)
    {
      if (YK.Pressed_Check(KEY_PRESSED_CTRL) && YK.Pressed_Check(KEY_PRESSED_W))
        mini_pitch_deploy_offset -= 50;
      if (YK.Pressed_Check(KEY_PRESSED_CTRL) && YK.Pressed_Check(KEY_PRESSED_S))
        mini_pitch_deploy_offset += 50;
    }
    else
    {
      mini_pitch_deploy_offset = 0;
    }

    if (MYmode == DIAO_SHE_MODE || (MYmode == ZHAN_DOU_MODE && deploy_flag))
    {
      // 微调吊射舵机
      if (YK.Pressed_Check(KEY_PRESSED_V) && !YK.Pressed_Check(KEY_PRESSED_CTRL))
        deploy_servo_ccr += DEPLOY_SERVO_STEP;
      if (YK.Pressed_Check(KEY_PRESSED_G) && !YK.Pressed_Check(KEY_PRESSED_CTRL) )
        deploy_servo_ccr -= DEPLOY_SERVO_STEP;
      deploy_servo_ccr = LIMIT(deploy_servo_ccr, DEPLOY_SERVO_CCR_MIN, DEPLOY_SERVO_CCR_MAX);  // 限制 3315~3615
    }
  }
  else
  {
    deploy_flag = 0;
    XTL_flag = 0;
    mini_pitch_anjian_flag = 0;
    mini_pitch_deploy_offset = 0;
  }
}
// Mini pitch鼠标调整，160Hz调用
void Mini_PITCH_Mouse_Adjust(void)
{
  static float mini_pitch_buf_ccr = 0;
  static uint8_t mini_pitch_ctrl_flag = 0;

  if (mini_pitch_ctrl_flag == 1)
  {
    mini_pitch_buf_ccr -= YK.shubiao.y / 40.0f;
  }
}
// 吊射模式超时检查，160Hz调用
void Deploy_Timeout_Check(void)
{
  static int16_t deploy_out_tim = 0;
  if (deploy_flag == 2)
  {
    deploy_out_tim++;
    if (deploy_out_tim > 160)
    {
      deploy_flag = 0; 
      deploy_out_tim = 0;
    }
  }
  else
  {
    deploy_out_tim = 0;
  }
}
#if CHASSIS_SAFE_ENABLE
// 底盘保险: 部署模式下若底盘4电机全部掉线, 待其全部恢复后自动退出部署(deploy_flag=0),
// 摩擦轮经 MCL_Logic 自然回落到 Near(3700)。仅在"全掉->全恢复"上升沿触发一次, 之后需手动再按R。
void Chassis_Safe_Guard(void)
{
  static uint8_t chassis_was_offline = 0;
  if (!deploy_flag) // 非部署: 不监测, 复位状态 (deploy_flag 已为0, 摩擦轮自然维持 Near 低速)
  {
    chassis_was_offline = 0;
    return;
  }
  uint8_t all_online = Chassis_Motor_M1_OK && Chassis_Motor_M2_OK && Chassis_Motor_M3_OK && Chassis_Motor_M4_OK;
  if (!all_online)
  {
    chassis_was_offline = 1; // 底盘电机(任一)掉线 -> 记住曾下线
  }
  else if (chassis_was_offline) // 曾下线 且 现已全部恢复上线 -> 上升沿, 触发一次
  {
    deploy_flag = 0;         // 退出部署 -> MCL_Logic 下一轮读到 !deploy_flag -> 摩擦轮回落 Near(3700) 降速
    chassis_was_offline = 0; // 清标志: 触发一次即可, 之后 deploy_flag=0 会走上面提前返回分支, 不再反复触发
  }
}
#endif
// 复位
void Reset(void)
{
  if (YK.Pressed_Check(KEY_PRESSED_Z) && YK.Pressed_Check(KEY_PRESSED_CTRL)) // 按住z和ctrl键进行软件复�?
  {
    for (uint8_t ii1 = 0; ii1 < 8; ii1++)
    {
      CAN_2.Send_RM(0x010, YK.yaogan.ch0, YK.yaogan.ch1, YK.yaogan.ch2, YK.yaogan.ch3);
      HAL_Delay(5);
      CAN_2.Send_RM(0x011, YK.shubiao.x, YK.shubiao.y, YK.jianpan, (uint16_t)(((YK.yaogan.s1 << 4) | (YK.yaogan.s2 << 2) | (YK.shubiao.press_l << 1) | (YK.shubiao.press_r)) & 0x003f));
      HAL_Delay(5);
    }

    uint8_t temp;
    HAL_Delay(1);
    CAN_2.Send_RM(0x200, 0, 0, 0, 0);
    HAL_Delay(1);
    CAN_2.Send_RM(0x1FF, 0, 0, 0, 0);
    HAL_Delay(1);
    for (temp = 0; temp < 25; temp++)
    {
      CAN_1.Send_RM(0x1FF, 0, 0, 0, 0);
      CAN_2.Broadcast_Send_LK(0, 0, 0, 0);
      HAL_Delay(5);
      CAN_1.Send_RM(0x200, 0, 0, 0, 0);
      HAL_Delay(5);
    }
    CAN_2.Send_RM(0x200, 0, 0, 0, 0);
    HAL_Delay(10);
    __set_FAULTMASK(1); // 关闭�?有中�?
    NVIC_SystemReset(); // 澶嶄綅
  }
}
//自瞄
void ZM_Check(void)
{
  static UpDown_check_class UD_ZM_Fire(0), UD_FIRE(0);
  static uint16_t mouse_release_counter = 0;

  #define MOUSE_SHORT_CLICK_TIME 300
  #define MOUSE_LONG_PRESS_TIME 700
  #define MOUSE_CLICK_GAP_TIME 700

  UpDown_check_state mouse_edge = UD_ZM_Fire.updata(YK.shubiao.press_r);

  // 鼠标右键上升沿，单击或双击
  if (mouse_edge == UpDown_check_rising)
  {
    mouse_press_counter = 0;

    if (is_mouse_single_clicked && mouse_release_counter < MOUSE_CLICK_GAP_TIME)
    {
      mouse_press_type = 2;  
    }
    else
    {
      mouse_press_type = 1;  
      is_mouse_single_clicked = 0;  // 清除单击标志
    } 
  }   

  // 松开鼠标上升沿
  else if (mouse_edge == UpDown_check_falling)
  {
    mouse_release_counter = 0;

    // 判断是否为短按单击
    if (mouse_press_counter < MOUSE_SHORT_CLICK_TIME)
    {
      is_mouse_single_clicked = 1;  // 标记单击状态
    }
    
    mouse_press_type = 0;
  }
  //计数
  if (YK.shubiao.press_r)
  {
    mouse_press_counter++;
    mouse_release_counter = 0;
  }
  else
  {
    mouse_release_counter++;
    mouse_press_counter = 0;

    // 超时清除单击标记
    if (is_mouse_single_clicked && mouse_release_counter >= MOUSE_CLICK_GAP_TIME)
    {
      is_mouse_single_clicked = 0;
    }
  }

  if (MYmode == ZHAN_DOU_MODE)
  {
    if (YK.shubiao.press_r)
    {
      if (mouse_press_type == 1 && mouse_press_counter >= MOUSE_LONG_PRESS_TIME) // 长按启用自瞄
      {
        request.zimiao_status = 1;
        zm_press1++;
      }
      else if (mouse_press_type == 2 && mouse_press_counter >= 100)  // 双击射击
      {
        request.zimiao_status = 1;
        if (SuperPower.mode == 2 && !ZM_Fire_delay && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL)))
        {
            Shoot_flag = 1;
            ZM_Fire_delay = 834;
        }
        zm_press2++;
      }
    }
    else
    {
      request.zimiao_status = 0;
    }
  }
  else if (MYmode == DAN_YUN_TAI_MODE)
  {
    if(SuperPower.mode == 0 && request.zimiao_status) 
    {
      MCL_ON_flag = 1;
      request.zimiao_status = 0;
      Yaw_goal = REAL_YAW_REF;
      Pitch_goal = REAL_PITCH_REF;
    }
    
    if (YK.yaogan.ch1 > 500) 
    {
      aim_mode = 0x01;
      request.zimiao_status = 1;
      MCL_ON_flag = 1;
      
      if (SuperPower.mode == 2 && !ZM_Fire_delay && YK.yaogan.ch0 < -500 && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL))) //自瞄火控
      // if (SuperPower.mode == 2 && !ZM_Fire_delay)
      {
        Shoot_flag = 1;
        ZM_Fire_delay = 834;
      }
      // else if(SuperPower.mode == 1 && !ZM_Fire_delay && YK.yaogan.ch0 > 500)
      else if(!ZM_Fire_delay && YK.yaogan.ch0 > 500 && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL))) //操作手控制
      {
        Shoot_flag = 1;
        ZM_Fire_delay = 834;
      }

    }
    else
    {
      MCL_ON_flag = 0;
      request.zimiao_status = 0;
    }
  }
  else
  {
    ZM_Fire_delay = 417;
    request.zimiao_status = 0;
  }
}
void Laser_deal(GPIO_PinState Laser_State)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, Laser_State);
}
// 自瞄保护模式处理
void ZM_Protect_Mode_Handle(void)
{
  if (MYmode == PROTECT_MODE)
  {
    request.zimiao_status = 0;
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 > 500)
    {
      Laser_deal(GPIO_PIN_SET);
    }
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 < -500)
    {
      Laser_deal(GPIO_PIN_RESET); 
    }
  }
}
// 自瞄控制
void ZM_Control(void)
{
  ZM_Check();
  ZM_Protect_Mode_Handle(); // 自瞄保护模式
}
#define PI 3.1415926
// 串口3中断自瞄(视觉)
void UART3_IT_ZM(void)
{
  static int aa;
  uint32_t temp;
//  extern BMI088 Pitch_088;
  extern float Pitch_goal, Yaw_goal;
  if ((__HAL_UART_GET_FLAG(&MINI_PC_USART_HANDLE, UART_FLAG_IDLE) != RESET))
  {

    __HAL_UART_CLEAR_IDLEFLAG(&MINI_PC_USART_HANDLE); // 清除空闲中断标志
    temp = MINI_PC_USART_HANDLE.Instance->SR;
    temp = MINI_PC_USART_HANDLE.Instance->DR;
    HAL_UART_DMAStop(&MINI_PC_USART_HANDLE);                                     // 停止DMA传输，防止数据覆盖
    // getReceiveData_ZM(Mini_PC_rx_buf);
    GetReceive_SP(Mini_PC_rx_buf);                                         // 处理接收到的数据
    HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_rx_buf, 128); // 重新启动DMA接收
    /*************** Your code *****************/

    if (Mini_PC_rx_buf[0] == 0x66 && Mini_PC_rx_buf[28] == 0x11) // 检查帧头帧尾
    {
      if(counter<= 1000)
      {
          zm_cnt++;
      }
      if (request.zimiao_status)
      {
        aa++;
        float zm_yaw_raw = SuperPower.yaw.f * 180.0f / PI;  // -180 ~ +180
        // 1. 计算当前连续yaw角度 -180~+180范围
        float current_mod = fmodf(yaw_cont.continuous_yaw, 360.0f);
        if (current_mod > 180.0f)  current_mod -= 360.0f;
        if (current_mod < -180.0f) current_mod += 360.0f;
        // 2. 计算视觉目标与当前角度 -180~+180范围
        float diff = zm_yaw_raw - current_mod;
        if (diff > 180.0f)  diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;
        Yaw_goal = yaw_cont.continuous_yaw + diff;
        Pitch_goal = -(SuperPower.pitch.f * 180.0 / PI);
				Yaw_ZM = yaw_cont.continuous_yaw + diff;
        Pitch_ZM = -(SuperPower.pitch.f * 180.0 / PI);        

      }
    }
  }
}
/**
 * @brief LED 4 自瞄状态更新函数（自定义回调）
 * @note 自瞄有三种状态：绿色（关闭）、蓝色（自瞄火控）、红色（操作手火控）
 */
void RGB_LED4_Zimiao_Update(void)
{
    if (request.zimiao_status == 0)
    {
        RGB_UI.WS281x_SetPixelRGB(4, 0, RGB_goal, 0);   
    }
    else if (PRESS_TYPE == ZM_FIRE_MODE)
    {
        RGB_UI.WS281x_SetPixelRGB(4, 0, 0, RGB_goal);   // 蓝色：自瞄火控模式
    }
    else if (PRESS_TYPE == OWN_FIRE_MODE)
    {
        RGB_UI.WS281x_SetPixelRGB(4, RGB_goal, 0, 0);   // 红色：操作手火控模式
    }
}
/**
 * @brief RGB Debug 初始化函数
 * @note 配置5个LED的设备映射和功能状态显示
 */
void RGB_Debug_Setup(void)
{
    // LED 0: Pitch/Yaw电机、云台IMU
    led_mappings[0].led_index = 0;
    led_mappings[0].device_count = 3;
    // Pitch电机（LK6010）
    led_mappings[0].devices[0].timeout_ptr = &timeout_pitch;
    led_mappings[0].devices[0].timeout_threshold = 10;  // 200ms
    led_mappings[0].devices[0].status_flag_ptr = NULL;
    // Yaw电机（LK6010）
    led_mappings[0].devices[1].timeout_ptr = &timeout_yaw;
    led_mappings[0].devices[1].timeout_threshold = 10;
    led_mappings[0].devices[1].status_flag_ptr = NULL;
    // 088
    led_mappings[0].devices[2].timeout_ptr = &timeout_imu;
    led_mappings[0].devices[2].timeout_threshold = 10;
    led_mappings[0].devices[2].status_flag_ptr = NULL;
    
    led_mappings[0].color_modes[0] = RED_MODE;        // Pitch
    led_mappings[0].color_modes[1] = GREEN_MODE;      // Yaw
    led_mappings[0].color_modes[2] = RED_GREEN_MODE;  // IMU

    // LED 1: 摩擦轮（6个，但LED映射最多4个）+ 拨盘
    // 优先监控：上轮、左轮、右轮、拨盘
    led_mappings[1].led_index = 1;
    led_mappings[1].device_count = 4;
    led_mappings[1].devices[0].timeout_ptr = &timeout_mcl_up;
    led_mappings[1].devices[0].timeout_threshold = 10;
    led_mappings[1].devices[0].status_flag_ptr = NULL;
    led_mappings[1].devices[1].timeout_ptr = &timeout_mcl_l;
    led_mappings[1].devices[1].timeout_threshold = 10;
    led_mappings[1].devices[1].status_flag_ptr = NULL;
    led_mappings[1].devices[2].timeout_ptr = &timeout_mcl_r;
    led_mappings[1].devices[2].timeout_threshold = 10;
    led_mappings[1].devices[2].status_flag_ptr = NULL;
    led_mappings[1].devices[3].timeout_ptr = &timeout_bopan;
    led_mappings[1].devices[3].timeout_threshold = 10;
    led_mappings[1].devices[3].status_flag_ptr = NULL;
    led_mappings[1].color_modes[0] = RED_MODE;
    led_mappings[1].color_modes[1] = GREEN_MODE;
    led_mappings[1].color_modes[2] = BLUE_MODE;
    led_mappings[1].color_modes[3] = RED_GREEN_MODE;

    // LED 2: 底盘3508电机（M1-M4）
    led_mappings[2].led_index = 2;
    led_mappings[2].device_count = 4;
    led_mappings[2].devices[0].timeout_ptr = NULL;
    led_mappings[2].devices[0].timeout_threshold = 0;
    led_mappings[2].devices[0].status_flag_ptr = &Chassis_Motor_M1_OK;
    led_mappings[2].devices[1].timeout_ptr = NULL;
    led_mappings[2].devices[1].timeout_threshold = 0;
    led_mappings[2].devices[1].status_flag_ptr = &Chassis_Motor_M2_OK;
    led_mappings[2].devices[2].timeout_ptr = NULL;
    led_mappings[2].devices[2].timeout_threshold = 0;
    led_mappings[2].devices[2].status_flag_ptr = &Chassis_Motor_M3_OK;
    led_mappings[2].devices[3].timeout_ptr = NULL;
    led_mappings[2].devices[3].timeout_threshold = 0;
    led_mappings[2].devices[3].status_flag_ptr = &Chassis_Motor_M4_OK;
    led_mappings[2].color_modes[0] = RED_MODE;
    led_mappings[2].color_modes[1] = GREEN_MODE;
    led_mappings[2].color_modes[2] = BLUE_MODE;
    led_mappings[2].color_modes[3] = RED_GREEN_MODE;

    // LED 3: 底盘IMU、超电
    led_mappings[3].led_index = 3;
    led_mappings[3].device_count = 2;
    led_mappings[3].devices[0].timeout_ptr = NULL;
    led_mappings[3].devices[0].timeout_threshold = 0;
    led_mappings[3].devices[0].status_flag_ptr = &Chassis_IMU_OK;
    led_mappings[3].devices[1].timeout_ptr = NULL;
    led_mappings[3].devices[1].timeout_threshold = 0;
    led_mappings[3].devices[1].status_flag_ptr = &V_Cap_OK_flag;
    led_mappings[3].color_modes[0] = RED_MODE;
    led_mappings[3].color_modes[1] = GREEN_MODE;

    // LED 4
    led_mappings[4].led_index = 4;
    led_mappings[4].device_count = 1;
    led_mappings[4].devices[0].timeout_ptr = NULL;
    led_mappings[4].devices[0].timeout_threshold = 0;
    led_mappings[4].devices[0].status_flag_ptr = &Chassis_REF_OK;
    led_mappings[4].color_modes[0] = RED_MODE;

    // ========== 初始化 RGB Debug ==========
    rgb_debug_config.rgb_ui = &RGB_UI;
    rgb_debug_config.led_mappings = led_mappings;
    rgb_debug_config.led_count = 5;
    rgb_debug_config.blink_period = 50;  // 50次计数=1秒@50Hz
    RGB_Debug_Init(&rgb_debug_config);

    // ========== 注册功能状态显示（无错误时显示） ==========

    // LED 0
    Function_Status_t xtl_status;
    xtl_status.led_index = 0;
    xtl_status.flag_ptr = &XTL_flag;
    xtl_status.on_color[0] = RGB_goal;  // 开启：红色
    xtl_status.on_color[1] = 0;
    xtl_status.on_color[2] = 0;
    xtl_status.off_color[0] = 0;    
    xtl_status.off_color[1] = RGB_goal;
    xtl_status.off_color[2] = 0;
    RGB_Control_Register_Function(xtl_status);

    // LED 1: 摩擦轮状态
    Function_Status_t mcl_status;
    mcl_status.led_index = 1;
    mcl_status.flag_ptr = &MCL_ON_flag;
    mcl_status.on_color[0] = RGB_goal;  // 开启：红色
    mcl_status.on_color[1] = 0;
    mcl_status.on_color[2] = 0;
    mcl_status.off_color[0] = 0;     
    mcl_status.off_color[1] = RGB_goal;
    mcl_status.off_color[2] = 0;
    RGB_Control_Register_Function(mcl_status);

    // LED 2: 吊射状态（替换26英雄的上台阶）
    Function_Status_t diaoshe_status;
    diaoshe_status.led_index = 2;
    diaoshe_status.flag_ptr = &deploy_flag; 
    diaoshe_status.on_color[0] = RGB_goal;   // 开启：红色
    diaoshe_status.on_color[1] = 0;
    diaoshe_status.on_color[2] = 0;
    diaoshe_status.off_color[0] = 0;      
    diaoshe_status.off_color[1] = RGB_goal;
    diaoshe_status.off_color[2] = 0;
    RGB_Control_Register_Function(diaoshe_status);

    // LED 3: 爆发状态
    Function_Status_t boost_status;
    boost_status.led_index = 3;
    boost_status.flag_ptr = &boost_flag;
    boost_status.on_color[0] = RGB_goal;  // 开启：红色
    boost_status.on_color[1] = 0;
    boost_status.on_color[2] = 0;
    boost_status.off_color[0] = 0;       
    boost_status.off_color[1] = RGB_goal;
    boost_status.off_color[2] = 0;
    RGB_Control_Register_Function(boost_status);

    // LED 4: 自瞄状态（特殊处理，使用自定义回调）
    RGB_Control_Register_LED4_Callback(RGB_LED4_Zimiao_Update);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // 8khz
{
  if (CAN_1.Receive(CAN_RX_FIFO0) == HAL_OK) 
  {
    if (Mini_Pitch_2006.update() == HAL_OK)
    {
      m2006_cnt++;
      timeout_mini_pitch = 0;  
      Mini_Pitch_2006.NSQD_8192mang_inf();

      if (Mini_Pitch_MODE == MANG_MODE)
      {
        PID_Mini_Pitch_2006_mang.PID_update(Mini_Pitch_targe, Mini_Pitch_2006.mang_inf);
        PID_Mini_Pitch_2006_sp.PID_update(PID_Mini_Pitch_2006_mang.OUT_PID, Mini_Pitch_2006.sp);
        Mini_Pitch_output = PID_Mini_Pitch_2006_sp.OUT_PID;
      }
      else if (Mini_Pitch_MODE == DUZHUAN_MODE)
      {
        Mini_Pitch_output = DUZHUAN_DIANLIU;
        if (Mini_Pitch_2006.sp > -DUZHUAN_SUDU_YUZHI && Mini_Pitch_2006.sp < DUZHUAN_SUDU_YUZHI)
          Mini_Pitch_duzhuan_cnt++;
        else
          Mini_Pitch_duzhuan_cnt = 0;
      }
      else
      {
        Mini_Pitch_output = 0;
      }
    }
    else if (MCL_Start_flag)
    {
    if (Motor_MCL_UP_up.update() == HAL_OK) // 1khz
    {
      timeout_mcl_upup = 0;  
      MCL_4_motorflag |= 0x1;
      PID_MCL_UP_sp.PID_update(-UP_targe_sp, Motor_MCL_UP_up.sp);
    } //-

    else if (Motor_MCL_R.update() == HAL_OK) // 1khz
    {
      timeout_mcl_r = 0; 
      MCL_4_motorflag |= 0x2;
      PID_MCL_R_sp.PID_update(-R_targe_sp, Motor_MCL_R.sp);
    } //-

    else if (Motor_MCL_L.update() == HAL_OK)
    {
      timeout_mcl_l = 0;  
      MCL_4_motorflag |= 0x4;
      PID_MCL_L_sp.PID_update(L_targe_sp, Motor_MCL_L.sp);
    } //+

    else if (Motor_MCL_RR.update() == HAL_OK)
    {
      timeout_mcl_rr = 0; 
      MCL_4_motorflag |= 0x8;
      PID_MCL_RR_sp.PID_update(-RR_targe_sp, Motor_MCL_RR.sp);
    } //-

    else if (Motor_MCL_UP.update() == HAL_OK)
    {
      timeout_mcl_up = 0; 
      MCL_2_motorflag |= 0x1;
      PID_MCL_UPUP_sp.PID_update(-UPUP_targe_sp, Motor_MCL_UP.sp);
    } //-

    else if (Motor_MCL_LL.update() == HAL_OK)
    {
      timeout_mcl_ll = 0;
      MCL_2_motorflag |= 0x2;
      PID_MCL_LL_sp.PID_update(LL_targe_sp, Motor_MCL_LL.sp);
    } //+

    // 6khz
   if (MCL_4_motorflag == 0x0F)
   {
       CAN_1.Send_RM(0x200, PID_MCL_L_sp.OUT_PID,PID_MCL_UP_sp.OUT_PID, PID_MCL_R_sp.OUT_PID,PID_MCL_LL_sp.OUT_PID); // 1khz
//      CAN_1.Send_RM(0x200, 0, 0, 0, PID_MCL_LL_sp.OUT_PID);
      MCL_4_motorflag = 0;
   } 

   if (MCL_2_motorflag == 0x3)
   {
     MCL_2_motorflag = 0;
     MCL_MID = (Motor_MCL_L.sp - Motor_MCL_R.sp - Motor_MCL_UP_up.sp + Motor_MCL_LL.sp - Motor_MCL_UP.sp - Motor_MCL_RR.sp) / 6; // 摩擦轮平均转速，用于判断摩擦�?
   }
     }
  }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) // 4khz
{
  static uint8_t BP_updated = 0;
  static uint8_t Mini_Pitch_updated = 0;

  uint8_t msg_count = 0;
  const uint8_t MAX_MSG_PER_INTERRUPT = 8;

  while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1) > 0 && msg_count < MAX_MSG_PER_INTERRUPT)
  {
    msg_count++;

    if (CAN_2.Receive(CAN_RX_FIFO1) != HAL_OK)
    {
      break;
    }

    if (Motor_LK6010_Yaw.LK_Broadcast_update() == HAL_OK)
    {
      timeout_yaw = 0;  
      YAW_Angle_Update();
    }
    else if (Motor_LK6010_Pitch.LK_Broadcast_update() == HAL_OK)
    {
      timeout_pitch = 0; 
      PITCH_Angle_Update();
    }
    else if (BoPan.update() == HAL_OK)
    {
      timeout_bopan = 0;  
      BoPan.NSQD_8192mang_inf();

      if (BP_MODE == MANG_MODE)
      {
        PID_BP_mang.PID_update(BP_targe, BoPan.mang_inf);
        PID_BP_sp.PID_update(PID_BP_mang.OUT_PID, BoPan.sp);
        BP_output = PID_BP_sp.OUT_PID;
      }
      else if (BP_MODE == SPEED_MODE)
      {
        BP_targe = BoPan.mang_inf;
        BP_calc_targe = BoPan.mang_inf;
        PID_BP_sp.PID_update(0, BoPan.sp);
        BP_output = PID_BP_sp.OUT_PID;
      }
      else if (BP_MODE == PROTECT_MODE)
      {
        BP_protect_cansend_flag = 1;
        BP_ON_flag = 0;
        BP_targe = BoPan.mang_inf;
        BP_calc_targe = BoPan.mang_inf;
        BP_output = 0; 
      }
    }
    switch (CAN_2.RxHeader.StdId)
    {
    case (0x012):
      DP_Tx_static_Flag = CAN_2.rx_buf[0] << 8 | CAN_2.rx_buf[1];
      turn_flag = DP_Tx_static_Flag & TURN_FLAG;
      mine_flag = DP_Tx_static_Flag & MINE_FLAG;
      heat_flag = DP_Tx_static_Flag & HEAT_FLAG;
      boost_flag = DP_Tx_static_Flag & BOOST_FLAG;
      Shangtaijie_flag = (DP_Tx_static_Flag & 0x0020) ? 1 : 0;  // bit5：底盘上台阶模式，用于让2006保持不动
      request.mine = mine_flag;
      Shoot_speed_u.c[0] = CAN_2.rx_buf[3];
      Shoot_speed_u.c[1] = CAN_2.rx_buf[2];
      Shoot_speed_u.c[2] = CAN_2.rx_buf[5];
      Shoot_speed_u.c[3] = CAN_2.rx_buf[4];
      shoot_sp = Shoot_speed_u.f_speed;
      break;

    case (0x013):
      Robot_pos_u.c[0] = CAN_2.rx_buf[1];
      Robot_pos_u.c[1] = CAN_2.rx_buf[0];
      Robot_pos_u.c[2] = CAN_2.rx_buf[3];
      Robot_pos_u.c[3] = CAN_2.rx_buf[2];
      // Yaw_ab = Robot_pos_u.f_pos;
      break;

    case (0x120): // 底盘状态反馈
    {
      // 底盘状态标志位在rx_buf[0]和rx_buf[1]
      uint16_t Chassis_Status_Flag_Received = CAN_2.rx_buf[0] << 8 | CAN_2.rx_buf[1];
      Chassis_Motor_M1_OK = (Chassis_Status_Flag_Received & CHASSIS_M1_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M2_OK = (Chassis_Status_Flag_Received & CHASSIS_M2_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M3_OK = (Chassis_Status_Flag_Received & CHASSIS_M3_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M4_OK = (Chassis_Status_Flag_Received & CHASSIS_M4_OK_FLAG) ? 1 : 0;
      Chassis_IMU_OK = (Chassis_Status_Flag_Received & CHASSIS_IMU_OK_FLAG) ? 1 : 0;
      Chassis_REF_OK = (Chassis_Status_Flag_Received & CHASSIS_REF_OK_FLAG) ? 1 : 0;

      // 超电电压在rx_buf[2]和rx_buf[3]（int16_t格式）
      int16_t v_cap_received = (int16_t)((CAN_2.rx_buf[2] << 8) | CAN_2.rx_buf[3]);
      V_Cap_Real = (float)v_cap_received / 100.0f;
      break;
    }

    default:
      break;
    }
  }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim12) // 1khz
  {
    TIM12_Callback();
    static uint8_t tim_bopan = 0;
    tim_bopan = !tim_bopan;
    if (tim_bopan)
    {
        CAN_2.Send_RM(0x200, BP_output, 0, 0, 0); 
    }
    CAN_1.Send_RM(0x1FF, MCL_Start_flag ? PID_MCL_UPUP_sp.OUT_PID : 0, MCL_Start_flag ? PID_MCL_RR_sp.OUT_PID : 0, Mini_Pitch_output, 0);
  }
  if (htim == &htim9) // 2kHz
  {
    static uint8_t can_scan = 1;

    if (can_scan)
    {
      // YAW和PITCH电机PID计算
      YAW_PID_Calc();
      PITCH_PID_Calc();

      //CAN_2.Broadcast_Send_LK(0, YAW_PID_OUT, 0, 0);
      CAN_2.Broadcast_Send_LK(PITCH_PID_OUT, YAW_PID_OUT, 0, 0);			
      can_scan = 0;
    }
    else
    {
      // 自瞄控制
       ZM_Control();
      //拨盘
      if (BP_MODE == MANG_MODE)
      {
        if (deploy_flag || MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
        // if (deploy_flag || MYmode == DIAO_SHE_MODE)        
        {
          PID_BP_mang.KP = 0.25f;
          I_slow(&BP_targe, BP_calc_targe, 120, 120, 53, &BP_Islow_incbuf);
        }
        else
        {
          PID_BP_mang.KP = 0.25f;
          BP_targe = BP_calc_targe;
        }
        BP_KD_TIM();
      }
      if (ZM_Fire_delay)
      {
        ZM_Fire_delay--;
      }
      Communication_boards();

      can_scan = 1;
    }
  }

  else if (htim == &htim7) // 160Hz
  {
    // Mini pitch鼠标调整
    Mini_PITCH_Mouse_Adjust();

    Deploy_Timeout_Check();

    // 舵机确认动作相关
    if(servo_confirm_flag)
    {
        servo_confirm_timer++;

        if(servo_confirm_timer < 30)  
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr + 900);
        }
        else if(servo_confirm_timer < 60) 
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr);
        }
        else
        {
            servo_confirm_flag = 0;
            servo_confirm_timer = 0;
        }
    }

    // 遥控器键鼠发送(交替)
    static uint8_t Send_flag = 0;
    if (Send_flag)
    {
      can2_bjtx011 = 1;
      Send_flag = 0;
    }
    else
    {
      can2_bjtx010 = 1;
      Send_flag = 1;
    }
  }

  else if (htim == &htim1) // 50Hz
  {
    static uint8_t scan_tim = 0;
    scan_tim = !scan_tim;
    if (scan_tim)
    {
      TX_VD_Deal();
    }

    // 更新所有超时计数器
    if (timeout_pitch < 255) timeout_pitch++;
    if (timeout_yaw < 255) timeout_yaw++;
    if (timeout_bopan < 255) timeout_bopan++;
    if (timeout_mcl_up < 255) timeout_mcl_up++;
    if (timeout_mcl_r < 255) timeout_mcl_r++;
    if (timeout_mcl_l < 255) timeout_mcl_l++;
    if (timeout_mcl_rr < 255) timeout_mcl_rr++;
    if (timeout_mcl_ll < 255) timeout_mcl_ll++;
    if (timeout_mcl_upup < 255) timeout_mcl_upup++;
    if (timeout_imu < 255) timeout_imu++;
    if (timeout_mini_pitch < 255) timeout_mini_pitch++;
    // 2006(0x207)掉线(>100ms 无反馈)时电流归0, 使0x1FF第3字段发0; 摩擦轮UPUP/RR照常发
    if (timeout_mini_pitch > 5) Mini_Pitch_output = 0;

    // 更新超电电压标志
    V_Cap_OK_flag = (V_Cap_Real != 0);

    // 调用 RGB Debug API 更新
    RGB_Debug_Update();

    /**保护模式 */
    if (BP_MODE == PROTECT_MODE)
    {
        BP_protect_cansend_flag = 1;
        BP_ON_flag = 0;
        BP_targe = BoPan.mang_inf;
        BP_calc_targe = BoPan.mang_inf;
    }
    // YAW保护模式
    if (yaw_control_mode == PROTECT_MODE)
    {
      YAW_PID_OUT = 0;
      Yaw_goal = REAL_YAW_REF;
      PID_YAW_Erro_IMU_MANG.Integral = 0;
      PID_YAW_Erro_IMU_MANG.OUT_I = 0;
    }

    // PITCH保护模式
    if (pitch_control_mode == PROTECT_MODE)
    {
      Pitch_goal = REAL_PITCH_REF;
    }
  }

  else if (htim == &htim8) // 20HZ
  {
    can2bjtx013 = 1;
    can2bjtx014 = 1;
  }

  else if (htim == &htim6) // 10hz 鐪嬮棬鐙楀畾鏃跺櫒
  {
      YK.DT16_watchdog_run();
      YK.VT13_watchdog_run();
      YK.YK_ctrl();
  }
  else if (htim == &htim5) 
  {
    AS.Q_info_0.f = hipnuc_raw.hi91.quat[0];
    AS.Q_info_1.f = hipnuc_raw.hi91.quat[1];
    AS.Q_info_2.f = hipnuc_raw.hi91.quat[2];
    AS.Q_info_3.f = hipnuc_raw.hi91.quat[3]; 
    bullet_speed_u.f = 11.9;   

      if (MYmode == ZHAN_DOU_MODE && YK.shubiao.press_r)
      {
          if (mouse_press_type == 1 && mouse_press_counter >= MOUSE_LONG_PRESS_TIME)
          {
              aim_mode = 0x01;  
          }
          else if (mouse_press_type == 2 && mouse_press_counter >= 100)
          {
              aim_mode = 0x01;  
          }
      }
      else if(MYmode == ZHAN_DOU_MODE && !YK.shubiao.press_r)
      {
        aim_mode = 0x00;
      }

      Mini_PC_SendData_ZM2(aim_mode);  
  }
}

extern "C" void My_Setup(void)
{
  HAL_Delay(15);
  Pitch_088_state = Pitch_088.Init();
  CAN_1.Init(0, 0);
  CAN_2.Init(1, 1);
  // YK.Init();
	YK.VT13_Init();
	YK.DT16_Init();
  yaw_control_mode = PROTECT_MODE;
  pitch_control_mode = PROTECT_MODE;
	RGB_UI.RGB_UI_Init();
	RGB_Debug_Setup();  // 初始化 RGB Debug API
  HAL_TIM_Base_Start_IT(&htim1); // 舵机
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim9); // 2khz主控，can2
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim7);
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim8); // 40hz 0x13id
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim6);//10hz
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim5);//800hz
  HAL_Delay(0);	
  HAL_TIM_Base_Start_IT(&htim12); 
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  Mini_PC_UART_Init();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); // 使能电机驱动
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);

  __HAL_UART_ENABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_IDLE);
  extern DMA_HandleTypeDef hdma_usart3_rx;
  __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT); 
  HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)VD_2rx_buf[0], sizeof(VD_2rx_buf[0]));

  HAL_Delay(1500);
  Mini_Pitch_MODE = DUZHUAN_MODE;
	IMU_UART_Init();
}

volatile uint8_t VD_rxcnt = 0;
volatile uint8_t VD_tx_state = 0;
volatile uint8_t u6_tx_cnt = 0;
volatile uint8_t rxcnt_sum = 0;
volatile uint32_t tx_drop_cnt = 0; // 满载丢包检测:上一帧未发完就来新帧(发送被中断饿死)则+1
volatile uint16_t ct_data_len = 0; // 辅助吊射 CT 帧第2、3字节的长度字段(小端),仅供调试/上报,不参与拒帧
extern "C" void My_Loop(void)
{
/*遥控器数据*/
//     INFO("%d,%d,%d,%d,%d,%d\r\n",YK.yaogan.ch0,YK.yaogan.ch1,YK.yaogan.ch2,YK.yaogan.ch3,YK.yaogan.s1,YK.yaogan.s2);
/*pitch内外环目标实际*/
  //  INFO("%.2f,%.2f\r\n", Pitch_goal, hipnuc_raw.hi91.pitch);
      // INFO("%.2f,%.2f\r\n", PID_LK_Pitch_Mang.OUT_PID, hipnuc_raw.hi91.gyr[0]);
    // INFO("%.2f,%.2f\r\n", PID_LK_Pitch_Mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[1]);
    // INFO("%.2f,%.2f\r\n", PID_LK_Erro_Pitch_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[0]);  
/*pitch编码器*/
//   	 INFO("%d\r\n",Motor_LK6010_Pitch.mang);
/*拨盘电机内外环目标实际*/
    //  INFO("%d,%d\r\n",BP_targe, BoPan.mang_inf);    
//     INFO("%.2f,%d\r\n", PID_BP_mang.OUT_PID, BoPan.sp);
/*拨盘电机编码器*/
//     INFO("%d\r\n", BoPan.mang_inf);   
/*bmi088螺仪三轴角度角速度*/
    // INFO("%.2f,%.2f,%.2f\r\n", Pitch_088.realAngle.yaw, Pitch_088.realAngle.roll,Pitch_088.realAngle.pitch);
//		 INFO("%.2f,%.2f,%.2f\r\n",Pitch_088.Anglespeed.Deal_yaw, Pitch_088.Anglespeed.Deal_roll,Pitch_088.realAngle.pitch);
//				 INFO("%.2f,%.2f,%.2f,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%d\r\n",response.pitch.f, response.yaw.f,response.distance.f,a,b,c,d,e,f,g,h,o,p,counter);
/*yaw内外环目标实际*/       
  //  INFO("%.2f,%.2f\r\n", Yaw_goal, yaw_cont.continuous_yaw);
//     INFO("%.2f,%.2f\r\n",PID_Yaw_mang.OUT_PID, hipnuc_raw.hi91.gyr[2]);   
    // INFO("%.2f,%.2f\r\n",PID_YAW_Erro_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[2]);
    // INFO("%.2f,%.2f\r\n",PID_Yaw_mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[2]);hipnuc_raw.hi91.pitch
/*yaw编码器*/      
//		 INFO("%d\r\n",Motor_LK6010_Yaw.mang);
/*摩擦轮电机反馈转速*/
//     INFO("%d,%d,%d,%d,%d,%d\r\n", Motor_MCL_UP_up.sp, Motor_MCL_R.sp, -Motor_MCL_L.sp, Motor_MCL_RR.sp, -Motor_MCL_LL.sp, Motor_MCL_UP.sp);
/*摩擦轮单个电机反馈转速*/
    // INFO("%d\r\n", Motor_MCL_UP_up.sp);
/*射速*/
//      INFO("%.2f\r\n", shoot_sp);
/*自瞄*/
	  // INFO("%.2f,%.2f,%.2f,%.2f,%d,%d,%d\r\n", Yaw_ZM,yaw_cont.continuous_yaw,Pitch_ZM,hipnuc_raw.hi91.pitch,SuperPower.mode,response.Fire_Flag,is_mouse_single_clicked);
/*CH010陀螺仪三轴角度*/
//     INFO("%.2f,%.2f,%.2f,%.2f\r\n",  hipnuc_raw.hi91.roll, hipnuc_raw.hi91.yaw, hipnuc_raw.hi91.pitch,yaw_cont.continuous_yaw);
/*CH010闄€铻轰华鍥涘厓鏁?*/    
		// INFO("%.2f,%.2f,%.2f,%.2f\r\n", hipnuc_raw.hi91.quat[0],hipnuc_raw.hi91.quat[1],hipnuc_raw.hi91.quat[2],hipnuc_raw.hi91.quat[3]);
/*CH010陀螺仪角速度*/
    // INFO("%.2f,%.2f,%.2f\r\n",hipnuc_raw.hi91.gyr[0],hipnuc_raw.hi91.gyr[1],hipnuc_raw.hi91.gyr[2]);
/*榧犳爣鏁版嵁*/
    // INFO("%d,%d\r\n", YK.shubiao.x, YK.shubiao.y);
/*热量标志*/
    // INFO("%d\r\n", heat_flag);
/*枪口位置信息*/
	  // INFO("%.2f\r\n", Robot_pos_u.f_pos);锛?
/*2006小pitch内外环目标实际*/
  //  INFO("%d,%d\r\n",Mini_Pitch_targe, Mini_Pitch_2006.mang_inf);
		//  INFO("%.2f,%d\r\n",PID_Mini_Pitch_2006_mang.OUT_PID, Mini_Pitch_2006.sp);
/*红蓝方*/
    // INFO("%d\r\n", mine_flag);
    HAL_Delay(10);

    // 模式
    MYMODE_while_application_layer();

#if CHASSIS_SAFE_ENABLE
    // 底盘保险: 部署时底盘全掉线后恢复 -> 自动退出部署降速
    Chassis_Safe_Guard();
#endif

    // 摩擦轮逻辑，返回摩擦轮是否启动标志
    uint8_t mcl_on = MCL_Logic();

    // 拨盘逻辑，根据摩擦轮状态决定是否拨动
    BP_Logic(mcl_on);

    // Mini_Pitch_2006控制逻辑
    Mini_Pitch_2006_Logic();

    // 舵机PWM输出（仅telescope，mini_pitch已改用2006电机）
    Servo_PWM_Output();

    // 键盘特殊功能处理
    Keyboard_Special_Func();

    // PITCH轴控制逻辑
    PITCH_Logic();

    // YAW轴控制逻辑
    YAW_Logic();

    // 掉头处理函数
    YAW_Turn_Handle();

    // 复位
    Reset();

    // 云台板发送标志位给底盘板
    YT_Tx_static_Flag = (bool)deploy_flag ? (YT_Tx_static_Flag | 0x0001) : (YT_Tx_static_Flag & (uint16_t)~1);
    YT_Tx_static_Flag = XTL_flag ? (YT_Tx_static_Flag | 0x0002) : (YT_Tx_static_Flag & (uint16_t)~2);
    YT_Tx_static_Flag = SP_Turn_Flag ? (YT_Tx_static_Flag | SP_TURN_FLAG) : (YT_Tx_static_Flag & ~SP_TURN_FLAG);
    YT_Tx_static_Flag = request.zimiao_status ? (YT_Tx_static_Flag | 0x0080) : (YT_Tx_static_Flag & (uint16_t)~0x0080);  // bit7：自瞄开关状态，传给底盘 UI 的 ZM 指示
}

// rxcnt_sum-VD_rxcnt
uint8_t VD_2rx_buf[2][VD_RX_NUM] = {0};
uint8_t VD_FIFO = 0;
uint16_t VD_rx_byte = 0;
#define VD_DATA_NUM 300
#define CT_HEADER_LEN 16
uint8_t NSQD_De_video_buffer[VD_DATA_NUM];
uint8_t TX_VD_buf[FRAME_HEADER_LENGTH + CMD_ID_LENGTH + VD_DATA_NUM + FRAME_TAIL_LENGTH];
volatile uint8_t VD_rx_state = 0;
volatile uint8_t DMATX = 0;
void VD_2rx(DMA_HandleTypeDef *hdma)
{
  uint32_t temp;
  uint32_t temp_ndtr;
  if ((__HAL_UART_GET_FLAG(&MINI_PC_USART_HANDLE, UART_FLAG_IDLE) != RESET))
  {
    __HAL_UART_CLEAR_IDLEFLAG(&MINI_PC_USART_HANDLE);
    temp = MINI_PC_USART_HANDLE.Instance->SR;
    temp = MINI_PC_USART_HANDLE.Instance->DR;
    HAL_UART_DMAStop(&MINI_PC_USART_HANDLE);
    temp_ndtr = hdma->Instance->NDTR;
    VD_rx_byte = VD_RX_NUM - temp_ndtr;
    VD_FIFO = !VD_FIFO;
    HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)VD_2rx_buf[VD_FIFO], sizeof(VD_2rx_buf[0]));

    rxcnt_sum++;
    // 发送端分两个独立的包发出(中间有空闲间隔,各触发一次 IDLE):
    //   CT 头包   = 16 字节  (C,T + len2 + pitch4 + yaw4 + distance4)
    //   VD 视频包 = 304 字节 (V,D + 2字节子头 + 300字节视频)
    uint8_t *ct_buf = VD_2rx_buf[!VD_FIFO];

    // CT 头包: 只解析辅助吊射数据 (小端 float32)
    if (ct_buf[0] == 'C' && ct_buf[1] == 'T' && VD_rx_byte == CT_HEADER_LEN)
    {
      // 读取第 2、3 字节的长度字段 (小端),仅供调试/上报,不参与拒帧
      ct_data_len = (uint16_t)(ct_buf[2] | (ct_buf[3] << 8));

      memcpy(&vision_pitch,    (const void *)(ct_buf + 4),  4);
      memcpy(&vision_yaw,      (const void *)(ct_buf + 8),  4);
      memcpy(&vision_distance, (const void *)(ct_buf + 12), 4);

      ct_buf[0] = 0;
      ct_buf[1] = 0;
    }
    // VD 视频包: 只转发视频 (视频负载在偏移 4 处,原有逻辑不变)
    else if (ct_buf[0] == 'V' && ct_buf[1] == 'D' && VD_rx_byte == VD_DATA_NUM + 4)
    {
      // 丢包检测:若 huart6 仍在发上一帧(gState!=READY),说明发送被高优先级中断饿死、
      // 跟不上接收节奏,本帧会覆写正在发送的缓冲导致坏帧。仅计数,不改变原有行为。
      if (huart6.gState != HAL_UART_STATE_READY)
      {
        tx_drop_cnt++;
      }
      memcpy(NSQD_De_video_buffer, VD_2rx_buf[!VD_FIFO] + 4, VD_DATA_NUM);
      TC.Data_Concatenation(NSQD_De_video_buffer, TX_VD_buf, VD_DATA_NUM, 0x310);
      DMATX = HAL_UART_Transmit_IT(&huart6, TX_VD_buf, FRAME_HEADER_LENGTH + CMD_ID_LENGTH + VD_DATA_NUM + FRAME_TAIL_LENGTH);

      VD_2rx_buf[!VD_FIFO][0] = 0;
      VD_2rx_buf[!VD_FIFO][1] = 0;
      VD_rxcnt++;
      VD_rx_state = 1;
    }
    /*deal*/
  }
}

void TX_VD_Deal(void)
{
  if (VD_rx_state)
  {

    VD_rx_state = 0;
    VD_tx_state = 1;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart6)
  {
    u6_tx_cnt++;
    VD_tx_state = 0;
  }
}
extern "C" void IT_UART5_YK_Handle(void)
{
  YK.DT16_RxCplt_IRQHandler();
}

extern "C" void IT_USART6_YK_Handle(void)
{
    if (YK.VT13_RxCplt_IRQHandler())
    {
            YK.rx_cnt++;
        YK.VT13_YK_deal();
        YK.VT13_self_ctrl_deal();
        YK.VT13_UART_Receive_enable();
    }
    else
    {
        YK.vt_yk_cnt++;
    }
}
