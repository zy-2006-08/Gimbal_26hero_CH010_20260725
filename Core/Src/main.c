/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
/*28215�?61050*/
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

uint8_t a,b,c,d,e,f,g,h,o,p;
#define MCL_KP 20.0F
uint32_t counter;
uint8_t XTL_flag = 0;
#define Laser_ON HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET) // �?光开
#define Laser_OFF HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET)
// 计数�?
uint8_t p_cnt, y_cnt, bp_cnt, mcl_cnt, zm_cnt, js_cnt,bj,imu_cnt,zm_press1,zm_press2,m2006_cnt,duoji;
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
MOTOR_RM  BoPan(0x201, &CAN_2),Mini_Pitch_2006(0x202, &CAN_2);
int16_t Mini_Pitch_output = 0; // Mini_Pitch电机电流输出（供遥测使用，文件作用域）

MOTOR_RM  Motor_MCL_UP_up(0x202, &CAN_1), 
					Motor_MCL_R(0x203, &CAN_1),         
					Motor_MCL_L(0x201, &CAN_1),       
					Motor_MCL_RR(0x206, &CAN_1),  
					Motor_MCL_UP(0x205, &CAN_1),        
					Motor_MCL_LL(0x204, &CAN_1);       

MOTOR_LK Motor_LK6010_Pitch(0X141, &CAN_2), Motor_LK6010_Yaw(0x142, &CAN_2); // LK6010电机 Pitch和Yaw+

PID_class PID_Mini_Pitch_2006_mang(0.6, 0, 0, 30000, 0, 30000, 16000, 0,0), // 0.178, 0, 0, 30000, 0, 0, 30000, 0,0                       
          PID_Mini_Pitch_2006_sp(0.6, 0, 0, 30000, 0, 0, 10000, 0, 0); // 1.5  C610 ESC max current cmd +-10000; was 10, 0, 0, 16000, 0, 0, 16000, 0, 0

PID_class PID_BP_mang(0.6f, 0, 0, 30000, 0, 0, 30000, 0,0), // 0.6f, 0, 0, 30000, 0, 0, 30000, 0,0
          PID_BP_sp(20, 0, 0, 16000, 0, 0, 16000, 0, 0); // 20, 0, 0, 16000, 0, 0, 16000, 0, 0
          
PID_class PID_MCL_UP_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0), // 上轮速度PID
          PID_MCL_R_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0), // 右轮速度PID
          PID_MCL_L_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0,0), // 左轮速度PID
          PID_MCL_RR_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0),   // 右后�? sp+
          PID_MCL_UPUP_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0), // 上轮 sp-20
          PID_MCL_LL_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0);   // 左后�? sp-

