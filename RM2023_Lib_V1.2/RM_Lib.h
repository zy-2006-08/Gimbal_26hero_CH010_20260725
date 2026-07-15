/**
  ******************************************************************************
  * File Name				: RM_Lib.hpp
  * Description			: RM c++库，包含USART，CAN，麦轮底盘算法，PID，遥控器，
  陀螺仪，snail电调，裁判系统交互，达妙4310，瓴控6010等库函数
  * Version					: v1.4
  * Creation Date		: 2025.4.12
  ******************************************************************************
  */

#ifndef __RM_LIB_H
#define __RM_LIB_H
#include "can.h" // 这行报错请屏蔽
#include "main.h"
#include "math.h"
#include "my_math.h"
#include "spi.h" // 这行报错请屏蔽
#include "stdio.h"
#include "string.h"
#include "tim.h"   // 这行报错请屏蔽
#include "usart.h" // 这行报错请屏蔽
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**************************************** USART **********************************************************/
#define USART_BUF_SIZE      128    // 数组大小，可修改
#define PRINTF_USART_HANDLE huart1 // 串口号，可修改,英雄是串口1
extern uint8_t info_ubuf[USART_BUF_SIZE];
#define INFO(...) HAL_UART_Transmit(&PRINTF_USART_HANDLE, (uint8_t *)info_ubuf, sprintf((char *)info_ubuf, __VA_ARGS__), 0xffff)

// uint8_t INFO_DMA(const char *fmt, ...);

/**************************************** CRC **********************************************************/
extern const uint16_t crctab16[];
extern uint16_t GetCrc16(const unsigned char *pData, uint16_t nLength); // 计算给定长度数据的16位CRC。
extern bool IsCrc16Good(const unsigned char *pData, uint16_t nLength);  // 检查给定长度数据的16位CRC是否正确。

/**************************************** C A N **********************************************************/
class USER_CAN
{
  public:
  CAN_HandleTypeDef *hcan;
  CAN_TxHeaderTypeDef TxHeader;
  CAN_RxHeaderTypeDef RxHeader;
  uint8_t rx_buf[8]; // 给RMD，RM，DM，LK电机接收数据使用
  uint8_t tx_buf[8]; // CAN发送数据使用
  uint32_t FIFO;
  uint8_t FreeTxNum; // 空闲发送邮箱个数，最多3个
  uint32_t TxMailbox;
  uint32_t can_send_busy_cnt          = 0; // 空闲邮箱队列满溢出总计次，也可能是因无法发送导致的
  uint32_t can_send_error_cnt         = 0; // 是否有can发送错误总计次，一直发送失败导致空闲邮箱队列满溢出后，发送错误计次可能不会增加
  uint32_t can_user_error_cnt         = 0; // 接收fifo的信息和回调函数不对应/canid位溢出，一般为自身代码问题导致的错误
  uint32_t can_send_RM_error_cnt      = 0; // 大疆电机一拖四发送错误计次
  uint32_t can_send_LK_error_cnt      = 0; // 领控电机广播模式发送错误计次
  uint32_t can_send_Xbit_error_cnt    = 0; // 板件通信错误计次
  uint32_t can_getRxMessage_error_cnt = 0; // 接收错误计次

  HAL_StatusTypeDef can_send_error_state         = HAL_TIMEOUT; // 发送错误状态记录/初始化成功标志
  HAL_StatusTypeDef can_getRxMessage_error_state = HAL_TIMEOUT; // 接收错误状态记录/是否有数据接收标志
  HAL_StatusTypeDef motor_send_error_state       = HAL_TIMEOUT; // 广播模式电机发送状态记录，然后计次

  void Init(uint16_t t, uint16_t x);
  HAL_StatusTypeDef Send8Bit(uint32_t CAN_ID_TYPE, uint32_t Id, uint8_t uint8_d1, uint8_t uint8_d2, uint8_t uint8_d3, uint8_t uint8_d4, uint8_t uint8_d5, uint8_t uint8_d6, uint8_t uint8_d7,
                             uint8_t uint8_d8);
  HAL_StatusTypeDef Send16Bit(uint32_t CAN_ID_TYPE, uint32_t Id, uint16_t uint16_d1, uint16_t uint16_d2, uint16_t uint16_d3, uint16_t uint16_d4);
  HAL_StatusTypeDef Send32Bit(uint32_t CAN_ID_TYPE, uint32_t Id, uint32_t uint32_d1, uint32_t uint32_d2);
  HAL_StatusTypeDef Send64Bit(uint32_t CAN_ID_TYPE, uint32_t Id, uint64_t uint64_data);
  HAL_StatusTypeDef Send_RM(uint16_t Id, int16_t M_201, int16_t M_202, int16_t M_203, int16_t M_204); // 大疆电机1拖4模式
  HAL_StatusTypeDef Broadcast_Send_LK(int16_t M_201, int16_t M_202, int16_t M_203, int16_t M_204);    // 领控电机广播模式,1拖4，不可使用Send_RM()

  HAL_StatusTypeDef STD_ID_Send(uint16_t Id, uint8_t *pData);
  HAL_StatusTypeDef EXT_ID_Send(uint32_t Id, uint8_t *pData);
  HAL_StatusTypeDef Receive(uint32_t fifo);

  USER_CAN(CAN_HandleTypeDef *p, uint32_t fifo) : hcan(p), FIFO(fifo) {}

  private:
};
#ifndef PI
////#define PI 3.14159
#endif
/*************************************  RM电机  ************************************************/
class MOTOR_RM
{
  public:
  const uint16_t ID; // 电机反馈ID
  USER_CAN *can_rev;

  int16_t mang;       // (int16_t) ((Can1_Data[0] << 8) | Can1_Data[1]);
  int16_t sp;         // (int16_t) ((Can1_Data[2] << 8) | Can1_Data[3]);
  int16_t AT_current; // (int16_t) ((Can1_Data[4] << 8) | Can1_Data[5]);
  int8_t temp;        //  Can1_Data[6];

  int mang_inf;
  int motor_number;
  int first_mang_inc; // 第一次上电或者被初始化的编码器值数据
  uint8_t first = 0;
  // HAL_StatusTypeDef motor_send_state=HAL_TIMEOUT; // 电机发送状态/是否有调用标志
  HAL_StatusTypeDef update(void);
  void update_mang_inf(void);   // 以上电为原点,或自定义的绝对编码值
  void NSQD_8192mang_inf(void); // 优化update_mang_inf，效果一样
  MOTOR_RM(const uint16_t id, class USER_CAN *CAN_rev) : ID(id), can_rev(CAN_rev) {}

  private:
  int Last_mang;

  int nsqd_8192xCnt_mang; // 圈速*编码值
};

/*************************************  瓴控6010电机  ************************************************/
class MOTOR_LK
{
  public:
  const uint16_t ID; // 电机反馈ID 321
  USER_CAN *can_rev;

  int16_t order; // 反馈的命令字节 161?
  int8_t temp;   // 温度
  int16_t iq;    // 转矩电流值
  int16_t sp;    // 速度
  uint16_t mang; // 角度
  int mang_inf;
  int motor_number;
  int nsqd_65535xCnt_mang;                          // 圈速*编码值
  uint32_t motor_send_error_cnt      = 0;           // 领空单电机模式发送错误计次
  HAL_StatusTypeDef motor_send_state = HAL_TIMEOUT; // 单电机控制发送状态/是否有调用单电机控制标志

  /*******************单电机控制*********************/
  HAL_StatusTypeDef LK_Close(uint16_t Id);                     // 电机关闭命令
  HAL_StatusTypeDef LK_Start(uint16_t Id);                     // 电机运行命令
  HAL_StatusTypeDef LK_Stop(uint16_t Id);                      // 电机停止命令
  HAL_StatusTypeDef LK_MIT_ClossControl(uint16_t Id);          // 转矩闭环控制命令 数值范围-2048~ 2048  对应MG电机实际转矩电流范围-33A~33A  电机在收到命令后回复主机
  HAL_StatusTypeDef LK_SP_ClossControl(uint16_t Id);           // 速度闭环控制命令
  HAL_StatusTypeDef LK_More_Mang_ClossControl_1(uint16_t Id);  // 多圈位置闭环控制命令 1
  HAL_StatusTypeDef LK_More_Mang_ClossControl_2(uint16_t Id);  // 多圈位置闭环控制命令 2
  HAL_StatusTypeDef LK_Alone_Mang_ClossControl_1(uint16_t Id); // 单圈位置闭环控制命令 1
  HAL_StatusTypeDef LK_Alone_Mang_ClossControl_2(uint16_t Id); // 单圈位置闭环控制命令 2
  HAL_StatusTypeDef LK_Read_PID(uint16_t Id);                  // 读取电机PID
  HAL_StatusTypeDef LK_Read_Acc(uint16_t Id);                  // 读取电机的加速度
  HAL_StatusTypeDef LK_Read_Encoder(uint16_t Id);              // 读取电机的编码器
  HAL_StatusTypeDef LK_Read_More_Mang(uint16_t Id);            // 读取电机多圈角度
  HAL_StatusTypeDef LK_Read_Alone_Mang(uint16_t Id);           // 读取电机单圈角度
  HAL_StatusTypeDef LK_Read_MotorState_1(uint16_t Id);         // 读取电机状态1，该命令读取当前电机的温度、电压和错误状态标志
  HAL_StatusTypeDef LK_Clear_MotorMismark(uint16_t Id);        // 该命令清除当前电机的错误状态，电机收到后返回
  HAL_StatusTypeDef LK_Read_MotorState_2(uint16_t Id);         // 读取电机状态2，该命令读取当前电机的温度、电压、转速、编码器位置。
  HAL_StatusTypeDef LK_Read_MotorState_3(uint16_t Id);         // 读取电机状态3，该命令读取当前电机的温度和相电流数据。

  HAL_StatusTypeDef LK_Broadcast_update(void);                                       // 接收数据时候用了什么函数就要在这里写
  void update_65535mang_inf_free(void);                                              // 以上电为原点,或自定义的绝对编码值
  void update_65535mang_inf_basic_zeromang(void);                                    // 以电机编码0点的绝对编码值
  MOTOR_LK(const uint16_t id, class USER_CAN *CAN_rev) : ID(id), can_rev(CAN_rev) {} // 不懂为什么要加这个
  private:
  uint8_t first = 0; // 接收到反馈数据的状态
  int Last_mang;
};

/*************************************  达妙4310电机  ************************************************/
// #define P_MIN -12.5f
// #define P_MAX 12.5f
// #define V_MIN -45.0f
// #define V_MAX 45.0f
// #define KP_MIN 0.0f
// #define KP_MAX 500.0f
// #define KD_MIN 0.0f
// #define KD_MAX 5.0f
// #define T_MIN -10.0f
// #define T_MAX 10.0f

class MOTOR_DM
{ // 达秒电机 ,在这里定义的东西需要使用this来提取
  public:
  const uint16_t ID; // 电机反馈ID
  USER_CAN *can_rev;