PID_class PID_LK_Pitch_Mang(14, 0, 0, 240, 1, 1, 240, 250, 50), //18, 0, 0, 240, 1, 1, 240, 250, 50
          PID_LK_Pitch_SP(6, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
          PID_LK_Pitch_Mang_zm(13, 0.03, 0, 240, 100, 1, 240, 0.12, 50), //13, 0.03, 0, 240, 100, 1, 240, 0.12, 50
          PID_LK_Pitch_SP_zm(8, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
          PID_LK_Erro_Pitch_IMU_MANG(14, 0.15, 0, 240, 500, 1, 240, 0.1, 50), //25, 0.15, 0, 240, 500, 1, 240, 0.1, 50
          PID_LK_Erro_Pitch_IMU_Gyro(6, 0, 0, 600, 0, 0, 600, 0, 0); //8, 0, 0, 600, 0, 0, 600, 0, 0

PID_class PID_Yaw_mang(12, 0, 0, 500, 100, 0, 500, 50, 0), //7, 0, 0, 500, 100, 0, 500, 50, 0
          PID_Yaw_sp(4, 0, 0, 400, 0, 0, 500, 0,0),   //3.5, 0, 0, 400, 0, 0, 500, 0,0
          PID_Yaw_mang_zm(13.5, 0.05, 0, 500, 100, 0, 500, 0.12, 50), //13.5, 0.05, 0, 500, 100, 0, 500, 0.12, 50
          PID_Yaw_sp_zm(7, 0, 0, 400, 0, 0, 500, 0,0),               // 7, 0, 0, 400, 0, 0, 500, 0,0                                                          
          PID_YAW_Erro_IMU_MANG(12, 0.12, 0, 500, 100, 0, 500, 0.06,50), //13, 0.07, 200, 500, 100, 0, 500, 0.1,50
          PID_YAW_Erro_IMU_GYRO(4, 0, 0, 400, 0, 0, 500, 0,0); // 9, 0, 0, 400, 0, 0, 500, 0,0
					
/*pitch拧紧参数*/		
//PID_class PID_LK_Pitch_Mang(18, 0, 0, 240, 1, 1, 240, 250, 50), //18, 0, 0, 240, 1, 1, 240, 250, 50
//          PID_LK_Pitch_SP(8, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
//          PID_LK_Pitch_Mang_zm(13, 0.03, 0, 240, 100, 1, 240, 0.12, 50), //13, 0.03, 0, 240, 100, 1, 240, 0.12, 50
//          PID_LK_Pitch_SP_zm(8, 0, 0, 600, 0, 0, 600, 0, 0), //8, 0, 0, 600, 0, 0, 600, 0, 0
//          PID_LK_Erro_Pitch_IMU_MANG(32, 0.15, 0, 240, 500, 1, 240, 0.05, 50), //18, 0, 0, 240, 1, 1, 240, 250, 50
//          PID_LK_Erro_Pitch_IMU_Gyro(10, 0, 0, 600, 0, 0, 600, 0, 0); //8, 0, 0, 600, 0, 0, 600, 0, 0

Vision_LPF V_Pitch(30, 0.001, 3.1415926f);
Vision_LPF V_Yaw(30, 0.001, 3.1415926f);
BMI088 Pitch_088(&hspi1, &htim12, GPIOC, GPIO_PIN_4, GPIOA, GPIO_PIN_4, 800, 0.0f, BMI088_GYRO_RANGE_2000, BMI088_ACC_RANGE_3, "put the Update function in 400Hz interrupt", 1);

RC YK(&huart5, &huart6);

extern DMA_HandleTypeDef hdma_usart6_rx; // 声明DMA句柄
TuChuan TC(&huart6, &hdma_usart6_rx);

RGB_UI RGB_UI(&htim3,TIM_CHANNEL_3,"put the Update function in 800kHz interrupt");
//RGB_UI RGB_UI(&htim4,TIM_CHANNEL_4,"put the Update function in 800kHz interrupt");
//RGB_UI RGB_UI(&htim3,TIM_CHANNEL_1,"put the Update function in 800kHz interrupt");

// ========== RGB Debug 相关变量 ==========
#define RGB_goal 225  // WS2812亮度值

// 超时计数器（用于设备故障检测，50Hz下10次=200ms）
uint8_t timeout_pitch = 0;      // Pitch电机超时计数
uint8_t timeout_yaw = 0;        // Yaw电机超时计数
uint8_t timeout_bopan = 0;      // 拨盘电机超时计数
uint8_t timeout_mcl_up = 0;     // 上摩擦轮超时计数
uint8_t timeout_mcl_r = 0;      // 右摩擦轮超时计数
uint8_t timeout_mcl_l = 0;      // 左摩擦轮超时计数
uint8_t timeout_mcl_rr = 0;     // 右后摩擦轮超时计数
uint8_t timeout_mcl_ll = 0;     // 左后摩擦轮超时计数
uint8_t timeout_mcl_upup = 0;   // 上上摩擦轮超时计数
uint8_t timeout_imu = 0;        // 云台IMU超时计数
uint8_t timeout_mini_pitch = 0; // Mini Pitch 2006电机超时计数

// 底盘设备状态标志位（从底盘板通过CAN接收）
uint8_t Chassis_Motor_M1_OK = 0;   // 底盘电机M1状态（0x201）
uint8_t Chassis_Motor_M2_OK = 0;   // 底盘电机M2状态（0x202）
uint8_t Chassis_Motor_M3_OK = 0;   // 底盘电机M3状态（0x203）
uint8_t Chassis_Motor_M4_OK = 0;   // 底盘电机M4状态（0x204）
uint8_t Chassis_IMU_OK = 0;        // 底盘IMU状态
uint8_t Chassis_REF_OK = 0;        // 裁判系统连接状态

// 超电电压相关
float V_Cap_Real = 0.0f;           // 超电电压实际值
uint8_t V_Cap_OK_flag = 0;         // 超电电压检查标志（0=故障，非0=正常）

// RGB Debug API 配置变量
RGB_Debug_Config_t rgb_debug_config;
LED_Mapping_t led_mappings[5];

// 底盘设备状态标志位定义（扩展DP_Tx_static_Flag，用于CAN通信）
#define CHASSIS_M1_OK_FLAG    ((uint16_t)0x0001 << 5)   // 底盘电机0x201
#define CHASSIS_M2_OK_FLAG    ((uint16_t)0x0001 << 6)   // 底盘电机0x202
#define CHASSIS_M3_OK_FLAG    ((uint16_t)0x0001 << 7)   // 底盘电机0x203
#define CHASSIS_M4_OK_FLAG    ((uint16_t)0x0001 << 8)   // 底盘电机0x204
#define CHASSIS_IMU_OK_FLAG   ((uint16_t)0x0001 << 9)   // 底盘IMU
#define CHASSIS_CAP_OK_FLAG   ((uint16_t)0x0001 << 10)  // 超级电容
#define CHASSIS_REF_OK_FLAG   ((uint16_t)0x0001 << 11)  // 裁判系统

/********  通信标志�?   **********/
uint16_t YT_Tx_static_Flag = 0, // 云台发�?�静态标志位bool
    DP_Tx_static_Flag = 0;      // 底盘发�?�静态标志位bool 0关闭 1�?启底盘跟�?
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

//�?螺仪状�??
uint8_t Pitch_088_state = BMI088_ERROR;

// 用于判断电机是否都接收到数据，用于控制发送频�?
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
int16_t MCL_MID;                                                                       // 摩擦轮中�?
int16_t MCL_MAX_Speed_Near = 5250, MCL_MAX_Speed_Far = 5200, MCL_MAX_Speed_Now = 3700; // 5170改为3750,3700， 5200
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

// Mini_Pitch_2006控制变量
uint8_t Mini_Pitch_MODE = PROTECT_MODE;
int32_t Mini_Pitch_targe = 0;              // 当前目标位置
int32_t Mini_Pitch_calc_targe = 0;         // 计算目标位置（用于平滑过渡）
uint8_t Mini_Pitch_preset_flag = 0;        // 预置触发标志
float Mini_Pitch_pwm_to_mang_ratio = 1.0f; // PWM到角度的映射比例（可调）
float Mini_Pitch_manual_speed = 5.0f;     // 手动控制速度（用于调参模式）
float Mini_Pitch_Islow_incbuf = 0;         // 平滑过渡缓冲变量
uint8_t Mini_Pitch_first_enter_diaoshe = 0; // 首次进入吊射模式标志
uint16_t Mini_Pitch_duzhuan_cnt = 0;
uint8_t Mini_Pitch_E_key_flag = 0;
// uint8_t Mini_Pitch_tune_enable = 0;         // 调参使能：0=电机不输出电流，需上位机命令开启

// huart1调参命令接收（DMA + IDLE + 乒乓缓冲，仿VD_2rx）
#define TUNE_RX_NUM 64
uint8_t tune_rx_buf[2][TUNE_RX_NUM] = {0};
uint8_t tune_FIFO = 0;
uint16_t tune_rx_byte = 0;

// YAW
#define REAL_YAW_REF yaw_cont.continuous_yaw
// #define REAL_YAW_REF Pitch_088.realAngle.yaw
const int YT_Erro = 44480;
#define Yaw_Mouse_Speed 700.0f // Yaw鼠标速度
#define Yaw_YK_GYRO_Speed 6000.0f
#define Yaw_YK_MANG_Speed 28000.0f
#define YAW_LEVEL_MANG 28660
#define YAW_LEVEL_MANG_2 28600 + 32768
// 拨盘射击前馈
#define YAW_SHOOT_FF -400.0f

float YAW_PID_OUT = 0;                   // PID输出，用于发送CAN数据
float Yaw_goal = 0;
uint8_t yaw_control_mode = PROTECT_MODE; // 控制模式，单位度
float Erro_Yaw = 0;                      // 误差，控制模�?6，用于角度误�?
float Buf_Yaw = 0;
float vision_yaw = -2.9;
float yaw_angle_mang;
int32_t diff_2, diff_1;
float Yaw_ab;
float diaoshe_yaw_offset = 0;
float diff_diaoshe;
float YAW_0 = 0.0f;//记录yaw0�?
uint8_t diaoshe_running_flag = 0;
float diaoshe_target_yaw;
float target_absolute_yaw;
uint8_t yaw_0_flag = 0;
float Yaw_X;

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
#define MINI_PITCH_BASE_POSITION 32468  // Mini_Pitch基准位置：对应舵机的1800
#define DIAO_SHE_ENCODER_OFFSET  36700   // 吊射模式目标编码器值（相对堵转零点，可调）
#define DUZHUAN_DIANLIU      (-3000)
#define DUZHUAN_SUDU_YUZHI   30
#define DUZHUAN_QUEREN_CISHU 400
#define MINI_PITCH_TUNE_MIN 0      // 调参行程下限（堵转零点）
#define MINI_PITCH_TUNE_MAX 40000  // 调参行程上限（吊射目标36700，留裕量）

float PITCH_PID_OUT = 0;                   // PID输出，用于发送CAN数据
float Pitch_goal = 0;                      // 目标角度，单位度
uint8_t pitch_control_mode = PROTECT_MODE; // 控制模式，单位度
float pitch_mang_goal = 50000;             // 俯仰角度模式目标
float kk_pitch_mang_lp = 1;
float PitchGyro_AngleError = 0.0f;
float pitch_angle;
float pitch_angle_mang;
float vision_pitch = 33.7;

float vision_distance = 0;
uint8_t Pitch_arr_flag = 0;
float Pitch_X;
// 坐标变换
float theta_angle = 0, theta_rad = 0;
float sin_theta = 0, cos_theta = 0;

// 展开模式标志�?
uint8_t deploy_flag = 0; // 0=关闭 1=展开模式 2=展开旋转
int32_t mini_pitch_deploy_offset = 0;

// 小陀螺标�?
uint8_t SP_Turn_Flag = 0;

// 摩擦轮到地面距离计算
float Pitch_MCL_length = 133.0f;
float Pitch_Ground_length = 397.0f;
float current_mcl_height = 0;

// 自瞄
uint8_t Anti_Time_Flag = 2;
int16_t Anti_Time_1 = 0, Anti_Time_2 = 0;
int16_t ZM_Fire_delay = 834;
float Yaw_ZM,Pitch_ZM;
float receive_freq = 0;  // 接收频率
uint8_t mouse_press_type = 0;          // 0=�? 1=单击长按 2=双击
uint16_t mouse_press_counter = 0;
uint8_t aim_mode = 0x00;
uint8_t is_mouse_single_clicked = 0;

// 重力补偿相关
#define PITCH_GRAVITY_COMP_K 70.0f // 70
#define PITCH_GRAVITY_ANGLE_OFFSET 10.0f
#define PITCH_ENCODER_MID 32768 // 编码器中�?
float gravity_comp;
float gravity_compensation = 0.0f;
float gravity_comp_filtered;

// 望远�?
uint8_t telescope_ON = 0;
// 舵机确认动作相关变量
uint8_t servo_confirm_flag = 0;
uint16_t servo_confirm_timer = 0;
uint16_t servo_original_ccr = 0;
//机器人位置信�?
union
{
  float f_pos;
  uint16_t u16[2];
  uint8_t c[4];
} Robot_pos_u;
#define GYRO_COMBO_DEADZONE 1.5f   // �?螺仪死区
#define GYRO_COMBO_LPF_ALPHA 0.4f  // 低�?�滤�?

float gyro_combo_lpf_last = 0.0f;
float gyro_filtered;

// 自瞄相关定义
uint16_t press_cnt = 0;
uint8_t PRESS_TYPE = 0;
#define NO_ZM_MODE 0
#define OWN_FIRE_MODE 1
#define ZM_FIRE_MODE 2

// int16缓变函数              用于缓慢改变数�?�，避免突变，�?�过增量缓慢改变
void I16_slow(int16_t *in, int16_t target, float add_inc, float cut_inc, int16_t stop_err, float *inc_buf) // int缓变函数
{
  // static float *inc_buf = 0;         //
  // 缓变函数，�?�过增量缓慢改变数�?�，避免突变，�?�过增量缓慢改变，增量大�?1才改�?
  if (abs(*in - target) < stop_err) // 到达目标
  {
    *in = target;
  }
  else
  {
    if (*in < target)
    {
      *inc_buf += add_inc;
    }
    else
    {
      *inc_buf -= cut_inc;
    }

    if (abs(*inc_buf) >= 1) // 增量大于1才改变数�?
    {
      int int_inc;
      int_inc = (int)*inc_buf;
      *in += int_inc;
      *inc_buf -= int_inc;
    }
  }
}
// 整型缓变函数，用于缓慢改变数�?
void I_slow(int *in, int target, float add_inc, float cut_inc, int stop_err, float *inc_buf) // int缓变函数
{
  // static float inc_buf = 0;          //
  // 缓变函数，�?�过增量缓慢改变数�?�，避免突变，�?�过增量缓慢改变，增量大�?1才改�?
  if (abs(*in - target) < stop_err) // 到达目标
  {
    *in = target;
  }
  else
  {
    if (*in < target)
    {
      *inc_buf += add_inc;
    }
    else
    {
      *inc_buf -= cut_inc;
    }

    if (abs(*inc_buf) >= 1) // 增量大于1才改变数�?
    {
      int int_inc;
      int_inc = (int)*inc_buf;
      *in += int_inc;
      *inc_buf -= int_inc;
    }
  }
}
// 缓出型缓变函数：离目标远时快速匀速逼近，接近目标时按比例减速平滑到位
// max_step: 远端最大步长(饱和速度)  min_step: 最小步长(保证最终能走到位)
// k: 减速比例系数(step = 剩余距离 * k)  stop_err: 到达阈值
void I_slow_ease(int *in, int target, float max_step, float min_step, float k, int stop_err, float *inc_buf)
{
  int remaining = target - *in;
  if (abs(remaining) <= stop_err) // 到达目标
  {
    *in = target;
    *inc_buf = 0; // 清空缓冲，避免残留量影响下次调用
    return;
  }

  // 按剩余距离成比例给步长：越近越慢
  float step = (float)remaining * k;

  // 限幅：远端饱和到 max_step，近端保底 min_step，保证最终收敛
  float mag = fabsf(step);
  if (mag > max_step)
    mag = max_step;
  else if (mag < min_step)
    mag = min_step;
  step = (remaining > 0) ? mag : -mag;

  // 沿用增量累积机制，支持亚整数步长推进
  *inc_buf += step;
  if (fabsf(*inc_buf) >= 1)
  {
    int int_inc = (int)*inc_buf;
    // 防止越过目标：本次推进不得超过剩余距离
    if (abs(int_inc) > abs(remaining))
      int_inc = remaining;
    *in += int_inc;
    *inc_buf -= int_inc;
  }
}
// 模式应用�?
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
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_UP) // 底盘低�?�底盘跟�?
  {
    MYmode = DI_PAN_L_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_MID) // 双重，底盘跟随，�?螺仪模式
  {
    MYmode = SHUANG_ZHONG_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_DOWN) // 吊射低�?�底盘跟随，pitch
  {
    MYmode = DIAO_SHE_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_UP) // 底盘高�?�底盘跟�?
  {
    MYmode = DI_PAN_H_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_MID) // 小陀�?
  {
    MYmode = XIAO_TUO_LUO_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_DOWN) // 双重战斗
  {
    MYmode = ZHAN_DOU_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_UP) // 双上保护
  {
    MYmode = PROTECT_MODE;
  }
  else // 其他情况保护模式
  {
    MYmode = PROTECT_MODE;
  }
}
// 摩擦轮�?�辑，返回摩擦轮�?关状�?
uint8_t MCL_Logic()
{
  static int16_t MCL_slow_speed = 0;
  static int16_t Buf_MCL_Speed = 0;
  static float MCL_I16slow_incbuf = 0;
  static UpDown_check_class UD_MCL(0), UD_C(0), UD_X(0), UD_TURN(0);
  static uint8_t now_state;
  //根据模式设置摩擦轮�?�度
  if (MYmode == DIAO_SHE_MODE || deploy_flag)
  {
    MCL_MAX_Speed_Now = MCL_MAX_Speed_Far;  // 5050
  }
  else if (MYmode != PROTECT_MODE) 
  {
    MCL_MAX_Speed_Now = MCL_MAX_Speed_Near;  // 3685
  }
  //保护模式切换摩擦轮�?�度
  if (MYmode == PROTECT_MODE)
  {
    now_state = (YK.yaogan.ch2 < -500 && YK.yaogan.ch3 > 500);
    if (UD_TURN.updata(now_state) == UpDown_check_rising && MCL_ON_flag != 1)
    {
      if (MCL_MAX_Speed_Now == MCL_MAX_Speed_Far)
      {
        MCL_MAX_Speed_Now = MCL_MAX_Speed_Near;
      }
      else
      {
        MCL_MAX_Speed_Now = MCL_MAX_Speed_Far;
      }
    }
  }
	else if(MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
	{
    MCL_ON_flag = (YK.yaogan.ch1 > 600);
	}
  // 键盘控制摩擦轮开�?
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    if (UD_MCL.updata(YK.Pressed_Check(KEY_PRESSED_B)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      MCL_ON_flag = !MCL_ON_flag;
    if (UD_C.updata(YK.Pressed_Check(KEY_PRESSED_C)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed += 50;
    else if (UD_X.updata(YK.Pressed_Check(KEY_PRESSED_X)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed -= 50;
  }
  // 摩擦轮�?�度限制在安全范围内
  if (MYmode == PROTECT_MODE)
  {
    MCL_protect_cansend_200_flag = 1;
    MCL_protect_cansend_1FF_flag = 1;
    MCL_4_motorflag = 0; // 清除电机接收标志�?
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
    MCL_slow_speed = 0;  // 保护模式清零缓变速度
    MCL_I16slow_incbuf = 0;  // 清零缓变增量缓冲�?
  }
  else
  {
    MCL_Start_flag = 1;
  }
  // 计算目标速度
  int16_t MCL_targe_sp = MCL_ON_flag ? (MCL_MAX_Speed_Now + Buf_MCL_Speed) : 0;
  I16_slow(&MCL_slow_speed, MCL_targe_sp, 200, 200, 300, &MCL_I16slow_incbuf);

  // 设置各电机目标�?�度
  if (MCL_Start_flag)
  {
    L_targe_sp = MCL_slow_speed - 14;
    R_targe_sp = MCL_slow_speed - 6;
    UP_targe_sp = MCL_slow_speed - 20;
    RR_targe_sp = MCL_slow_speed - 15;
    LL_targe_sp = MCL_slow_speed - 7;
    UPUP_targe_sp = MCL_slow_speed - 14;
  }
  else // 保护模式时目标�?�度=缓变速度
  {
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
    if (UD_BoPan_ON_buf == UpDown_check_rising && MCL_ON_flag && mouse_press_type != 2) // 鼠标左键按下且摩擦轮�?�?
    {
      Shoot_flag = 1;
      Shoot_time = 0; // 射击计时器清�?
    }
    else if (UD_BoPan_ON_buf == UpDown_check_falling) // 松开
    {
      Shoot_time = 0; // 清零
    }
  }
  // 遥控器拨盘控�?
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
// 拨盘卡弹�?测和�?弹处�? 1khz调用
void BP_KD_TIM(void)
{
  static uint16_t kadan_tui_mang = 6000;
  static int16_t Kadan_cnt = 0; // 卡弹计数�?
  static uint16_t kadan_tim = 0;

  if (BP_MODE == MANG_MODE)
  {
    /*�?测卡弹并�?�?*/
    if (PID_BP_sp.OUT_PID < -14000)
    {
      kadan_tim++;
      if (kadan_tim > 500) // 240
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
  if (MYmode == DI_PAN_H_MODE)
  {
    Mini_Pitch_MODE = MANG_MODE;
    // m2006_cnt++;
    Mini_Pitch_targe += (float)(YK.yaogan.ch3) / Mini_Pitch_manual_speed;
  }
  else if (MYmode == DIAO_SHE_MODE || deploy_flag || Mini_Pitch_E_key_flag || MYmode == SHANG_XIA_MODE)
  {
    Mini_Pitch_MODE = MANG_MODE;
    Mini_Pitch_calc_targe = DIAO_SHE_ENCODER_OFFSET;
    // Mini_Pitch_targe = DIAO_SHE_ENCODER_OFFSET;
    if (Mini_Pitch_first_enter_diaoshe == 0)
      Mini_Pitch_first_enter_diaoshe = 1;
    // 缓出：远离目标快速匀速逼近，接近 DIAO_SHE_ENCODER_OFFSET 时平滑减速到位
    I_slow_ease(&Mini_Pitch_targe, Mini_Pitch_calc_targe, 900, 20, 0.15f, 5, &Mini_Pitch_Islow_incbuf);
  }
  else if (MYmode == SHUANG_ZHONG_MODE || MYmode == ZHAN_DOU_MODE || MYmode == XIAO_TUO_LUO_MODE || MYmode == DAN_YUN_TAI_MODE )
  {
    Mini_Pitch_MODE = MANG_MODE;
    // Mini_Pitch_MODE = PROTECT_MODE;    
    Mini_Pitch_calc_targe = 0;
    Mini_Pitch_first_enter_diaoshe = 0; 
    I_slow(&Mini_Pitch_targe, Mini_Pitch_calc_targe, 900, 900, 50, &Mini_Pitch_Islow_incbuf);
    // I_slow_ease(&Mini_Pitch_targe, Mini_Pitch_calc_targe, 900, 20, 0.05f, 5, &Mini_Pitch_Islow_incbuf);    
  }
  else
  {
    // 预置触发：保护模式 + 右摇杆右下
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
  if (can2_bjtx010) // 发�?�遥控器摇杆数据x
  {
    CAN_2.Send_RM(0x010, YK.yaogan.ch0, YK.yaogan.ch1, YK.yaogan.ch2, YK.yaogan.ch3);
    can2_bjtx010 = 0;
  }
  else if (can2_bjtx011) // 发�?�鼠标yz和键盘数�?
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
    can2bjtx014 = 0; // 发�?�完成标志位
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
  else if (MCL_protect_cansend_1FF_flag)
  {
    CAN_1.Send_RM(0x1FF, 0, 0, 0, 0); // 保护6个摩擦轮
    MCL_protect_cansend_1FF_flag = 0;
  }
}
// 定时�?12回调
void TIM12_Callback(void)
{
 Pitch_088.analyse();
 timeout_imu = 0;  // BMI088数据更新，清零超时计数
 if (abs(Pitch_088.realAngle.pitch) < 2.0f)
  {
   Pitch_088.realAngle.pitch = 0;
  }
}
// Mini PITCH舵机获取PWM�?
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
  // PWM值计�?
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
// 望远镜舵机获取PWM�?
uint16_t Telescope_Get_PWM()
{
  static uint16_t telescope_ccr = 0;
  static const uint16_t bj_en_ccr = 4900;
  static UpDown_check_class UD_E(0);

  // 标志处理
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
    telescope_ccr = 1350;
    // telescope_ccr = 2000;   
  }

  // // E键控制望远镜
  // if (UD_E.updata(YK.Pressed_Check(KEY_PRESSED_E)) == UpDown_check_rising)
  // {
  //   telescope_ON = !telescope_ON;
  // }

  return LIMIT(telescope_ccr, 1150, 4900);
}
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
    if (UD_SET_Y.updata(YK.yaogan.ch0 > 500 && YK.yaogan.ch1 > 500) == UpDown_check_rising)
    {
      YAW_0 = hipnuc_raw.hi91.yaw;
    }   
    else if(UD_SET2_Y.updata(YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500) == UpDown_check_rising)
    {
      yaw_0_flag = 1;
    } 
    yaw_control_mode = PROTECT_MODE;
    Yaw_goal = REAL_YAW_REF;  
    YAW_PID_OUT = 0;
  }
}
void YAW_PID_Calc(void)
{
  static float kk_yaw_mang_lp = 1;
  static float kk_yaw_sp_lp = 1; 
  static UpDown_check_class UD_TURN_Y(0),UD_SET_Y(0);
  if (yaw_control_mode == GYRO_MODE)
  {
    Yaw_goal -= ((float)YK.yaogan.ch2 / Yaw_YK_GYRO_Speed + (float)LIMIT(YK.shubiao.x, -1000, 1000) / Yaw_Mouse_Speed);

    if (request.zimiao_status)  // 自瞄�?�?
    {
      PID_Yaw_mang.Integral = 0;
      PID_Yaw_mang.OUT_I = 0;
      PID_Yaw_sp.Integral = 0;
      PID_Yaw_sp.OUT_I = 0;

      PID_Yaw_mang_zm.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, kk_yaw_mang_lp);
      PID_Yaw_sp_zm.PID_update_LP(PID_Yaw_mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[2], kk_yaw_sp_lp);
      YAW_PID_OUT = -PID_Yaw_sp_zm.OUT_PID;
    }
    else  // 自瞄关闭
    {
      PID_Yaw_mang_zm.Integral = 0;
      PID_Yaw_mang_zm.OUT_I = 0;
      PID_Yaw_sp_zm.Integral = 0;
      PID_Yaw_sp_zm.OUT_I = 0;

      PID_Yaw_mang.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, kk_yaw_mang_lp);
      PID_Yaw_sp.PID_update_LP(PID_Yaw_mang.OUT_PID, hipnuc_raw.hi91.gyr[2], kk_yaw_sp_lp);
      YAW_PID_OUT = -PID_Yaw_sp.OUT_PID;
    }
  }
  else if (yaw_control_mode == MANG_MODE)
  {
    if (UD_TURN_Y.updata(YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500) == UpDown_check_rising)
    {
        // 计算当前连续yaw角度�?-180~+180范围
        float current_angle = fmodf(yaw_cont.continuous_yaw, 360.0f);
        if (current_angle > 180.0f) current_angle -= 360.0f;
        if (current_angle < -180.0f) current_angle += 360.0f;

        // 目标角度
        float target_angle = YAW_0 - vision_yaw;
        if (target_angle > 180.0f) target_angle -= 360.0f;
        if (target_angle < -180.0f) target_angle += 360.0f;

        // 计算�?短旋转偏移量
        float offset = target_angle - current_angle;
        if (offset > 180.0f) offset -= 360.0f;
        if (offset < -180.0f) offset += 360.0f;

        // 单击偏移量应用到连续yaw角度
        Yaw_goal = yaw_cont.continuous_yaw + offset;
        
//        PID_YAW_TURN_MANG.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, 0.2);
//        PID_YAW_TURN_GYRO.PID_update(PID_YAW_TURN_MANG.OUT_PID, hipnuc_raw.hi91.gyr[2]);
    }
    // if(diaoshe_running_flag)
    // {
			  // PID_YAW_TURN_MANG.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, 1);
				// PID_YAW_TURN_GYRO.PID_update(PID_YAW_TURN_MANG.OUT_PID,  hipnuc_raw.hi91.gyr[2]);		
        // diaoshe_yaw_offset = vision_yaw - (Yaw_ab - YAW_0);
        // if (fabs(diaoshe_yaw_offset) < 1.0f)
        // {
        //   diaoshe_running_flag = 0;  
        // }
    // }
    else
    {
				Yaw_goal -= ((float)YK.yaogan.ch2 / Yaw_YK_MANG_Speed) + ((YK.Pressed_Check(KEY_PRESSED_D) - YK.Pressed_Check(KEY_PRESSED_A)) * 0.002f);
			  PID_YAW_Erro_IMU_MANG.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, 1);
				// PID_YAW_Erro_IMU_GYRO.PID_update(PID_YAW_Erro_IMU_MANG.OUT_PID,  hipnuc_raw.hi91.gyr[2]);
        PID_YAW_Erro_IMU_GYRO.PID_update(PID_YAW_Erro_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[2]);
    }
    diaoshe_target_yaw = YAW_0 - hipnuc_raw.hi91.yaw; 
    YAW_PID_OUT = -PID_YAW_Erro_IMU_GYRO.OUT_PID;
  }  
  else//28215�?61050 
  {
    if(yaw_0_flag && (Motor_LK6010_Yaw.mang_inf % 28215 == 0 || Motor_LK6010_Yaw.mang_inf % 61050 == 0))
    {
      YAW_0 = hipnuc_raw.hi91.yaw;
      yaw_0_flag = 0;
      // 触发舵机确认动作
      servo_confirm_flag = 1;
      servo_confirm_timer = 0;
      servo_original_ccr = Telescope_Get_PWM(); // 保存当前舵机�?
    }
    Yaw_goal = yaw_cont.continuous_yaw;
    YAW_PID_OUT = 0;
  }
}
// YAW角度更新，接收CAN数据后调�?
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
void PITCH_Logic(void)
{
  static uint8_t now_state_pitch;
  static UpDown_check_class UD_TURN_P(0);
  
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
    if (UD_TURN_P.updata(YK.yaogan.ch0 > 500 && YK.yaogan.ch1 < -500) == UpDown_check_rising)
    {
      Pitch_goal = -vision_pitch;
    }
  }
  else
  {
    pitch_control_mode = PROTECT_MODE;
    Pitch_goal = hipnuc_raw.hi91.pitch;
  }
}
// 重力补偿计算，根据pitch角度计算
float PITCH_Gravity_Compensation(float pitch_angle)
{
  float pitch_rad = (pitch_angle + PITCH_GRAVITY_ANGLE_OFFSET) * 0.0174533f; // 角度转弧�?
  gravity_comp = -PITCH_GRAVITY_COMP_K * cosf(pitch_rad);
  gravity_comp_filtered += 1.0f * (gravity_comp - gravity_comp_filtered);

  return gravity_comp_filtered;
}
void PITCH_PID_Calc(void)
{
  if (pitch_control_mode == GYRO_MODE)
  {
    Pitch_goal -= (float)YK.yaogan.ch3 / Pitch_YK_Speed + ((float)LIMIT(YK.shubiao.y, -500, 500) / Pitch_Mouse_Speed);
    Pitch_goal = LIMIT(Pitch_goal, -44, 16);
    gravity_compensation = PITCH_Gravity_Compensation(hipnuc_raw.hi91.pitch);

    if (request.zimiao_status)  // 自瞄�?�?
    {
      PID_LK_Pitch_Mang.Integral = 0;
      PID_LK_Pitch_Mang.OUT_I = 0;
      PID_LK_Pitch_SP.Integral = 0;
      PID_LK_Pitch_SP.OUT_I = 0;

      PID_LK_Pitch_Mang_zm.PID_update_LP(Pitch_goal, hipnuc_raw.hi91.pitch, kk_pitch_mang_lp);
      PID_LK_Pitch_SP_zm.PID_update(PID_LK_Pitch_Mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[0]);
      // PITCH_PID_OUT = PID_LK_Pitch_SP_zm.OUT_PID + gravity_compensation;
      PITCH_PID_OUT = -PID_LK_Pitch_SP_zm.OUT_PID;      
    }
    else  // 自瞄关闭
    {
      PID_LK_Pitch_Mang_zm.Integral = 0;
      PID_LK_Pitch_Mang_zm.OUT_I = 0;
      PID_LK_Pitch_SP_zm.Integral = 0;
      PID_LK_Pitch_SP_zm.OUT_I = 0;

      PID_LK_Pitch_Mang.PID_update_LP(Pitch_goal, hipnuc_raw.hi91.pitch, kk_pitch_mang_lp);
      PID_LK_Pitch_SP.PID_update(PID_LK_Pitch_Mang.OUT_PID, hipnuc_raw.hi91.gyr[0]);
      // PITCH_PID_OUT = PID_LK_Pitch_SP.OUT_PID + gravity_compensation;
      PITCH_PID_OUT = -PID_LK_Pitch_SP.OUT_PID;
    }
  }
  else if (pitch_control_mode == MANG_MODE)
  {
    float ws_pitch = YK.Pressed_Check(KEY_PRESSED_CTRL) ? 0.0f : (YK.Pressed_Check(KEY_PRESSED_W) - YK.Pressed_Check(KEY_PRESSED_S)) * 0.002f;
    Pitch_goal -= ((float)(YK.yaogan.ch3 / Pitch_YK_MANG_Speed) + ws_pitch);
    Pitch_goal = LIMIT(Pitch_goal, -43, 16);
    gravity_compensation = PITCH_Gravity_Compensation(hipnuc_raw.hi91.pitch);
    PID_LK_Erro_Pitch_IMU_MANG.PID_update_LP(Pitch_goal, hipnuc_raw.hi91.pitch, 1);
    PID_LK_Erro_Pitch_IMU_Gyro.PID_update(PID_LK_Erro_Pitch_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[0]);
    // PITCH_PID_OUT = PID_LK_Erro_Pitch_IMU_Gyro.OUT_PID + gravity_compensation;
    PITCH_PID_OUT = -PID_LK_Erro_Pitch_IMU_Gyro.OUT_PID;
  }
  else
  {
    Pitch_goal = REAL_PITCH_REF;
    PITCH_PID_OUT = 0;
  }
}
// PITCH角度更新，接收CAN数据后调�?
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
  // uint16_t mini_pitch_ccr = Mini_PITCH_Get_PWM();  // 已禁用：现在使用2006电机

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, telescope_ccr);
  // __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, mini_pitch_ccr);  // 已禁用
}
// 键盘特殊功能处理
void Keyboard_Special_Func()
{
  static UpDown_check_class UD_XTL(0), UD_R(0), UD_Deploy_Turn(0), UD_Arr_PY(0), UD_Boost(0), UD_MCL_Speed_Turn(0),UD_Z(0),UD_X(0),UD_C(0);
  static int targe_f = YT_Erro;
  static uint8_t mini_pitch_anjian_flag = 0;

  /*键盘控制使能标志*/ // 根据模式决定是否启用键盘控制功能(防止误触导致意外操作)
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    jianshu_ctrl_flag = ENABLE_MODE;
  }
  else
  {
    jianshu_ctrl_flag = DISABLE_MODE;
  }
  /**按键功能 */
  if (jianshu_ctrl_flag == ENABLE_MODE)
  {
    // Q�? - 小陀�?
    if (UD_XTL.updata(YK.Pressed_Check(KEY_PRESSED_Q)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
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
    // G�? - boost
    if (UD_Boost.updata(boost_flag) == UpDown_check_rising)
    {
      mini_pitch_anjian_flag = 0;
      Pitch_goal = 0;
      telescope_ON = 0;
    }
    // R�? - 展开模式切换
    if (UD_R.updata(YK.Pressed_Check(KEY_PRESSED_R)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
    {
      deploy_flag = !deploy_flag;
      mini_pitch_anjian_flag = 0;
      telescope_ON = deploy_flag ? 1 : 0;
      // Pitch_goal = -36.5f;
    }
    // F�? - 展开模式旋转180�?
    if (UD_Deploy_Turn.updata(YK.Pressed_Check(KEY_PRESSED_F)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL) && deploy_flag)
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
    // �?测旋转到位，恢复展开标志
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
  }
  else
  {
    deploy_flag = 0;
    XTL_flag = 0;
    mini_pitch_anjian_flag = 0;
    mini_pitch_deploy_offset = 0;
  }
}
// Mini pitch鼠标调整�?160Hz调用
void Mini_PITCH_Mouse_Adjust(void)
{
  static float mini_pitch_buf_ccr = 0;
  static uint8_t mini_pitch_ctrl_flag = 0;

  if (mini_pitch_ctrl_flag == 1)
  {
    mini_pitch_buf_ccr -= YK.shubiao.y / 40.0f;
  }
}
// 展开模式超时�?查，160Hz调用
void Deploy_Timeout_Check(void)
{
  static int16_t deploy_out_tim = 0;
  if (deploy_flag == 2)
  {
    deploy_out_tim++;
    if (deploy_out_tim > 160)
    {
      deploy_flag = 0; // 保护，超时恢复标�?
      deploy_out_tim = 0;
    }
  }
  else
  {
    deploy_out_tim = 0;
  }
}
// 复位函数
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

    Laser_OFF;
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
    NVIC_SystemReset(); // 复位
  }
}
//自瞄�?�?
void ZM_Check(void)
{
  static UpDown_check_class UD_ZM_Fire(0), UD_FIRE(0);
  static uint16_t mouse_release_counter = 0;

  #define MOUSE_SHORT_CLICK_TIME 300
  #define MOUSE_LONG_PRESS_TIME 700
  #define MOUSE_CLICK_GAP_TIME 700

  UpDown_check_state mouse_edge = UD_ZM_Fire.updata(YK.shubiao.press_r);

  // 鼠标右键上升沿，�?测单击或双击
  if (mouse_edge == UpDown_check_rising)
  {
    mouse_press_counter = 0;

    if (is_mouse_single_clicked && mouse_release_counter < MOUSE_CLICK_GAP_TIME)
    {
      mouse_press_type = 2;  // 双击
    }
    else
    {
      mouse_press_type = 1;  // 单击长按
      is_mouse_single_clicked = 0;  // 清除单击标志
    } 
  }   

  // 松开鼠标上升沿，�?测短�?
  else if (mouse_edge == UpDown_check_falling)
  {
    mouse_release_counter = 0;

    // 判断是否为短按单�?
    if (mouse_press_counter < MOUSE_SHORT_CLICK_TIME)
    {
      is_mouse_single_clicked = 1;  // 标记单击状�??
    }
    
    mouse_press_type = 0;
  }
  //计数�?
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
    if(SuperPower.mode == 0 && request.zimiao_status) // 没有识别到
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
      else if(!ZM_Fire_delay && YK.yaogan.ch0 > 500 && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL))) //手打
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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, Laser_State); // �?光开关控�?
}
// 自瞄保护模式处理
void ZM_Protect_Mode_Handle(void)
{
  if (MYmode == PROTECT_MODE)
  {
    request.zimiao_status = 0;
    // 自瞄保护，遥控器特定操作控制PY�?�?
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 > 500)
    {
      Laser_deal(GPIO_PIN_SET);
    }
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 < -500)
    {
      Laser_deal(GPIO_PIN_RESET); // 关闭�?�?
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
  if ((__HAL_UART_GET_FLAG(&MINI_PC_USART_HANDLE, UART_FLAG_IDLE) != RESET)) // �?测到空闲中断，UART接收完成，串口空闲中断，�?测到�?帧数据接收完�?
  {

    __HAL_UART_CLEAR_IDLEFLAG(&MINI_PC_USART_HANDLE); // 清除空闲中断标志�?
    temp = MINI_PC_USART_HANDLE.Instance->SR;
    temp = MINI_PC_USART_HANDLE.Instance->DR;
    HAL_UART_DMAStop(&MINI_PC_USART_HANDLE);                                     // 停止DMA传输，防止数据覆�?
    // getReceiveData_ZM(Mini_PC_rx_buf);
    GetReceive_SP(Mini_PC_rx_buf);                                         // 处理接收到的数据，从Mini_PC_rx_buf中提取有效信�?
    HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_rx_buf, 128); // 重新启动DMA接收，准备接收下�?帧数据，�?�?128字节
    // if (isnan(response.yaw.f))
    //   response.yaw.f = 0.0; // 如果视觉数据为NaN，则将其设置�?0，防止异常�?�影响控�?
    // if (isnan(response.pitch.f))
    //   response.pitch.f = 0.0; // 同上
    /*************** Your code *****************/

    if (Mini_PC_rx_buf[0] == 0x66 && Mini_PC_rx_buf[28] == 0x11) // �?查数据帧头尾是否�?0x66，验证数据有效�??
    {
      if(counter<= 1000)
      {
          zm_cnt++;
      }
      if (request.zimiao_status)
      {
        aa++;
        // Yaw_goal =  response.yaw.f;
        // Pitch_goal = -response.pitch.f;
        // Yaw_goal = (SuperPower.yaw.f*180.0 / PI);
        float zm_yaw_raw = SuperPower.yaw.f * 180.0f / PI;  // -180 ~ +180
        // 1. 计算当前连续yaw角度�?-180~+180范围
        float current_mod = fmodf(yaw_cont.continuous_yaw, 360.0f);
        if (current_mod > 180.0f)  current_mod -= 360.0f;
        if (current_mod < -180.0f) current_mod += 360.0f;
        // 2. 计算视觉目标与当前角�?-180~+180范围内的�?短偏�?
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

// ========== RGB Debug 初始化相关函数 ==========

/**
 * @brief LED 4 自瞄状态更新函数（自定义回调）
 * @note 自瞄有三种状态：绿色（关闭）、蓝色（自瞄火控）、红色（操作手火控）
 */
void RGB_LED4_Zimiao_Update(void)
{
    if (request.zimiao_status == 0)
    {
        RGB_UI.WS281x_SetPixelRGB(4, 0, RGB_goal, 0);   // 绿色：自瞄关闭
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
    // ========== 配置 LED 映射表 ==========

    // LED 0: Pitch/Yaw电机、云台IMU
    led_mappings[0].led_index = 0;
    led_mappings[0].device_count = 3;
    // Pitch电机（LK6010）
    led_mappings[0].devices[0].timeout_ptr = &timeout_pitch;
    led_mappings[0].devices[0].timeout_threshold = 10;  // 200ms超时
    led_mappings[0].devices[0].status_flag_ptr = NULL;
    // Yaw电机（LK6010）
    led_mappings[0].devices[1].timeout_ptr = &timeout_yaw;
    led_mappings[0].devices[1].timeout_threshold = 10;
    led_mappings[0].devices[1].status_flag_ptr = NULL;
    // 云台IMU（BMI088）
    led_mappings[0].devices[2].timeout_ptr = &timeout_imu;
    led_mappings[0].devices[2].timeout_threshold = 10;
    led_mappings[0].devices[2].status_flag_ptr = NULL;
    // 故障颜色配置
    led_mappings[0].color_modes[0] = RED_MODE;        // Pitch故障：红闪
    led_mappings[0].color_modes[1] = GREEN_MODE;      // Yaw故障：绿闪
    led_mappings[0].color_modes[2] = RED_GREEN_MODE;  // IMU故障：红绿交替

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

    // LED 4: 裁判系统
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

    // LED 0: 小陀螺状态
    Function_Status_t xtl_status;
    xtl_status.led_index = 0;
    xtl_status.flag_ptr = &XTL_flag;
    xtl_status.on_color[0] = RGB_goal;  // 开启：红色
    xtl_status.on_color[1] = 0;
    xtl_status.on_color[2] = 0;
    xtl_status.off_color[0] = 0;        // 关闭：绿色
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
    mcl_status.off_color[0] = 0;        // 关闭：绿色
    mcl_status.off_color[1] = RGB_goal;
    mcl_status.off_color[2] = 0;
    RGB_Control_Register_Function(mcl_status);

    // LED 2: 吊射状态（替换26英雄的上台阶）
    Function_Status_t diaoshe_status;
    diaoshe_status.led_index = 2;
    diaoshe_status.flag_ptr = &deploy_flag;  // 使用deploy_flag
    diaoshe_status.on_color[0] = RGB_goal;   // 开启：红色
    diaoshe_status.on_color[1] = 0;
    diaoshe_status.on_color[2] = 0;
    diaoshe_status.off_color[0] = 0;         // 关闭：绿色
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
    boost_status.off_color[0] = 0;        // 关闭：绿色
    boost_status.off_color[1] = RGB_goal;
    boost_status.off_color[2] = 0;
    RGB_Control_Register_Function(boost_status);

    // LED 4: 自瞄状态（特殊处理，使用自定义回调）
    RGB_Control_Register_LED4_Callback(RGB_LED4_Zimiao_Update);
}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// 原视觉滤波/防陀螺全局变量(Vision_process/extKalman/LOW_Pass_Filter/moving_Average_Filter/TOP_Data/Kf 实例)
// 均为死代码(全工程零活跃读写),随 my_math 模块一并移除。
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern uint8_t bb;
extern uint8_t uart4_recbuf[32];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM7_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_TIM1_Init();
  MX_TIM9_Init();
  MX_USART1_UART_Init();
  MX_TIM12_Init();
  MX_TIM6_Init();
  MX_UART4_Init();
  MX_TIM8_Init();
  MX_UART5_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();
  MX_TIM11_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  V_Yaw.Vision_Low_Pass_Filter_Init();
  V_Pitch.Vision_Low_Pass_Filter_Init();
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
  HAL_TIM_Base_Start_IT(&htim1); // 舵机定时�?
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim9); // 2khz主控，can2发�??
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim7);
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim8); // 40hz 0x13id发�??
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim6);//10hz遥控�?
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim5);//800hz
  HAL_Delay(0);	
  HAL_TIM_Base_Start_IT(&htim12); /**********�?螺仪读取************/
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  Mini_PC_UART_Init();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); // 使能电机驱动
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(1500);
  Mini_Pitch_MODE = DUZHUAN_MODE;
	IMU_UART_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN 
	ILE */
  while (1)
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
   INFO("%.2f,%.2f\r\n", Yaw_goal, yaw_cont.continuous_yaw);
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
/*CH010陀螺仪四元数*/    
		// INFO("%.2f,%.2f,%.2f,%.2f\r\n", hipnuc_raw.hi91.quat[0],hipnuc_raw.hi91.quat[1],hipnuc_raw.hi91.quat[2],hipnuc_raw.hi91.quat[3]);