  int16_t id;  // 由达秒的串口助手设置
  int16_t ERR; // 反馈回来的电机错误信息，8：超压 9：欠压 A：过电流 B：mos过温 C：线圈过温 D：通讯丢失 E：过载
  int p_int;
  int v_int;
  int t_int;
  float mang;    // 位置 16位
  float sp;      //   速度  12位
  float Torque;  // 扭矩 12位
  float T_Rotor; // 表示电机内部线圈的平均温度 单位：摄氏度
  float T_MOS;   // 表示驱动上 MOS 的平均温度

  float nsqd_8PI_Cnt_mang; // 圈速*编码值
  float mang_inf;          // 过圈编码值
  uint8_t first = 0;       // 初始标志
  float Last_mang;         // 上次的角度值，判断过圈用
  int16_t motor_number;    // 圈速

  uint32_t motor_send_error_cnt      = 0;           // 达妙单电机发送错误计次
  HAL_StatusTypeDef motor_send_state = HAL_TIMEOUT; // 电机发送状态/是否有调用标志
  HAL_StatusTypeDef DM_Start(uint16_t id);
  HAL_StatusTypeDef DM_End(uint16_t id);
  HAL_StatusTypeDef DM_Savezero(uint16_t id);
  HAL_StatusTypeDef DM_MIT(uint16_t id, float _pos, float _vel, float _KP, float _KD, float _torq);
  HAL_StatusTypeDef DM_POS(uint16_t id, float _pos, float _vel);
  HAL_StatusTypeDef DM_VEL(uint16_t id, float _vel);
  void update_4PI_mang_inf_basic_zeromang(void); // 不改变0点的过圈检测
  HAL_StatusTypeDef DM_update(void);             // 得到速度，位置等参数
  MOTOR_DM(const uint16_t id, class USER_CAN *CAN_rev) : ID(id), can_rev(CAN_rev) {}

  private:
  float P_MIN = -12.5f, P_MAX = 12.5f, V_MIN = -45.0f, V_MAX = 45.0f, KP_MIN = 0.0f, KP_MAX = 500.0f, KD_MIN = 0.0f, KD_MAX = 5.0f, T_MIN = -10.0f, T_MAX = 10.0f;
  // 这里定义的东西是用给class类里面的函数参数定义
};
/****************************************Cyber_Gear**********************************************/

float uint_to_float(int value, float x_min, float x_max, int bits);
int float_to_uint(float x, float x_min, float x_max, int bits);
// uint32_t Cyber_Gear_EXTID_SET(uint8_t mode, uint8_t Motor_id, uint16_t data);
// uint8_t Motor_Id_Get(uint32_t EXTID); // 针对通讯协议2,从ID里面读取电机ID
#ifndef PI
// #define PI 3.1415926f
#endif

// 01电机无绝对编码，过圈阈值0.81
class Cyber_Gear // 小米，灵足时代电机
{
  public:
  /*可读写单个参数列 (7019-701C 为最新版本固件可)*/
  struct
  {
    float voltage; // 电压
    float speed;   // 速度
    float temp;    // 温度

    float VBUS;    // 母线电压(只读)
    float mechVel; // 负载端转(只读)
    float iqf;     // iq滤波(只读)
    float mechPos; // 负载 计圈机械角度(只读)

    float limit_cur;     // 速度模式电流限制
    float limit_spd;     // 位置模式速度限制
    float loc_ref;       // 位置模式角度指令
    float cur_filt_gain; // 电流的滤波系
    float cur_ki;        // 电流的Ki
    float cur_kp;        // 电流的Kp
    float imit_torque;   // 转矩限制
    float spd_ref;       // 转模式转速指
    float iq_ref;        // 电流Iq指令(可读)
  } index;
  struct
  {
    uint8_t A_phase_sampling_overcurrent; // A相采样电
    uint8_t B_phase_sampling_overcurrent; // B相采样电
    uint8_t C_phase_sampling_overcurrent; // C相采样电
    uint8_t Mcu_Error;                    // 驱动芯片故障
    uint8_t Encoder_not_calibrated;       // 编码器未标定
    uint8_t OverLoad_Voltage;             // 过压故障
    uint8_t UnderLoad_Voltage;            // 欠压故障
    uint8_t Over_temperature_fault;       // 过温故障
    uint16_t Fault;
    uint16_t Overload_fault; // 过载故障
  } Error;
  struct
  {
    uint8_t Over_temperature_warning_80_degrees; // 80度过温警
    uint8_t Over_temperature_warning_75_degrees; // 75度过温警
    uint32_t WarningValue;
  } Temp_Warning;
  USER_CAN *can_rev;
  const uint16_t MOTOR_ID;            // 电机反馈ID
  const uint16_t MY_Master_ID = 0xFE; // 主机ID
  uint8_t RunMode;                    // 运行模式
  float sp;                           // 速度
  float mang;                         // 角度
  float torque;                       // 力矩
  float temp;                         // 温度

  float nsqd_8PI_Cnt_mang; // 圈速*编码值
  float mang_inf;          // 过圈编码值
  uint8_t first = 0;       // 初始标志
  float Last_mang;         // 上次的角度值，判断过圈用
  int16_t motor_number;    // 圈速

  uint32_t motor_send_error_cnt      = 0;           // 达妙单电机发送错误计次
  HAL_StatusTypeDef motor_send_state = HAL_TIMEOUT; // 电机发送状态/是否有调用标志
  uint8_t Motor_Id_Get(uint32_t EXTID);             // 针对通讯协议2,从ID里面读取电机ID
  HAL_StatusTypeDef torque_Send(uint8_t motor_id, float torque);
  HAL_StatusTypeDef Stop(uint8_t motor_id);
  HAL_StatusTypeDef Enable(uint8_t motor_id);
  uint32_t EXTID_SET(uint8_t mode, uint8_t Motor_id, uint16_t data);

  void update_4PI_mang_inf_basic_zeromang(void); // 不改变0点的过圈检测
  HAL_StatusTypeDef update(void);
  Cyber_Gear(const uint16_t id, class USER_CAN *CAN_rev) : MOTOR_ID(id), can_rev(CAN_rev) {}

  private:
  float P_MIN = -12.5f, P_MAX = 12.5f, V_MIN = -30.0f, V_MAX = 30.0f, KP_MIN = 0.0f, KP_MAX = 500.0f, KD_MIN = 0.0f, KD_MAX = 5.0f, T_MIN = -12.0f,
        T_MAX = 12.0f; // 小米电机参数10
};

/*************************************  RMD结构体  ************************************************/

// 这里定义的东西是用给class类里面的函数参数定义
typedef struct
{

  uint16_t anglePidKp;
  uint16_t anglePidKi;
  uint16_t speedPidKp;
  uint16_t speedPidKi;
  uint16_t iqPidKp;
  uint16_t iqPidKi;
  int32_t Accel;
  uint16_t encoder;       // 编码器位置     0~65535
  uint16_t encoderRaw;    // 编码器原始位置 0~65535
  uint16_t encoderOffset; // 编码器零偏     0~65535
  int64_t motorAngle;
  uint16_t circleAngle;
  int8_t temperature;
  uint16_t voltage;
  uint8_t eerorState;
  int16_t iq;
  int16_t speed;
  uint16_t now_encoder; // 编码器位置值
  int16_t iA;
  int16_t iB;
  int16_t iC;
  int16_t iqControl;
  int32_t speedControl;
  int32_t angleControl;

} RMD_typedef;

/*************************************  RMD电机  ************************************************/

class MOTOR_RMD
{
  public:
  const uint16_t ID; // 电机反馈ID
  USER_CAN *can_rev;
  RMD_typedef RMD_X;
  uint32_t motor_send_error_cnt      = 0;           // 达妙单电机发送错误计次
  HAL_StatusTypeDef motor_send_state = HAL_TIMEOUT; // 电机发送状态/是否有调用标志

  HAL_StatusTypeDef RMD_Read_and_Write_Things(uint16_t Id, uint16_t Order_Id);
  HAL_StatusTypeDef RMD_Write_PID(uint16_t Id, uint16_t Order_Id, uint16_t anglePidKp, uint16_t anglePidKi, uint16_t speedPidKp, uint16_t speedPidKi, uint16_t iqPidKp, uint16_t iqPidKi);
  HAL_StatusTypeDef RMD_Write_ACCLE_to_RAM(uint16_t Id, int32_t Accel);
  HAL_StatusTypeDef RMD_Write_EncoderOffset_to_ROM(uint16_t Id, uint16_t EncoderOffset); // 写入编码器值
  HAL_StatusTypeDef RMD_Iqcontrol_Motor(uint16_t Id, int16_t iqControl);
  // HAL_StatusTypeDef RMD_Speedcontrol_Motor(uint16_t Id, int32_t angleControl);//好像没有这个函数
  HAL_StatusTypeDef RMD_Speedcontrol_Motor(uint16_t Id, uint16_t Order_Id, uint16_t maxSpeed, int32_t angleControl);
  HAL_StatusTypeDef RMD_Anglecontrol_Motor(uint16_t Id, uint16_t Order_Id, uint8_t spinDirection, uint16_t maxSpeed, uint16_t angleControl);
  HAL_StatusTypeDef RMD_update(void);

  MOTOR_RMD(const uint16_t id, class USER_CAN *CAN_rev) : ID(id), can_rev(CAN_rev) {}

  private:
};

typedef struct
{
  float qy;
  float hy;
  float qz;
  float hz;
} ML_typedef;

class MOTOR_DiPan
{
  public:
  ML_typedef ML;

  MOTOR_DiPan(void);
  void ML_Data_Deal(float lx, float ly, float lp, int MAX_rate);
};

/*************************************************** Duo Lun  ******************************************************/

// #define M_PI 3.1415926535897932384626433832795f
#define COS_45    0.70710678118654752440084436210485f
#define RAD2MANG  1303.7972938088065906186957895476f
#define SPEED_MAX 3000

struct wheel_dir_and_weight {
  bool dir;
  float speed;
  int yaogan_speed;
};
struct lun_xy {
  float x, y;
};

enum {
  QZ = 0,
  HZ,
  HY,
  QY
};

class RUDDER_DiPan
{
  public:
  int16_t g_angle_6020[4]              = {0, 0, 0, 0};
  wheel_dir_and_weight g_wheel_3508[4] = {0, 0, 0, 0};
  lun_xy QZ_xy, HZ_xy, HY_xy, QY_xy;
  uint16_t ZERO[4] = {1685, 7564, 6822, 5802}; // qz,hz,hy,qy  每个舵轮都不一样，自己修改

  void Not_Xiaotuoluo_Jie_Suan(float CH0, float CH1, int16_t CH2);
  void Xiaotuoluo_jie_Suan(uint16_t Mang_yaw, int16_t CH0, int16_t CH1, int16_t CH2);

  private:
  float Get_Ch0_Ch1_Vector_Speed(int16_t CH0, int16_t CH1);
  float Get_Max_Speed(int16_t CH0, int16_t CH1, int16_t CH2);
  float absf(float d0);
  float Get_Max_float(float d0, float d1);
  int16_t Get_Max_int16(int16_t d0, int16_t d1);
};
/**************************************** P I D **********************************************************/

int16_t float_to_int16(float a, float a_max, float a_min, int16_t b_max, int16_t b_min);
float int16_to_float(int16_t a, int16_t a_max, int16_t a_min, float b_max, float b_min);
#define LIMIT(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
// #define NL -3
// #define NM -2
// #define NS -1
// #define ZE 0
// #define PS 1
// #define PM 2
// #define PL 3
class PID_class
{
  public:
  float error;
  float KP, KI, KD;
  float LIMIT_P, LIMIT_I, LIMIT_D, LIMIT_PID, Integral;
  float Deadzoom, Separate, Gama;
  float OUT_P, OUT_I, OUT_D, OUT_PID, delta_OUT_PID;
  float LP_out_last       = 0; // 上一次滤波值
  float g_speed_3508[4]   = {0, 0, 0, 0};
  int16_t g_angle_6020[4] = {0, 0, 0, 0};

  bool Get_6020mang_need_turn_direction_is(uint16_t goal, uint16_t now);
  void PID_update_for_6020mang(int16_t goal, uint16_t now, struct wheel_dir_and_weight *wheel_dir_and_weight);
  void PID_new_update(float goal, float now);
  void PID_update(float goal, float now);
  void PID_update_LP(float goal, float now, float k_value);
  float low_pass_filter(float value, float k_value);
  void PID_Inc_update(float goal, float now);
  PID_class(float kp, float ki, float kd, float limit_p, float limit_i, float limit_d, float limit_pid, float dz, float separate)
      : KP(kp), KI(ki), KD(kd), LIMIT_P(limit_p), LIMIT_I(limit_i), LIMIT_D(limit_d), LIMIT_PID(limit_pid), Deadzoom(dz), Separate(separate)
  {
  }

  private:
  float alpha = 0.5;
  float LAST_Error;
  float last_error;
  float previous_error;
};

class PID_Fuzzy_class
{
  public:
  float LastError;        // 前次误差
  float error;            // 当前误差
  float SumError;         // 积分误差
  float IMax;             // 积分限制
  float POut, IOut, DOut; // 比例输出
  float DOut_last;        // 上一次微分输出
  float OutMax;           // 限幅
  float Out;              // 总输出
  float Out_last;         // 上一次输出

  float I_U; // 变速积分上限
  float I_L; // 变速积分下限

  float Kp0, Ki0, Kd0; // PID初值
  float dKp, dKi, dKd; // PID变化量

  float stair, Kp_stair, Ki_stair, Kd_stair; // 动态调整梯度   //0.25f

  void FuzzyPID_update(float goal, float now);
  PID_Fuzzy_class(float kp, float ki, float kd, float limit_i, float limit_pid, float IL, float Stair, float KP_stair, float KI_stair, float KD_stair)
      : Kp0(kp), Ki0(ki), Kd0(kd), IMax(limit_i), OutMax(limit_pid), I_L(IL), stair(Stair), Kp_stair(KP_stair), Ki_stair(KI_stair), Kd_stair(KD_stair)
  {
  }

  private:
  float NL = -3, NM = -2, NS = -1, ZE = 0, PS = 1, PM = 2, PL = 3; // 负大 负中 负小 零 正小 正中 正大
  const float fuzzyRuleKp[7][7] = {
      /******Kp隶属度规则表******/
      /*

      kp |   de/dt （e的速率）隶属度
      ---|-------------------------------
      e  | PL,PL,PM,	PM,	PS,	ZE,	ZE,
      隶 | PL,PL,	PM,	PS,	PS,	ZE,	NS,
      属 | PM,PM,	PM,	PS,	ZE,	NS,	NS,
      度 | PM,PM,	PS,	ZE,	NS,	NM,	NM,
         | PS,PS,	ZE,	NS,	NS,	NM,	NM,
         | PS,ZE,	NS,	NM,	NM,	NM,	NL,
         | ZE,ZE,	NM,	NM,	NM,	NL,	NL

      */
      PL, PL, PM, PM, PS, ZE, ZE, // NL(负大)NM(负中)NS(负小)ZE(零)PS(正小)PM(正中)PL(正大)
      PL, PL, PM, PS, PS, ZE, NS, // 中间是0，靠近0变小
      PM, PM, PM, PS, ZE, NS, NS, PM, PM, PS, ZE, NS, NM, NM, PS, PS, ZE, NS, NS, NM, NM, PS, ZE, NS, NM, NM, NM, NL, ZE, ZE, NM, NM, NM, NL, NL};

  const float fuzzyRuleKi[7][7] = {NL, NL, NM, NM, NS, ZE, ZE, // 中间是0，靠近0变大
                                   NL, NL, NM, NS, NS, ZE, ZE, NL, NM, NS, NS, ZE, PS, PS, NM, NM, NS, ZE, PS, PM, PM,
                                   NS, NS, ZE, PS, PS, PM, PL, ZE, ZE, PS, PS, PM, PL, PL, ZE, ZE, PS, PM, PM, PL, PL};

  const float fuzzyRuleKd[7][7] = {PS, NS, NL, NL, NL, NM, PS, PS, NS, NL, NM, NM, NS, ZE, ZE, NS, NM, NM, NS, NS, ZE, ZE, NS, NS, NS,
                                   NS, NS, ZE, ZE, ZE, ZE, ZE, ZE, ZE, ZE, PL, NS, PS, PS, PS, PS, PL, PL, PM, PM, PM, PS, PS, PL};

  void fuzzy(float goal, float now);
};

class CRC16_class
{
  private:
  uint16_t crc16_init           = 0xffff;
  const uint16_t crc16_tab[256] = {
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

  public:
  uint16_t get_crc16_check_sum(uint8_t *p_msg, uint16_t len, uint16_t crc16) // 得到crc校验的值
  {
    uint8_t data;
    if (p_msg == NULL)
      return 0xffff;
    while (len--) {
      data    = *p_msg++;
      (crc16) = ((uint16_t)(crc16) >> 8) ^ crc16_tab[((uint16_t)(crc16) ^ (uint16_t)(data)) & 0x00ff];
    }
    return crc16;
  }

  bool verify_crc16_check_sum(uint8_t *p_msg, uint16_t len) // 校验
  {
    uint16_t w_expected = 0;

    if ((p_msg == NULL) || (len <= 2)) {
      return false;
    }
    w_expected = get_crc16_check_sum(p_msg, len - 2, crc16_init);

    return ((w_expected & 0xff) == p_msg[len - 2] && ((w_expected >> 8) & 0xff) == p_msg[len - 1]);
  }

  CRC16_class();
};

/****************************************** D B U S **********************************************************/
#define YK_SW_UP          ((uint16_t)1)
#define YK_SW_MID         ((uint16_t)3)
#define YK_SW_DOWN        ((uint16_t)2)

#define KEY_PRESSED_W     ((uint16_t)0x01 << 0)
#define KEY_PRESSED_S     ((uint16_t)0x01 << 1)
#define KEY_PRESSED_A     ((uint16_t)0x01 << 2)
#define KEY_PRESSED_D     ((uint16_t)0x01 << 3)
#define KEY_PRESSED_SHIFT ((uint16_t)0x01 << 4)
#define KEY_PRESSED_CTRL  ((uint16_t)0x01 << 5)
#define KEY_PRESSED_Q     ((uint16_t)0x01 << 6)
#define KEY_PRESSED_E     ((uint16_t)0x01 << 7)
#define KEY_PRESSED_R     ((uint16_t)0x01 << 8)
#define KEY_PRESSED_F     ((uint16_t)0x01 << 9)
#define KEY_PRESSED_G     ((uint16_t)0x01 << 10)
#define KEY_PRESSED_Z     ((uint16_t)0x01 << 11)
#define KEY_PRESSED_X     ((uint16_t)0x01 << 12)
#define KEY_PRESSED_C     ((uint16_t)0x01 << 13)
#define KEY_PRESSED_V     ((uint16_t)0x01 << 14)
#define KEY_PRESSED_B     ((uint16_t)0x01 << 15)

typedef struct
{
  uint8_t s1; // 左开关
  uint8_t s2;
  int16_t ch0; // 横滚
  int16_t ch1; // 俯仰
  int16_t ch2; // 偏航
  int16_t ch3; // 油门
  int16_t v;   // 波轮 以上范围都为-660 600 11bit
} yaogan_typedef;
typedef struct
{
  uint8_t press_l;
  uint8_t press_r;
  int16_t x; // 左右
  int16_t y; // 前后
  int16_t z;

} shubiao_typedef;

class DBUS
{
  public:
  bool dog = false;
  UART_HandleTypeDef *huart;
  uint8_t dbus_rx_buffer[25];

  yaogan_typedef yaogan;
  shubiao_typedef shubiao;
  uint16_t jianpan;
  uint32_t rx_error_cnt;

  void feed_watchdog(void) { time_100ms = 0; }
  // HAL_StatusTypeDef receive_run(void)
  // {
  //     return HAL_UART_Receive_IT(this->huart, this->dbus_rx_buffer, 18);
  // }
  // HAL_StatusTypeDef receive_refresh(void)
  // {
  //     return HAL_UART_AbortReceive_IT(this->huart);
  // }

  void Init(void);
  void DBUS_RxCplt_IRQHandler(void);
  void jianpan_deal(void);
  void set_zero(void);
  void data_deal(void);
  void watchdog_run(void);
  void can_receive_data_deal(uint8_t num, uint8_t *buf);
  //  void YK_ctrl(void);
  uint8_t Pressed_Check(uint16_t key_value); // 按下状态 返回1

  DBUS(UART_HandleTypeDef *p) : huart(p) {}
  //  void YK_ctrl(void)
  //  {
  //    if (!dt16_signal_flag && !vt13yk_signal_flag)
  //    {
  //      YK_set_zero();
  //    }
  //    else if (dt16_signal_flag && !vt13yk_signal_flag)
  //    {
  //      dt16_ctrl();
  //    }
  //    else if (!dt16_signal_flag && vt13yk_signal_flag)
  //    {
  //      vt13_ctrl();
  //    }
  //    else if (dt16_signal_flag && vt13yk_signal_flag)
  //    {
  //      if (this->VT13_Data.mode_sw == 0)
  //      {
  //        dt16_ctrl();
  //      }
  //      else if (this->VT13_Data.mode_sw == 1)
  //      {
  //        vt13_ctrl();
  //      }
  //      else if (this->VT13_Data.mode_sw == 2)
  //      {
  //        vt13_ctrl();
  //      }
  //    }
  //  }

  private:
  uint8_t first;
  uint16_t time_100ms;
  uint8_t index;           // 无用
  uint16_t delaycount[16]; // 无用
  uint16_t last_jianpan;   // 无用