/*CH010陀螺仪角速度*/    
    // INFO("%.2f\r\n",hipnuc_raw.hi91.gyr[0]);
/*鼠标数据*/
    // INFO("%d,%d\r\n", YK.shubiao.x, YK.shubiao.y);
/*热量标志*/
    // INFO("%d\r\n", heat_flag);
/*枪口位置信息*/
	  // INFO("%.2f\r\n", Robot_pos_u.f_pos);，
/*2006小pitch内外环目标实际*/
  //  INFO("%d,%d\r\n",Mini_Pitch_targe, Mini_Pitch_2006.mang_inf);
		//  INFO("%.2f,%d\r\n",PID_Mini_Pitch_2006_mang.OUT_PID, Mini_Pitch_2006.sp);
/*红蓝方*/
    // INFO("%d\r\n", mine_flag);
//    INFO("%.2f,%.2f\r\n", hipnuc_raw.hi91.pitch, yaw_cont.continuous_yaw);
//    INFO("ok\r\n");
    // INFO("%d\r\n", duoji);
    HAL_Delay(10);

    // 模式应用�?
    MYMODE_while_application_layer();

    // �?启摩擦轮逻辑，返回摩擦轮是否启动标志
    uint8_t mcl_on = MCL_Logic();

    // 拨盘逻辑，根据摩擦轮状�?�决定是否拨�?
    BP_Logic(mcl_on);

    // Mini_Pitch_2006控制逻辑
    Mini_Pitch_2006_Logic();

    // 舵机PWM输出（仅telescope，mini_pitch已改用2006电机）
    Servo_PWM_Output();

    // 键盘特殊功能处理
    Keyboard_Special_Func();

    // PITCH轴控制�?�辑
    PITCH_Logic();

    // YAW轴控制�?�辑
    YAW_Logic();

    // 掉头处理函数
    YAW_Turn_Handle();

    // 复位函数
    Reset();

    // 云台板发送标志位给底盘板
    YT_Tx_static_Flag = (bool)deploy_flag ? (YT_Tx_static_Flag | 0x0001) : (YT_Tx_static_Flag & (uint16_t)~1);
    YT_Tx_static_Flag = XTL_flag ? (YT_Tx_static_Flag | 0x0002) : (YT_Tx_static_Flag & (uint16_t)~2);
    YT_Tx_static_Flag = SP_Turn_Flag ? (YT_Tx_static_Flag | SP_TURN_FLAG) : (YT_Tx_static_Flag & ~SP_TURN_FLAG);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/*******************************************************************************************************
 * can1 中断 6个摩擦轮电机
 * ************************************************************************************************/

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // 8khz
{
     if (CAN_1.Receive(CAN_RX_FIFO0) == HAL_OK && MCL_Start_flag) //
  //    接收到can1数据后进行处�?// �?启后会导致拨盘不转，可能是因为拨盘电机在这里更新了pid
  // if (CAN_1.Receive(CAN_RX_FIFO0) == HAL_OK)
  {
    if (Motor_MCL_UP_up.update() == HAL_OK) // 1khz
    {
      timeout_mcl_upup = 0;  // 清零超时计数
      MCL_4_motorflag |= 0x1;
      PID_MCL_UP_sp.PID_update(-UP_targe_sp, Motor_MCL_UP_up.sp);
    } //-

    else if (Motor_MCL_R.update() == HAL_OK) // 1khz
    {
      timeout_mcl_r = 0;  // 清零超时计数
      MCL_4_motorflag |= 0x2;
      PID_MCL_R_sp.PID_update(-R_targe_sp, Motor_MCL_R.sp);
    } //-

    else if (Motor_MCL_L.update() == HAL_OK)
    {
      timeout_mcl_l = 0;  // 清零超时计数
      MCL_4_motorflag |= 0x4;
      PID_MCL_L_sp.PID_update(L_targe_sp, Motor_MCL_L.sp);
    } //+

    else if (Motor_MCL_RR.update() == HAL_OK)
    {
      timeout_mcl_rr = 0;  // 清零超时计数
      MCL_4_motorflag |= 0x8;
      PID_MCL_RR_sp.PID_update(-RR_targe_sp, Motor_MCL_RR.sp);
    } //-

    else if (Motor_MCL_UP.update() == HAL_OK)
    {
      timeout_mcl_up = 0;  // 清零超时计数
      MCL_2_motorflag |= 0x1;
      PID_MCL_UPUP_sp.PID_update(-UPUP_targe_sp, Motor_MCL_UP.sp);
    } //-

    else if (Motor_MCL_LL.update() == HAL_OK)
    {
      timeout_mcl_ll = 0;  // 清零超时计数
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
       CAN_1.Send_RM(0x1FF, PID_MCL_UPUP_sp.OUT_PID, PID_MCL_RR_sp.OUT_PID, 0, 0);
//     CAN_1.Send_RM(0x1FF, PID_MCL_UPUP_sp.OUT_PID, 0, 0, 0);
     MCL_2_motorflag = 0;
     MCL_MID = (Motor_MCL_L.sp - Motor_MCL_R.sp - Motor_MCL_UP_up.sp + Motor_MCL_LL.sp - Motor_MCL_UP.sp - Motor_MCL_RR.sp) / 6; // 摩擦轮平均转速，用于判断摩擦�?
   }
  }
}