  HAL_StatusTypeDef check_and_deal(void);
};

/****************************************** RC **********************************************************/
#define YK_MODE_SW_C ((uint16_t)0)
#define YK_MODE_SW_N ((uint16_t)1)
#define YK_MODE_SW_S ((uint16_t)2)

typedef struct
{
  uint8_t sof_1;   // 0xa9
  uint8_t sof_2;   // 0x53
  int16_t ch_0;    //+-660 横滚
  int16_t ch_1;    //+-660 俯仰
  int16_t ch_2;    //+-660 油门
  int16_t ch_3;    //+-660 偏航
  uint8_t mode_sw; // cns:012
  uint8_t pause;   // 暂停按键
  uint8_t fn_1;    // 左自定义按键
  uint8_t fn_2;    // 右自定义按键
  int16_t wheel;   // 拨轮
  uint8_t trigger; // 扳机键

  int16_t mouse_x;      //+-32768 右正
  int16_t mouse_y;      //+-32768 前正
  int16_t mouse_z;      //+-32768 鼠标滚轮滚动速度
  uint8_t mouse_left;   // 01
  uint8_t mouse_right;  // 01
  uint8_t mouse_middle; // 01鼠标中键
  uint16_t key;         // 键盘
  uint16_t crc16;
} remote_data_t;

static const uint16_t crc16_tab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

typedef union {
  float f;
  unsigned char c[4];
  int i;
} dat;
// 自定义控制器发送端，将数据赋值到此处,定时调用self_controler_data_tx即可
// 机器人接收端将接收到的数据会存入此处调用
typedef __PACKED_STRUCT
{
  // acc[3],gyro[3],24+3*2
  dat j0;
  dat j1;
  dat j2;
  dat j3;
  dat j4;
  dat j5;
  dat j6;
  uint8_t y, z;
  // bool KEY_1;
  // bool KEY_2;
}
custom_controller_data_t;
/****************************************tu  chuan***************************************************/
// 0xA5,0xC0,seq(0-255向上计数),crc8
// cmdid_L 4,cmdid_H 3
// 12data
#define FRAME_HEADER_LENGTH            5                                                                       // frame帧头                                                                // 帧头数据长度
#define CMD_ID_LENGTH                  2                                                                       // 命令id                                                                // 命令码ID数据长度
#define DATA_LENGTH                    30                                                                      // 数据位                                                               // 数据段长度
#define FRAME_TAIL_LENGTH              2                                                                       // 帧尾crc16整包校验                                                                // 帧尾数据长度
#define DATA_FRAME_LENGTH              (FRAME_HEADER_LENGTH + CMD_ID_LENGTH + DATA_LENGTH + FRAME_TAIL_LENGTH) // 整个数据帧的长度
#define CONTROLLER_CMD_ID              0x0302                                                                  // 自定义控制器                                                           // 自定义控制器命令码
#define KEY_MOUSE_CMD_ID               0X304                                                                   // 键鼠发送
#define PASS_BACK_CMD_ID               0X309                                                                   // 机器人发自定义控制器
#define CUSTOM_ANALOG_KEY_MOUSE_CMD_ID 0X306                                                                   // 自定义控制器模拟键鼠

typedef __PACKED_STRUCT
{
  __PACKED_STRUCT
  {
    uint8_t sof;          // 起始字节，固定值为0xA5
    uint16_t data_length; // 数据帧中data的长度
    uint8_t seq;          // 包序号
    uint8_t crc8;         // 帧头CRC8校验
  }
  frame_header;                 // 帧头
  __packed uint16_t cmd_id;     // 命令码
  __packed uint8_t data[300];   // 数据帧（扩展到300字节以支持视频数据）
  __packed uint16_t frame_tail; // 帧尾CRC16校验
}
Controller_t; // 图传链路帧格式结构体,0x302,0x304,0x309

typedef __PACKED_STRUCT
{
  dat j1_angle_iq;
  dat j2_angle_iq;
  dat j3_angle_iq;
  dat end_pitch_angle;
  dat end_roll_angle;
  dat end_yaw_angle;
}
force_feedback_t;

// 机器人发送端，将数据赋值到此处,定时调用robo_data_tx即可
// 自定义控制器接收端将接收到的数据会存入此处调用，此处是用作力反馈
typedef __PACKED_STRUCT
{
  dat j0_f;
  dat j1_f;
  dat j2_f;
  dat j3_f;
  dat j4_f;
  dat j5_f;
  uint8_t tc_flag;
}
custom_robo_data_t;

// 自定义控制器发机器人302 30 30hz_max
// 机器人发自定义控制器309 30 10hz
// 图传链路键鼠30hz
class RC
{
  public:
  UART_HandleTypeDef *DT16_huart;
  UART_HandleTypeDef *TC_huart;
  uint8_t DR16_rx_buffer[50] = {0};

  /*抽象遥控*/
  yaogan_typedef yaogan   = {0}; // 协议与dt7为准
  shubiao_typedef shubiao = {0};
  uint16_t jianpan        = 0;

  /*DBUS */
  yaogan_typedef DT16_yaogan   = {0};
  shubiao_typedef DT16_shubiao = {0};
  uint16_t DT16_jianpan        = 0;
  uint8_t dt16_signal_flag     = 0;
  /*TC*/
  custom_controller_data_t Custom_Controller_data = {0}, poly_Custom_Controller_data = {0}; // 自定义控制器数据
  remote_data_t VT13_Data    = {0};                                                         // VT13遥控器数据
  uint16_t crc16_init        = 0xffff;
  uint16_t DT16_time_100ms   = 0;
  uint16_t VT13_time_100ms   = 0;
  uint8_t VT13_rx_buffer[70] = {0};
  uint8_t vt13yk_signal_flag = 0;
  bool self_ctrl_enable_flag = 0;

  /*调试*/
  uint32_t rx_cnt = 0, vt_yk_cnt = 0, err_cnt = 0, selfctrl_cnt = 0, self_err_cnt = 0;

  void memset_VT13_rx_buffer(void) { memset(&VT13_rx_buffer, 0, sizeof(VT13_rx_buffer)); }
  void memset_dr16_rx_buffer(void) { memset(&DR16_rx_buffer, 0, sizeof(DR16_rx_buffer)); }
  void DT16_feed_watchdog(void) { DT16_time_100ms = 0; }
  void VT13_feed_watchdog(void) { VT13_time_100ms = 0; }

  void DT16_Init(void);
  void VT13_Init(void);
  void DT16_RxCplt_IRQHandler(void);
  uint8_t VT13_RxCplt_IRQHandler(void);
  void VT13_YK_deal(void);
  void VT13_self_ctrl_deal(void);
  void DT16_set_zero(void);
  void VT13_YK_set_zero(void);
  void DT16_data_deal(void);
  void VT13_data_deal(void);
  void DT16_watchdog_run(void);
  void VT13_watchdog_run(void);
  void fill_data(void);
  void VT13_UART_Receive_enable(void);
  void can_receive_data_deal(uint8_t *buf);  // 没用
  uint8_t Pressed_Check(uint16_t key_value); // 按下状态 返回1
  void YK_set_zero(void)
  {
    yaogan.ch0      = 0;
    yaogan.ch1      = 0;
    yaogan.ch2      = 0;
    yaogan.ch3      = 0;
    yaogan.v        = 0;
    yaogan.s1       = YK_SW_UP;
    yaogan.s2       = YK_SW_UP;
    shubiao.x       = 0;
    shubiao.y       = 0;
    shubiao.z       = 0;
    shubiao.press_l = 0;
    shubiao.press_r = 0;
    jianpan         = 0;
  }
  void vt13_ctrl(void)
  {
    this->yaogan.ch0 = this->VT13_Data.ch_0;
    this->yaogan.ch1 = this->VT13_Data.ch_1;
    this->yaogan.ch2 = this->VT13_Data.ch_3;
    this->yaogan.ch3 = this->VT13_Data.ch_2;
    this->jianpan    = this->VT13_Data.key;
    if (this->VT13_Data.mode_sw == 1) {
      this->yaogan.s1 = YK_SW_DOWN;
      this->yaogan.s2 = YK_SW_DOWN; // this->VT13_Data.mouse_x
    } else {
      this->yaogan.s1 = YK_SW_UP;
      this->yaogan.s2 = YK_SW_UP; // this->VT13_Data.mouse_x
    }
    this->shubiao.x       = this->VT13_Data.mouse_x;
    this->shubiao.y       = this->VT13_Data.mouse_y;
    this->shubiao.z       = this->VT13_Data.mouse_z;
    this->shubiao.press_l = this->VT13_Data.mouse_left;
    this->shubiao.press_r = this->VT13_Data.mouse_right;
    this->yaogan.v        = this->VT13_Data.wheel;
  }
  void dt16_ctrl(void)
  {
    this->yaogan.ch0      = this->DT16_yaogan.ch0;
    this->yaogan.ch1      = this->DT16_yaogan.ch1;
    this->yaogan.ch2      = this->DT16_yaogan.ch2;
    this->yaogan.ch3      = this->DT16_yaogan.ch3;
    this->yaogan.s1       = DT16_yaogan.s1;
    this->yaogan.s2       = DT16_yaogan.s2;
    this->shubiao.x       = this->DT16_shubiao.x;
    this->shubiao.y       = this->DT16_shubiao.y;
    this->shubiao.z       = this->DT16_shubiao.z;
    this->shubiao.press_l = this->DT16_shubiao.press_l;
    this->shubiao.press_r = this->DT16_shubiao.press_r;
    this->jianpan         = this->DT16_jianpan;
    this->yaogan.v        = this->DT16_yaogan.v;
  }
  void YK_ctrl(void)
  {
    if (!dt16_signal_flag && !vt13yk_signal_flag) {
      YK_set_zero();
    } else if (dt16_signal_flag && !vt13yk_signal_flag) {
      dt16_ctrl();
    } else if (!dt16_signal_flag && vt13yk_signal_flag) {
      vt13_ctrl();
    } else if (dt16_signal_flag && vt13yk_signal_flag) {
      if (this->VT13_Data.mode_sw == 0) {
        dt16_ctrl();
      } else if (this->VT13_Data.mode_sw == 1) {
        vt13_ctrl();
      } else if (this->VT13_Data.mode_sw == 2) {
        vt13_ctrl();
      }
    }
  }
  // void dp_deal(void)
  // {
  //     uint8_t buffer[8] = {0};
  //     //(int16_t)((this->can_rev->rx_buf[0] << 8) | this->can_rev->rx_buf[1])
  //     // ch1234
  //     yaogan.ch0 = (int16_t)((buffer[0] << 8) | buffer[1]);
  //     yaogan.ch1 = (int16_t)((buffer[2] << 8) | buffer[3]);
  //     yaogan.ch2 = (int16_t)((buffer[4] << 8) | buffer[5]);
  //     yaogan.ch3 = (int16_t)((buffer[6] << 8) | buffer[7]);
  //     // shubiao x,y,jianpan,s1,s2
  //     shubiao.x = (int16_t)((buffer[0] << 8) | buffer[1]);
  //     shubiao.y = (int16_t)((buffer[2] << 8) | buffer[3]);
  //     jianpan = (uint16_t)((buffer[4] << 8) | buffer[5]);
  //     shubiao.press_r = buffer[6] & 0x01;
  //     shubiao.press_l = buffer[6] & 0x02;
  //     yaogan.s2 = buffer[6] & 0x0C;
  //     yaogan.s1 = buffer[6] & 0x30;
  //     // buffer[6] = (uint8_t)(((yaogan.s1 << 4) | (yaogan.s2 << 2) | (shubiao.press_l << 1) | (shubiao.press_r)) & 0x3f);
  // }

  // extKalman_t KF_Mouse_Y_Speed, KF_Mouse_X_Speed;   // 没用
  // float SF(float t, float *slopeFilter, float res); // 没用
  // float Mouse_X_Speed(float Xmax);                                            // 没用
  // float Mouse_Y_Speed(float Ymax);                                            // 没用
  bool verify_crc16_check_sum(uint8_t *p_msg, uint16_t len);                  // 没用
  uint16_t get_crc16_check_sum(uint8_t *p_msg, uint16_t len, uint16_t crc16); // 没用

  RC(UART_HandleTypeDef *f, UART_HandleTypeDef *s) : DT16_huart(f), TC_huart(s) {}

  private:
  uint8_t first;
  uint8_t index;
  uint16_t delaycount[16];
  uint16_t last_jianpan;

  HAL_StatusTypeDef DT16_check_and_deal(void);
  HAL_StatusTypeDef VT13_check_and_deal(void);
};

/**************************************** TuChuan 图传链路类 **********************************************************/
class TuChuan
{
  public:
  UART_HandleTypeDef *huart;
  DMA_HandleTypeDef *hdma_usart_rx;
  uint8_t tuchuan_rx_buffer[512];       // 接收缓冲区（扩展到512字节）
  uint8_t tuchuan_tx_data[DATA_LENGTH]; //
  uint8_t seq = 0;
  uint8_t video_buffer[300]; // 视频数据缓冲区（300字节）

  uint16_t jianpan;
  shubiao_typedef shubiao;
  uint32_t self_ctrl_txcnt = 0, rx_cnt = 0, rx_302id_cnt = 0, rx_304id_cnt = 0, rx_309id_cnt = 0, err_cnt = 0;
  uint32_t crc_yes = 0;
  uint16_t robo_time_40ms;
  uint8_t robo_data_run_flag = 0;

  // 发送状态统计
  uint32_t tx_ok_cnt            = 0;      // 发送成功计数
  uint32_t tx_busy_cnt          = 0;      // 发送BUSY计数
  uint32_t tx_timeout_cnt       = 0;      // 发送超时计数
  uint32_t tx_error_cnt         = 0;      // 发送其他错误计数
  HAL_StatusTypeDef last_status = HAL_OK; // 最后一次发送状态

  custom_controller_data_t Custom_Controller_data, poly_Custom_Controller_data; // 控制器发给机器人数据
  custom_robo_data_t Custom_Robo_data, poly_Custom_Robo_data;                   // 机器人发给控制器数据
  remote_data_t VT13_Data = {};                                                 // VT13遥控器数据
  Controller_t tx_data;                                                         // 图传链路发送数据帧

  TuChuan(UART_HandleTypeDef *p, DMA_HandleTypeDef *dma_usart_RX = NULL) : huart(p), hdma_usart_rx(dma_usart_RX) {}

  /**
   * @brief 数据拼接函数，将帧头、命令码、数据段、帧尾头拼接成一个数组
   * @param data 数据段的数组指针
   * @param data_lenth 数据段长度
   */
  void Data_Concatenation(uint8_t *data, uint8_t *data_out, uint16_t data_lenth, uint16_t cmd_id)
  {
    /// 帧头数据
    data_out[0] = 0xA5; // 数据帧起始字节，固定值为 0xA5
    // tx_data.frame_header.data_length = data_lenth;                // 数据帧中数据段的长度
    memcpy(data_out + 1, (uint8_t *)&data_lenth, sizeof(data_lenth));
    data_out[1 + sizeof(data_lenth)] = seq++; // 包序号
    // tx_data.frame_header.seq         = seq++;                                     // 包序号
    Append_CRC8_Check_Sum((uint8_t *)(data_out), 1 + sizeof(data_lenth) + 1 + 1); // 添加帧头 CRC8 校验位
    /// 命令码ID
    // data_out[1 + sizeof(data_lenth) + 1 + 1] = cmd_id;
    memcpy(data_out + 1 + sizeof(data_lenth) + 1 + 1, (uint8_t *)&cmd_id, sizeof(cmd_id));

    /// 数据段
    memcpy(data_out + 1 + sizeof(data_lenth) + 1 + 1 + sizeof(cmd_id), data, data_lenth);
    /// 帧尾CRC16，整包校验
    Append_CRC16_Check_Sum((uint8_t *)data_out, FRAME_HEADER_LENGTH + CMD_ID_LENGTH + data_lenth + FRAME_TAIL_LENGTH);
  }

  void set_zero(void)
  {
    memset(&Custom_Controller_data, 0, sizeof(custom_controller_data_t));
    memset(&tuchuan_rx_buffer, 0, sizeof(tuchuan_rx_buffer));
    err_cnt++;
  }

  void set_robo_zero(void)
  {
    memset(&Custom_Robo_data, 0, sizeof(Custom_Robo_data));
    memset(&tuchuan_rx_buffer, 0, sizeof(tuchuan_rx_buffer));
    err_cnt++;
  }

  void robo_watchdog_run(void)
  {
    this->robo_time_40ms++;
    if (this->robo_time_40ms > 20) {
      this->robo_time_40ms     = 0;
      this->robo_data_run_flag = 0;
      set_robo_zero();
    }
    this->robo_data_run_flag = 1;
  }

  void robo_feed_watchdog(void)
  {
    this->robo_time_40ms = 0;
  }

  void Init(void)
  {
    this->set_zero();
    err_cnt = 0;
    UART_RX_Enable();
  }

  void UART_RX_Enable(void)
  {
    __HAL_UART_ENABLE_IT(this->huart, UART_IT_IDLE);
    HAL_UART_Receive_DMA(this->huart, tuchuan_rx_buffer, 70);
  }

  void Init_IDLE(void)
  {
    this->set_zero();
    __HAL_DMA_DISABLE_IT(hdma_usart_rx, DMA_IT_HT);
    HAL_UARTEx_ReceiveToIdle_DMA(this->huart, tuchuan_rx_buffer, 70);
  }

  void self_controler_data_tx(void) // id0x302 自定义控制器数据发送
  {
    self_ctrl_txcnt++;
    Data_Concatenation((uint8_t *)&Custom_Controller_data, (uint8_t *)(&tx_data), DATA_LENGTH, 0x0310);
    HAL_UART_Transmit(huart, (uint8_t *)(&tx_data), sizeof(tx_data), 100);
    // HAL_UART_Transmit_DMA(huart, (uint8_t *)(&tx_data), sizeof(tx_data));
    // HAL_UART_Transmit_DMA(huart, (uint8_t *)(&tx_data), sizeof(tx_data));
    // // 切换到 DMA 发送测试
    // HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(huart, (uint8_t *)(&tx_data), sizeof(tx_data));
    // last_status = status;

    // // 统计发送状态
    // if (status == HAL_OK)
    // {
    //     tx_ok_cnt++;
    // }
    // else if (status == HAL_BUSY)
    // {
    //     tx_busy_cnt++;
    // }
    // else if (status == HAL_TIMEOUT)
    // {
    //     tx_timeout_cnt++;
    // }
    // else
    // {
    //     tx_error_cnt++;
    // }
  }

  void robo_data_tx(void) // id0x0309 机器人发自定义控制器数据发送
  {
    Data_Concatenation((uint8_t *)&Custom_Robo_data, (uint8_t *)(&tx_data), DATA_LENGTH, PASS_BACK_CMD_ID);
    HAL_UART_Transmit_DMA(huart, (uint8_t *)(&tx_data), sizeof(tx_data));
  }

  void video_data_tx(void) // id0x0310 视频数据发送（300字节）
  {
    Data_Concatenation((uint8_t *)video_buffer, (uint8_t *)(&tx_data), 300, 0x0310);

    // 使用阻塞发送，并统计状态
    HAL_StatusTypeDef status = HAL_UART_Transmit(huart, (uint8_t *)(&tx_data), sizeof(tx_data), 100);
    last_status              = status;

    // 统计发送状态
    if (status == HAL_OK) {
      tx_ok_cnt++;
    } else if (status == HAL_BUSY) {
      tx_busy_cnt++;
    } else if (status == HAL_TIMEOUT) {
      tx_timeout_cnt++;
    } else {
      tx_error_cnt++;
    }
  }

  void Tc_RxCplt_IRQHandler(void)
  {
    uint32_t tmp_flag = 0;
    uint32_t temp;
    tmp_flag = __HAL_UART_GET_FLAG(this->huart, UART_FLAG_IDLE);
    if (tmp_flag != RESET) {
      __HAL_UART_CLEAR_IDLEFLAG(this->huart);
      temp = this->huart->Instance->SR;
      temp = this->huart->Instance->DR;
      HAL_UART_DMAStop(this->huart);
      this->data_deal();
      this->VT13_YK_deal();
      HAL_UART_Receive_DMA(this->huart, tuchuan_rx_buffer, sizeof(tuchuan_rx_buffer));
    }
  }

  void VT13_YK_deal(void)
  {
    if (this->tuchuan_rx_buffer[0] == 0xA9 && this->tuchuan_rx_buffer[1] == 0x53) {
      this->VT13_Data.ch_0    = ((tuchuan_rx_buffer[2] | (tuchuan_rx_buffer[3] << 8)) & 0x07ff) - 1024;
      this->VT13_Data.ch_1    = (((tuchuan_rx_buffer[3] >> 3) | (tuchuan_rx_buffer[4] << 5)) & 0x07ff) - 1024;
      this->VT13_Data.ch_2    = (((tuchuan_rx_buffer[4] >> 6) | (tuchuan_rx_buffer[5] << 2) |
                                  (tuchuan_rx_buffer[6] << 10)) &
                                 0x07ff) -
                                1024;
      this->VT13_Data.ch_3    = (((tuchuan_rx_buffer[6] >> 1) | (tuchuan_rx_buffer[7] << 7)) & 0x07ff) - 1024;
      this->VT13_Data.mode_sw = ((tuchuan_rx_buffer[7] >> 4) & 0x0003);
      this->VT13_Data.pause   = ((tuchuan_rx_buffer[7] >> 6) & 0x01);
      this->VT13_Data.fn_1    = ((tuchuan_rx_buffer[7] >> 7) & 0x01);
      this->VT13_Data.fn_2    = ((tuchuan_rx_buffer[8] >> 0) & 0x01);
      this->VT13_Data.wheel   = (((tuchuan_rx_buffer[8] >> 1) | (tuchuan_rx_buffer[9] << 7)) & 0x07FF) - 1024;
      this->VT13_Data.trigger = (tuchuan_rx_buffer[9] >> 4) & 0x01;

      this->VT13_Data.mouse_x      = (tuchuan_rx_buffer[10] | (tuchuan_rx_buffer[11] << 8));
      this->VT13_Data.mouse_y      = (tuchuan_rx_buffer[12] | (tuchuan_rx_buffer[13] << 8));
      this->VT13_Data.mouse_z      = (tuchuan_rx_buffer[14] | (tuchuan_rx_buffer[15] << 8));
      this->VT13_Data.mouse_left   = (tuchuan_rx_buffer[16] >> 0) & 0x03;
      this->VT13_Data.mouse_right  = (tuchuan_rx_buffer[16] >> 2) & 0x03;
      this->VT13_Data.mouse_middle = (tuchuan_rx_buffer[16] >> 4) & 0x03;
      this->VT13_Data.key          = (tuchuan_rx_buffer[17] | (tuchuan_rx_buffer[18] << 8));
      this->VT13_Data.crc16        = (tuchuan_rx_buffer[19] | (tuchuan_rx_buffer[20] << 8));
    }
  }