/*******************************************************************************************************
 * can2  中断接收 yaw pitch  拨盘tx
 * ************************************************************************************************/

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) // 4khz
{
  static int16_t BP_output = 0;
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
      timeout_yaw = 0;  // 清零超时计数
      YAW_Angle_Update();
    }
    else if (Motor_LK6010_Pitch.LK_Broadcast_update() == HAL_OK)
    {
      timeout_pitch = 0;  // 清零超时计数
      PITCH_Angle_Update();
    }
    else if (BoPan.update() == HAL_OK)
    {
      timeout_bopan = 0;  // 清零超时计数
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
    }
    else if(Mini_Pitch_2006.update() == HAL_OK)
    {
      m2006_cnt++;
      timeout_mini_pitch = 0;  // 清零超时计数
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
    switch (CAN_2.RxHeader.StdId)
    {
    case (0x012):
      DP_Tx_static_Flag = CAN_2.rx_buf[0] << 8 | CAN_2.rx_buf[1];
      turn_flag = DP_Tx_static_Flag & TURN_FLAG;
      mine_flag = DP_Tx_static_Flag & MINE_FLAG;
      heat_flag = DP_Tx_static_Flag & HEAT_FLAG;
      boost_flag = DP_Tx_static_Flag & BOOST_FLAG;
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
      Yaw_ab = Robot_pos_u.f_pos;
      if (Yaw_ab > 180.0f)
        Yaw_ab -= 360.0f;
      else if (Yaw_ab < -180.0f)
        Yaw_ab += 360.0f;
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
    CAN_2.Send_RM(0x200, BP_output, Mini_Pitch_output, 0, 0);
    // CAN_2.Send_RM(0x200, BP_output, 0, 0, 0);    
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim == &htim12) // 1khz
  {
    TIM12_Callback();
  }
  if (htim == &htim9) // 2kHz
  {
    static uint8_t can_scan = 1;
    if (can_scan)
    {
      // YAW和PITCH电机PID计算
      YAW_PID_Calc();
      PITCH_PID_Calc();

      // 发�?�摩擦轮数据
//      CAN_2.Broadcast_Send_LK(0, YAW_PID_OUT, 0, 0);
      CAN_2.Broadcast_Send_LK(PITCH_PID_OUT, YAW_PID_OUT, 0, 0);			
      can_scan = 0;
    }
    else
    {
      // 自瞄控制
       ZM_Control();
      // 拨盘逻辑处理，根据模式调�?
      // 拨盘角度模式时，根据展开状�?�调整增量缓变�?�度，防止突变导致卡�?
      if (BP_MODE == MANG_MODE)
      {
        if (deploy_flag || MYmode == DIAO_SHE_MODE || MYmode == SHANG_XIA_MODE)
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

    // 展开模式超时�?�?
    Deploy_Timeout_Check();

    // 舵机确认动作相关
    if(servo_confirm_flag)
    {
        servo_confirm_timer++;

        if(servo_confirm_timer < 30)  // �?30个周期，�?0.19�? - 抬起舵机
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr + 900);
        }
        else if(servo_confirm_timer < 60)  // 30-60个周期，�?0.19�? - 恢复原位
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr);
        }
        else  // 结束动作
        {
            servo_confirm_flag = 0;
            servo_confirm_timer = 0;
        }
    }

    // 遥控器数�?(交替)
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
      Erro_Yaw = Motor_LK6010_Yaw.mang_inf;
      PID_YAW_Erro_IMU_MANG.Integral = 0;
      PID_YAW_Erro_IMU_MANG.OUT_I = 0;
    }

    // PITCH保护模式
    if (pitch_control_mode == PROTECT_MODE)
    {
      Pitch_goal = REAL_PITCH_REF;
      pitch_mang_goal = Motor_LK6010_Pitch.mang;
    }
  }

  else if (htim == &htim8) // 20HZ
  {
    can2bjtx013 = 1;
    can2bjtx014 = 1;
  }

  else if (htim == &htim6) // 10hz 看门狗定时器
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

      request.Yaw_Angle.f = V_Yaw.Vision_Low_Pass_Filter(yaw_cont.continuous_yaw);
      request.Pitch_Angle.f = V_Pitch.Vision_Low_Pass_Filter(-hipnuc_raw.hi91.pitch);
      request.Yaw_Anglespeed.f = hipnuc_raw.hi91.gyr[1];
      //q4

      if (MYmode == ZHAN_DOU_MODE && YK.shubiao.press_r)
      {
          if (mouse_press_type == 1 && mouse_press_counter >= MOUSE_LONG_PRESS_TIME)
          {
              aim_mode = 0x01;  // 单击长按 = 启用自瞄模式
          }
          else if (mouse_press_type == 2 && mouse_press_counter >= 100)
          {
              aim_mode = 0x01;  // 双击 = 启用自瞄模式
          }
      }
      else if(MYmode == ZHAN_DOU_MODE && !YK.shubiao.press_r)
      {
        aim_mode = 0x00;
      }

      Mini_PC_SendData_ZM2(aim_mode);  // 发�?�自瞄模式数�?
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