  uint32_t rx_crc_ok_cnt  = 0; // 接收CRC校验成功计数
  uint32_t rx_crc_err_cnt = 0; // 接收CRC校验失败计数

  void data_deal(void)
  {
    if (this->tuchuan_rx_buffer[0] == 0XA5 && this->tuchuan_rx_buffer[6] == 0x03) {
      rx_cnt++;

      // 🔍 CRC校验检查
      uint16_t data_len  = (this->tuchuan_rx_buffer[2] << 8) | this->tuchuan_rx_buffer[1];
      uint16_t total_len = 5 + 2 + data_len + 2; // 帧头5 + CMD_ID2 + 数据段 + CRC16

      // 检查CRC16（整个数据包）
      if (IsCrc16Good(this->tuchuan_rx_buffer, total_len)) {
        rx_crc_ok_cnt++; // CRC正确
      } else {
        rx_crc_err_cnt++; // CRC错误
      }

      if (tuchuan_rx_buffer[5] == 0x09) // 机器人发自定义控制器
      {
        memcpy(&poly_Custom_Robo_data.j0_f.c[0], &tuchuan_rx_buffer[7], 4);
        memcpy(&poly_Custom_Robo_data.j1_f.c[0], &tuchuan_rx_buffer[7 + 4], 4);
        memcpy(&poly_Custom_Robo_data.j2_f.c[0], &tuchuan_rx_buffer[7 + 4 + 4], 4);
        memcpy(&poly_Custom_Robo_data.j3_f.c[0], &tuchuan_rx_buffer[7 + 4 + 4 + 4], 4);
        memcpy(&poly_Custom_Robo_data.j4_f.c[0], &tuchuan_rx_buffer[7 + 4 + 4 + 4 + 4], 4);
        memcpy(&poly_Custom_Robo_data.j5_f.c[0], &tuchuan_rx_buffer[7 + 4 + 4 + 4 + 4 + 4], 4);
        poly_Custom_Robo_data.tc_flag = tuchuan_rx_buffer[7 + 4 + 4 + 4 + 4 + 4 + 4];

        if (isnan(poly_Custom_Robo_data.j0_f.f) ||
            isnan(poly_Custom_Robo_data.j1_f.f) ||
            isnan(poly_Custom_Robo_data.j2_f.f) ||
            isnan(poly_Custom_Robo_data.j3_f.f) ||
            isnan(poly_Custom_Robo_data.j4_f.f) ||
            isnan(poly_Custom_Robo_data.j5_f.f)) {
          this->set_robo_zero();
          err_cnt++;
        } else {
          Custom_Robo_data.j0_f.f  = poly_Custom_Robo_data.j0_f.f;
          Custom_Robo_data.j1_f.f  = poly_Custom_Robo_data.j1_f.f;
          Custom_Robo_data.j2_f.f  = poly_Custom_Robo_data.j2_f.f;
          Custom_Robo_data.j3_f.f  = poly_Custom_Robo_data.j3_f.f;
          Custom_Robo_data.j4_f.f  = poly_Custom_Robo_data.j4_f.f;
          Custom_Robo_data.j5_f.f  = poly_Custom_Robo_data.j5_f.f;
          Custom_Robo_data.tc_flag = poly_Custom_Robo_data.tc_flag;

          this->robo_feed_watchdog();
          rx_302id_cnt++;
        }

        rx_309id_cnt++;
      } else
        this->set_zero();
    }
  }

  uint8_t Pressed_Check(uint16_t keyvalue)
  {
    if (this->jianpan & keyvalue)
      return 1;
    else
      return 0;
  }

  static unsigned char Get_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength)
  {
    unsigned char ucCRC8 = 0xff;
    unsigned char ucIndex;
    static const unsigned char CRC8_TAB[256] = {
        0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
        0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e, 0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
        0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
        0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
        0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
        0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
        0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
        0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
        0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
        0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
        0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
        0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
        0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
        0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
        0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
        0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35};
    while (dwLength--) {
      ucIndex = ucCRC8 ^ (*pchMessage++);
      ucCRC8  = CRC8_TAB[ucIndex];
    }
    return (ucCRC8);
  }

  static void Append_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength)
  {
    unsigned char ucCRC = 0;
    if ((pchMessage == 0) || (dwLength <= 2))
      return;
    ucCRC                    = Get_CRC8_Check_Sum((unsigned char *)pchMessage, dwLength - 1);
    pchMessage[dwLength - 1] = ucCRC;
  }

  static uint16_t Get_CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength)
  {
    uint8_t chData;
    uint16_t wCRC                         = 0xffff;
    static const uint16_t wCRC_Table[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5,
        0x472c, 0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 0x2102, 0x308b,
        0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c,
        0xd9f5, 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42, 0x9ed9, 0x8f50,
        0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c,
        0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528,
        0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 0x6306, 0x728f, 0x4014,
        0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb,
        0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9,
        0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf,
        0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791,
        0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 0xb58b,
        0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e,
        0x5cf5, 0x4d7c, 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd, 0x6956,
        0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a,
        0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 0xf78f, 0xe606,
        0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1,
        0x0f78};
    if (pchMessage == NULL) {
      return 0xFFFF;
    }
    while (dwLength--) {
      chData = *pchMessage++;
      (wCRC) = ((uint16_t)(wCRC) >> 8) ^ wCRC_Table[((uint16_t)(wCRC) ^ (uint16_t)(chData)) & 0x00ff];
    }
    return wCRC;
  }

  static void Append_CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength)
  {
    uint16_t wCRC = 0;
    if ((pchMessage == NULL) || (dwLength <= 2))
      return;
    wCRC                     = Get_CRC16_Check_Sum((uint8_t *)pchMessage, dwLength - 2);
    pchMessage[dwLength - 2] = (uint8_t)(wCRC & 0x00ff);
    pchMessage[dwLength - 1] = (uint8_t)((wCRC >> 8) & 0x00ff);
  }
};

// 状态定义
// #define	YK_MODE_SW_C			((uint16_t)0)
// #define	YK_MODE_SW_N			((uint16_t)1)
// #define	YK_MODE_SW_S			((uint16_t)2)

// 状态变化类型枚举
typedef enum {
  TRANS_NONE, // 无变化
  C_N,        // C -> N
  C_S,        // C -> S
  N_C,        // N -> C
  N_S,        // N -> S
  S_C,        // S -> C
  S_N         // S -> N
} TransitionState;

// 状态检测类
class YKStateTransitionDetector
{
  public:
  // 构造函数：需要初始状态
  YKStateTransitionDetector(uint16_t initial_state) : prev_state(initial_state) {}

  // 状态更新函数：返回状态变化类型
  TransitionState update(uint16_t current_state)
  {
    if (current_state == prev_state) {
      return TRANS_NONE; // 状态未变化
    }

    // 通过状态组合判断变化类型
    TransitionState transition = getTransitionType(prev_state, current_state);
    prev_state                 = current_state; // 更新历史状态
    return transition;
  }

  private:
  uint16_t prev_state; // 保存上一次状态

  // 状态变化映射表
  TransitionState getTransitionType(uint16_t prev, uint16_t curr)
  {
    // UP 状态分支
    if (prev == YK_MODE_SW_C) {
      if (curr == YK_MODE_SW_N)
        return C_N;
      if (curr == YK_MODE_SW_S)
        return C_S;
    }
    // MID 状态分支
    else if (prev == YK_MODE_SW_N) {
      if (curr == YK_MODE_SW_C)
        return N_C;
      if (curr == YK_MODE_SW_S)
        return N_S;
    }
    // DOWN 状态分支
    else if (prev == YK_MODE_SW_S) {
      if (curr == YK_MODE_SW_C)
        return S_C;
      if (curr == YK_MODE_SW_N)
        return S_N;
    }
    return TRANS_NONE;
  }
};

/**************************************** ADXRS290 **********************************************************/
#ifdef __SPI_H__

#define ADXRS290_ADI_ID     0x00
#define ADXRS290_MEMS_ID    0x01
#define ADXRS290_DEV_ID     0x02
#define ADXRS290_REV_ID     0x03
#define ADXRS290_SN0        0x04
#define ADXRS290_SN1        0x05
#define ADXRS290_SN2        0x06
#define ADXRS290_SN3        0x07
#define ADXRS290_DATAX0     0x08
#define ADXRS290_DATAX1     0x09
#define ADXRS290_DATAY0     0x0A
#define ADXRS290_DATAY1     0x0B
#define ADXRS290_TEMP0      0x0C
#define ADXRS290_TEMP1      0x0D
#define ADXRS290_POWER_CTL  0x10
#define ADXRS290_Filter     0x11
#define ADXRS290_DATA_READY 0x12

typedef struct gyro {
  float v;
  float v_nonoise;
  float theta_euler;
  float bias;
  uint32_t dev_count;
} ADXRS290_TYPEDEF;

typedef enum {
  ADXRS290_OK        = 0x00U,
  ADXRS290_SET_ERROR = 0x01U,
  ADXRS290_ID_ERROR  = 0x02U,
  ADXRS290_ERROR     = 0x03U,
} ADXRS290_StatusTypeDef;

class ADXRS290
{
  public:
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *GPIOx;
  uint16_t GPIO_Pin;

  ADXRS290_TYPEDEF sensor_data_X;
  ADXRS290_TYPEDEF sensor_data_Y;

  ADXRS290_StatusTypeDef Init(uint8_t hpf_corner, uint8_t odr_lpf);
  void adxrs290_update(void);
  ADXRS290_StatusTypeDef adxrs290_writeByte(uint8_t subAddress, uint8_t data);
  uint8_t adxrs290_readByte(uint8_t subAddress);
  void adxrs290_readBytes(uint8_t subAddress, uint8_t count, uint8_t *spi_rev_buf);

  ADXRS290(SPI_HandleTypeDef *q, GPIO_TypeDef *w, uint16_t e, uint16_t t, float y, const char *u) : hspi(q), GPIOx(w), GPIO_Pin(e), SELF_TEST_NUM_290(t), DEAD_ZONE_290(y), string_check_290(u) {}

  private:
  uint16_t SELF_TEST_NUM_290;
  float DEAD_ZONE_290;
  const char *string_check_290;
  void ADXRS290_SPI_ON() { HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET); }
  void ADXRS290_SPI_OFF() { HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET); }
};
#endif

/*****************************************ADXRS453*******************************************/
#ifdef __SPI_H__

typedef struct
{
  float v;
  float v_nonoise;
  float theta_euler;
  float bias;
  float offset_v;
  float offset_max;
  float offset_min;
  uint32_t dev_count;
  uint8_t calibration;
} ADXRS453_TYPEDEF;
typedef enum {
  ADXRS453_OK        = 0x00U,
  ADXRS453_RW_ERROR  = 0x01U,
  ADXRS453_P0_ERROR  = 0x02U, // P0是奇偶校验位，为比特建立奇偶校验。[31:16]设备响应的。
  ADXRS453_P1_ERROR  = 0x03U, // P1是奇偶校验位，它为整个数据建立奇偶校验32位设备响应。
  ADXRS453_SPI_ERROR = 0x04U,
  ADXRS453_RE_ERROR  = 0x05U,
  ADXRS453_DU_ERROR  = 0x06U,
  ADXRS453_PLL_ERROR = 0x07U,
  ADXRS453_Q_ERROR   = 0x08U,
  ADXRS453_NVM_ERROR = 0x09U,
  ADXRS453_POR_ERROR = 0x0AU,
  ADXRS453_PWR_ERROR = 0x0BU,
  ADXRS453_CST_ERROR = 0x0CU,
  ADXRS453_CHK_ERROR = 0x0DU,
  ADXRS453_ERROR
} ADXRS453_StatusTypeDef;

class ADXRS453
{
  public:
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *GPIOx;
  uint16_t GPIO_Pin;
  TIM_HandleTypeDef *htim;

  ADXRS453_TYPEDEF sensor_data;

  ADXRS453_StatusTypeDef Init();
  ADXRS453_StatusTypeDef adxrs453_update(void);
  ADXRS453_StatusTypeDef sensor(bool CHK, int16_t *date);
  uint32_t TransmitReceive(uint32_t address);
  ADXRS453_StatusTypeDef addread(uint8_t address, int16_t *date);

  ADXRS453(SPI_HandleTypeDef *q, GPIO_TypeDef *w, uint16_t e, TIM_HandleTypeDef *r, uint16_t t, float y, const char *u)
      : hspi(q), GPIOx(w), GPIO_Pin(e), htim(r), SELF_TEST_NUM(t), DEAD_ZONE(y), string_check(u)
  {
  }

  private:
  uint16_t SELF_TEST_NUM;
  float DEAD_ZONE;
  const char *string_check;
  void SPI_ON() { HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET); }
  void SPI_OFF() { HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET); }
  bool odd_check(uint32_t date);
};
#endif

/**************************************BMI088************************************************/
#ifdef __SPI_H__

#define ACC_CHIP_ID       0x00
#define ACC_ERR_REG       0X02
#define ACC_STATUS        0X03
#define ACC_X_LSB         0X12
#define ACC_X_MSB         0X13
#define ACC_Y_LSB         0X14
#define ACC_Y_MSB         0X15
#define ACC_Z_LSB         0X16
#define ACC_Z_MSB         0X17
#define SENSORTIME_0      0X18
#define SENSORTIME_1      0X19
#define SENSORTIME_2      0X1A
#define ACC_INT_STAT_1    0X1D
#define TEMP_MSB          0X22
#define TEMP_LSB          0X23
#define ACC_CONF          0X40
#define ACC_RANGE         0X41
#define INT1_IO_CTRL      0X53
#define INT2_IO_CTRL      0X54
#define INT_MAP_DATA      0X58
#define ACC_SELF_TEST     0X6D
#define ACC_PWR_CONF      0X7C
#define ACC_PWR_CTRL      0X7D
#define ACC_SOFTRESET     0X7E

#define GYRO_CHIP_ID      0X00
#define RATE_X_LSB        0X02
#define RATE_X_MSB        0X03
#define RATE_Y_LSB        0X04
#define RATE_Y_MSB        0X05
#define RATE_Z_LSB        0X06
#define RATE_Z_MSB        0X07
#define GYRO_INT_STAT_1   0X0A
#define GYRO_RANGE        0X0F
#define GYRO_BANDWIDTH    0X10
#define GYRO_LPM1         0X11
#define GYRO_SOFTRESET    0X14
#define GYRO_INT_CTRL     0X15
#define INT3_INT4_IO_CONF 0X16
#define INT3_INT4_IO_MAP  0X18
#define GYRO_SELF_TEST    0X3C
float inVSqrt(float x);
typedef struct {
  struct {
    float x;
    float y;
    float z;
    float LPF_x;
    float LPF_y;
    float LPF_z;
  } acc;

  struct {
    float x_nonoise;
    float y_nonoise;
    float z_nonoise;
    struct {
      float x;
      float y;
      float z;
    } calibration;
    struct {
      float x;
      float y;
      float z;
    } origin;
    struct {
      float x;
      float y;
      float z;
    } dynamicSum;
    struct {
      float x;
      float y;
      float z;
    } offset;
    struct {
      float x;
      float y;
      float z;
    } offset_max;
    struct {
      float x;
      float y;
      float z;
    } offset_min;
    struct {
      float x;
      float y;
      float z;
    } dps;
    struct {
      float x;
      float y;
      float z;
    } LPF;
  } gyro;

  struct {
    float x;
    float y;
    float z;
    float now_x;
    float now_y;
    float now_z;
    float last_x;
    float last_y;
    float last_z;
    float Real_x;
    float Real_y;
    float Real_z;
  } mang;

  uint16_t runningTimes;
  float temperature;
  uint8_t calibration;

} BMI088_TYPEDEF;

typedef enum {
  BMI088_OK             = 0x00U,
  BMI088_SET_ERROR      = 0x01U,
  BMI088_ACC_ID_ERROR   = 0x02U,
  BMI088_GYRO_ID_ERROR  = 0x03U,
  BMI088_ERROR          = 0x04U,
  BMI088_SELFTEXT_ERROR = 0x05U,
} BMI088_StatusTypeDef;

typedef enum {
  BMI088_GYRO_RANGE_2000 = 0x00U,
  BMI088_GYRO_RANGE_1000 = 0x01U,
  BMI088_GYRO_RANGE_500  = 0x02U,
  BMI088_GYRO_RANGE_250  = 0x03U,
  BMI088_GYRO_RANGE_125  = 0x04U,
} BMI088_GyroRangeTypeDef;

typedef enum {
  BMI088_ACC_RANGE_3  = 0X00U,
  BMI088_ACC_RANGE_6  = 0X01U,
  BMI088_ACC_RANGE_12 = 0X02U,
  BMI088_ACC_RANGE_24 = 0X03U,
} BMI088_AccRangeTypeDef;

struct {
  float CUTOFF_FREQ = 50.0f;     // 截止频率
  float SAMPLE_RATE = 0.5f;      // 采样周期
  float pi          = 3.1415926; // π
  float alpha;                   // 滤波系数
} LPF_factor;

typedef struct {
  int16_t roundYaw;
  int16_t roundPitch;
  int16_t roundRoll;
} angleRound_t;

/*四元数↓*/

typedef struct {
  int16_t roundYaw;
  int16_t roundPitch;
  int16_t roundRoll;
} angleRound;

/*二维float向量结构体*/

void BMI_CrossRound_err(void);

typedef struct vec2f {
  float data[2];
} vec2f;

/*二维int16向量结构体*/

typedef struct vec2int16 {
  short data[2];
} vec2int16;

/*三维float向量结构体*/

typedef struct vec3f {
  float data[3];
} vec3f;

/*三维int16向量结构体*/

typedef struct vec3int16 {
  short data[3];
} vec3int16;

/*四维float向量结构体*/

typedef struct vec4f {
  float data[4];
} vec4f;

/*四维int16向量结构体*/

typedef struct vec4int16 {
  short data[4];
} vec4int16;

/*结构体*/

typedef struct accdata {
  vec3int16 origin;      // 原始值
  vec3f offset_max;      // 零偏值最大值
  vec3f offset_min;      // 零偏值最小值
  vec3f offset;          // 零偏值
  vec3f calibration;     // 校准值
  vec3f filter;          // 滑动平均滤波值
  vec3f accValue;        // 加速度值，单位：m/s2
  vec3f dynamicSum;      // 校准时求和计算
  uint16_t runningTimes; // 运行次数
} accdata;

typedef struct gyrodata {
  vec3int16 origin;  // 原始值
  vec3f offset_max;  // 零偏值最大值
  vec3f offset_min;  // 零偏值最小值
  vec3f offset;      // 零偏值
  vec3f calibration; // 校准值
  vec3f filter;      // 滑动平均滤波值
  vec3f dps;         // 度每秒
  vec3f radps;       // 弧度每秒
  vec3f dynamicSum;  // 校准时求和计算
} gyrodata;

typedef struct {
  float x;
  float y;
  float z;
} Deal_acc_t;
typedef struct {
  float x;
  float y;
  float z;
} Deal_gyro_t;

typedef struct {
  float q0;
  float q1;
  float q2;
  float q3;
} quaterInfo_t;

typedef struct {
  float pitch;
  float roll;
  float yaw;
} eulerianAngles_t;

typedef struct
{
  float pitch;
  float roll;
  float yaw;
  float Deal_pitch;
  float Deal_roll;
  float Deal_yaw;
} Anglespeed_t;

/*四元数↑*/

class BMI088
{
  public:
  uint8_t calibrationState;
  SPI_HandleTypeDef *hspi;
  TIM_HandleTypeDef *htim;
  GPIO_TypeDef *CSB1_GPIOx, *CSB2_GPIOx;
  uint16_t CSB1_GPIO_Pin, CSB2_GPIO_Pin;
  angleRound_t Round;
  BMI088_TYPEDEF sensor_data;
  BMI088_StatusTypeDef Init_High(void);
  BMI088_StatusTypeDef Init(void);
  Deal_acc_t Deal_acc;
  Deal_gyro_t Deal_gyro;
  accdata acc;
  gyrodata gyro;
  eulerianAngles_t eulerAngle; // 欧拉角
  eulerianAngles_t lastAngle;  // 上一次的欧拉角
  eulerianAngles_t nowAngle;   // 现在的欧拉角
  eulerianAngles_t realAngle;  // 现在真实的欧拉角（已经叠加了圈数）
  Anglespeed_t Anglespeed;
  float q0_t;
  float q1_t;
  float q2_t;
  float q3_t;
  /*****************************************************************************     新     **************************************************************************************************************/

  float last_angle_speed_pitch, last_angle_speed_roll, last_angle_speed_yaw;
  uint8_t angle_speed_filter_count_pitch, angle_speed_filter_count_roll, angle_speed_filter_count_yaw;

  /*****************************************************************************     新     **************************************************************************************************************/

  quaterInfo_t Q_info = {1, 0, 0, 0}; // 全局四元数
  void BMI088_write_Acc(uint8_t subAddress, uint8_t data);
  void BMI088_write_Gyro(uint8_t subAddress, uint8_t data);
  void BMI088_read_Acc(uint8_t subAddress, uint8_t len, uint8_t *spi_rev_buf);
  void BMI088_read_Gyro(uint8_t subAddress, uint8_t len, uint8_t *spi_rev_buf);
  void set_zero(void);
  void low_pass_filter_init(void);
  float low_pass_filter(float value);
  void BMI088_update(void);
  void BMI088_New_update(void);
  void BMI_Get_EulerAngle(void);
  void getValues(void);
  void QuatToEulerAngles(void);
  void analyse(void);
  void BMI_analyse(void);
  void BMI_QuatToEulerAngles(void);
  void BMI_CrossRound(void);
  void BMI_CrossRound_err(void);
  void BMI088_AHRS(float gx, float gy, float gz, float ax, float ay, float az);
  void Analyse_speed(void);
  BMI088(SPI_HandleTypeDef *q, TIM_HandleTypeDef *t, GPIO_TypeDef *w1, uint16_t p1, GPIO_TypeDef *w2, uint16_t p2, uint16_t num, float dz, BMI088_GyroRangeTypeDef gyrorange, BMI088_AccRangeTypeDef accrange, const char *u, uint8_t enacc = 0) : hspi(q), htim(t), CSB1_GPIOx(w1), CSB1_GPIO_Pin(p1), CSB2_GPIOx(w2), CSB2_GPIO_Pin(p2), SELF_TEST_NUM(num), dead_zoom(dz), GyroRange(gyrorange), AccRange(accrange), string_check_088(u), enable_acc(enacc) {}

  private:
  BMI088_GyroRangeTypeDef GyroRange; // 陀螺仪量程
  BMI088_AccRangeTypeDef AccRange;   // 加速度量程
  float GyroResolution;              // 陀螺仪分辨率
  float AccRangsetting;              // 设置量程为
  float Acc_Temperature_Offset = 0, Gyro_Temperature_Offset = 0;
  float dead_zoom;
  uint8_t enable_acc;
  uint16_t SELF_TEST_NUM;
  const char *string_check_088;
  uint16_t timer_1ms          = 0;
  uint8_t selftext_error_flag = 0, selftext_reset_step = 0;
  float last_gyro_x, last_gyro_y, last_gyro_z, last_temperature, filter_count_x, filter_count_y, filter_count_z, filter_count_temperature;

  void BMI088_SPI_ON(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
  {
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
  }
  void BMI088_SPI_OFF(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
  {
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
  }
  void BMI088_writeByte(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t subAddress, uint8_t data);
  void BMI088_readBytes(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t subAddress, uint8_t len, uint8_t *spi_rev_buf);
  void selftext_error_reset(void);
};

#endif

/**************************************Vision_LPF***********************************************/
class Vision_LPF
{
  public:
  void Vision_Low_Pass_Filter_Init(void);
  float Vision_Low_Pass_Filter(float value);
  Vision_LPF(float CF, float SR, float V_PI) : CUTOFF_FREQ(CF), SAMPLE_RATE(SR), pi(V_PI) {}
  float CUTOFF_FREQ; // 截止频率
  float SAMPLE_RATE; // 采样周期
  float pi;          // π
  float alpha;       // 滤波系数
  float b;
  float out_last = 0; // 上一次滤波值
  float out;
};
/**************************************PWM_MCL***********************************************/
#ifdef __TIM_H__

class MCL_snail
{
  public:
  TIM_HandleTypeDef *htim;
  TIM_HandleTypeDef *htim_z;
  uint32_t Channel_z;
  TIM_HandleTypeDef *htim_y;
  uint32_t Channel_y;

  void Init(void);
  void Init_XC_Calibration(uint8_t speed_z_max, uint8_t speed_y_max);
  void Init_Change_Steer(uint8_t dir, uint8_t speed_max);
  void stop(void);
  void run(uint8_t grade);
  HAL_StatusTypeDef shoot_state(void);
  void state_tick(TIM_HandleTypeDef *p);
  void set_speed(uint8_t speed_z, uint8_t speed_y);

  MCL_snail(TIM_HandleTypeDef *htim, TIM_HandleTypeDef *htim_z, uint32_t Channel_z, TIM_HandleTypeDef *htim_y, uint32_t Channel_y, uint8_t grade_1, uint8_t grade_2, uint8_t grade_3,
            uint8_t grade_1_error, uint8_t grade_2_error, uint8_t grade_3_error, const char *string_check)
      : htim(htim), htim_z(htim_z), Channel_z(Channel_z), htim_y(htim_y), Channel_y(Channel_y), grade_1(grade_1), grade_2(grade_2), grade_3(grade_3), grade_1_error(grade_1_error),
        grade_2_error(grade_2_error), grade_3_error(grade_3_error), string_check(string_check)
  {
  }

  private:
  uint8_t grade_1, grade_2, grade_3;
  uint8_t grade_1_error, grade_2_error, grade_3_error;
  const char *string_check;
  uint8_t shoot_state_byte, run_stete;
  uint32_t time_20ms;
  uint8_t first_state;
};
#endif

/*************************************UD_check**********************************************/
typedef enum {
  UpDown_check_nothing,
  UpDown_check_falling,
  UpDown_check_rising
} UpDown_check_state;
class UpDown_check_class
{
  public:
  UpDown_check_class(bool initial_conditions) : bit(initial_conditions) {}
  UpDown_check_state updata(bool Condition)
  {
    if (((Condition) != 0) && ((bit & 1) == 0)) {
      bit |= 1;
      return UpDown_check_rising;
    } else if (!((Condition) != 0) && ((bit & 1) != 0)) {
      bit &= ~1;
      return UpDown_check_falling;
    } else
      return UpDown_check_nothing;
  }

  private:
  bool bit;
};

#endif

/*************************************RGB**********************************************/
#ifdef __TIM_H__

#define PIXEL_NUM 5                      // 灯珠数
#define NUM       (24 * PIXEL_NUM + 300) // Reset 280us（复位时间） / 1.25us = 224   NUM的值为单个灯珠的位宽（24）* 灯珠数量（PIXEL_NUM）+ 复位脉冲数    1/84M = 11.9ns
#define WS1       75                     // 重装值105
#define WS0       30

class RGB_UI
{
  public:
  TIM_HandleTypeDef *htim;
  uint32_t Channel;
  void RGB_UI_Init(void);
  void WS_Load(void);
  void WS_WriteAll_RGB(uint8_t n_R, uint8_t n_G, uint8_t n_B);
  void WS_CloseAll(void);
  void WS281x_SetPixelRGB(uint16_t n, uint8_t red, uint8_t green, uint8_t blue);

  RGB_UI(TIM_HandleTypeDef *htim, uint32_t Channel, const char *string_check_rgb_ui) : htim(htim), Channel(Channel), string_check_rgb_ui(string_check_rgb_ui) {}

  private:
  uint16_t send_Buf[NUM];
  const char *string_check_rgb_ui;
  uint32_t WS281x_Color(uint8_t red, uint8_t green, uint8_t blue);
  void WS281x_SetPixelColor(uint16_t n, uint32_t GRBColor);
};

/**LPB60B激光测距 */
// class LPB60B
//{
// private:
//     UART_HandleTypeDef *huart;
//     DMA_HandleTypeDef *hdma_usart_rx;
//     uint8_t LP_rxbuffer[64];
//     uint8_t cmd_txbuffer[8];
//     const uint8_t cmdid_1[8]={0x55,0x01,0x00,0x00,0x00,0x00,0xD3,0xAA};
//     const uint8_t cmdid_2[8]={0x55,0x30,0x00,0x00,0x00,0x00,0x84,0xAA};
//     const uint8_t cmdid3

// public:
//     HAL_StatusTypeDef Init(void)
//     {
//         __HAL_DMA_DISABLE_IT(hdma_usart_rx, DMA_IT_HT); // 关闭dma传输过半中断
//         HAL_UARTEx_ReceiveToIdle_DMA(huart, LP_rxbuffer, sizeof(LP_rxbuffer));

//        // __HAL_UART_ENABLE_IT(this->huart, UART_IT_IDLE);
//        // return HAL_UART_Receive_DMA(this->huart, LP_rxbuffer, 128);
//    }
//    void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) // 不定长接收视觉数据
//    {

//        if (huart == this->huart)
//        {
//            HAL_UARTEx_ReceiveToIdle_DMA(huart, LP_rxbuffer, sizeof(LP_rxbuffer));
//            __HAL_DMA_DISABLE_IT(hdma_usart_rx, DMA_IT_HT); // 关闭dma传输过半中断
//        }
//    }
//    // 0x55,Key,value[4],crc8,0xAA
//    // Key 字段，此处表示此数据包为接收数据。
//    // Value 字段的高字节，此处表示系统状态，0表示系统正常。
//    // 表示测量的距离值，16进制表示，单位是mm。

//    // 01获取设备信息 2获取温度信息 3设置测量频率
//    // 4设置数据格式 5启动测量 6停止测量
//    // 7测量数据返回 8保存设置 A获取序列号
//    // D设置测量模式 E高速测量数据返回
//    // 11配置设备地址 12设置波特率
//    HAL_StatusTypeDef cmdtx(uint8_t cmd_id)
//    {
//        switch (cmd_id)
//        {
//        case 0x01:
//        {
//            cmd_txbuffer[0] = 0x55;
//            cmd_txbuffer[1] =
//                cmd_txbuffer[2] =
//                    cmd_txbuffer[3] =
//                        cmd_txbuffer[4] =
//                            cmd_txbuffer[5] =
//                                cmd_txbuffer[6] =
//                                    cmd_txbuffer[7] = AA;
//            break;
//        }

//        case 0X02:
//        {
//            cmd_txbuffer[0] = 0x55;
//            cmd_txbuffer[1] =
//                cmd_txbuffer[2] =
//                    cmd_txbuffer[3] =
//                        cmd_txbuffer[4] =
//                            cmd_txbuffer[5] =
//                                cmd_txbuffer[6] =
//                                    cmd_txbuffer[7] = AA;
//            break;
//        }

//        default:
//            break;
//        }
//    }

//    /* 生成多项式为CRC-8x8+x5+x4+1 0x31(0x131) */
//    uint8_t crc_high_first(uint8_t *ptr, uint8_t data_len)
//    {
//        uint8_t i;
//        uint8_t crc = 0x00;
//        while (data_len--)
//        {
//            crc ^= *ptr++;
//            for (uint8_t ii = 8; ii > 0; --ii)
//            {
//                if (crc & 0x80)
//                    crc = (crc << 1) ^ 0x31;
//                else
//                    crc = (crc << 1);
//            }
//        }
//        return crc;
//    }

//    LPB60B(UART_HandleTypeDef *uart, DMA_HandleTypeDef *dma_usart_rx) : huart(uart), hdma_usart_rx(dma_usart_rx);
//};

#endif