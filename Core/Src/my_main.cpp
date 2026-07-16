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

/* ===== include 环境:与原 main.c 一致(搬运业务所需) ===== */
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

// 骨架阶段:空实现。业务仍在 main.c,行为不变。

/* ===== main.c PTD: globals + business functions (verbatim, order preserved) ===== */

uint8_t a,b,c,d,e,f,g,h,o,p;
#define MCL_KP 20.0F
uint32_t counter;
uint8_t XTL_flag = 0;
#define Laser_ON HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET) // 锟?鍏夊紑
#define Laser_OFF HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET)
// 璁℃暟锟?
uint8_t p_cnt, y_cnt, bp_cnt, mcl_cnt, zm_cnt, js_cnt,bj,imu_cnt,zm_press1,zm_press2,m2006_cnt,duoji;
/********   鎺у埗妯″紡   **********/
#define PROTECT_MODE 0
#define GYRO_MODE 2
#define MANG_MODE 1
#define SPEED_MODE 3
#define DUZHUAN_MODE 5 // 2006涓婄數鍫佃浆璁颁綅缃?
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

/********   鐢垫満   **********/
USER_CAN CAN_1(&hcan1, 0), CAN_2(&hcan2, 1);
MOTOR_RM  BoPan(0x201, &CAN_2),Mini_Pitch_2006(0x202, &CAN_2);
int16_t Mini_Pitch_output = 0; // Mini_Pitch鐢垫満鐢垫祦杈撳嚭锛堜緵閬ユ祴浣跨敤锛屾枃浠朵綔鐢ㄥ煙锛?

MOTOR_RM  Motor_MCL_UP_up(0x202, &CAN_1), 
					Motor_MCL_R(0x203, &CAN_1),         
					Motor_MCL_L(0x201, &CAN_1),       
					Motor_MCL_RR(0x206, &CAN_1),  
					Motor_MCL_UP(0x205, &CAN_1),        
					Motor_MCL_LL(0x204, &CAN_1);       

MOTOR_LK Motor_LK6010_Pitch(0X141, &CAN_2), Motor_LK6010_Yaw(0x142, &CAN_2); // LK6010鐢垫満 Pitch鍜孻aw+

PID_class PID_Mini_Pitch_2006_mang(0.6, 0, 0, 30000, 0, 30000, 16000, 0,0), // 0.178, 0, 0, 30000, 0, 0, 30000, 0,0                       
          PID_Mini_Pitch_2006_sp(0.6, 0, 0, 30000, 0, 0, 10000, 0, 0); // 1.5  C610 ESC max current cmd +-10000; was 10, 0, 0, 16000, 0, 0, 16000, 0, 0

PID_class PID_BP_mang(0.6f, 0, 0, 30000, 0, 0, 30000, 0,0), // 0.6f, 0, 0, 30000, 0, 0, 30000, 0,0
          PID_BP_sp(20, 0, 0, 16000, 0, 0, 16000, 0, 0); // 20, 0, 0, 16000, 0, 0, 16000, 0, 0
          
PID_class PID_MCL_UP_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0), // 涓婅疆閫熷害PID
          PID_MCL_R_sp(MCL_KP, 0, 0, 16000, 0, 0, 16000, 0,0), // 鍙宠疆閫熷害PID
          PID_MCL_L_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0,0), // 宸﹁疆閫熷害PID
          PID_MCL_RR_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0),   // 鍙冲悗锟? sp+
          PID_MCL_UPUP_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0), // 涓婅疆 sp-20
          PID_MCL_LL_sp(MCL_KP, 0, 0, 16000, 0, 0, 19000, 0, 0);   // 宸﹀悗锟? sp-

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
					
/*pitch鎷х揣鍙傛暟*/		
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

extern DMA_HandleTypeDef hdma_usart6_rx; // 澹版槑DMA鍙ユ焺
TuChuan TC(&huart6, &hdma_usart6_rx);

RGB_UI RGB_UI(&htim3,TIM_CHANNEL_3,"put the Update function in 800kHz interrupt");
//RGB_UI RGB_UI(&htim4,TIM_CHANNEL_4,"put the Update function in 800kHz interrupt");
//RGB_UI RGB_UI(&htim3,TIM_CHANNEL_1,"put the Update function in 800kHz interrupt");

// ========== RGB Debug 鐩稿叧鍙橀噺 ==========
#define RGB_goal 225  // WS2812浜害鍊?

// 瓒呮椂璁℃暟鍣紙鐢ㄤ簬璁惧鏁呴殰妫€娴嬶紝50Hz涓?0娆?200ms锛?
uint8_t timeout_pitch = 0;      // Pitch鐢垫満瓒呮椂璁℃暟
uint8_t timeout_yaw = 0;        // Yaw鐢垫満瓒呮椂璁℃暟
uint8_t timeout_bopan = 0;      // 鎷ㄧ洏鐢垫満瓒呮椂璁℃暟
uint8_t timeout_mcl_up = 0;     // 涓婃懇鎿﹁疆瓒呮椂璁℃暟
uint8_t timeout_mcl_r = 0;      // 鍙虫懇鎿﹁疆瓒呮椂璁℃暟
uint8_t timeout_mcl_l = 0;      // 宸︽懇鎿﹁疆瓒呮椂璁℃暟
uint8_t timeout_mcl_rr = 0;     // 鍙冲悗鎽╂摝杞秴鏃惰鏁?
uint8_t timeout_mcl_ll = 0;     // 宸﹀悗鎽╂摝杞秴鏃惰鏁?
uint8_t timeout_mcl_upup = 0;   // 涓婁笂鎽╂摝杞秴鏃惰鏁?
uint8_t timeout_imu = 0;        // 浜戝彴IMU瓒呮椂璁℃暟
uint8_t timeout_mini_pitch = 0; // Mini Pitch 2006鐢垫満瓒呮椂璁℃暟

// 搴曠洏璁惧鐘舵€佹爣蹇椾綅锛堜粠搴曠洏鏉块€氳繃CAN鎺ユ敹锛?
uint8_t Chassis_Motor_M1_OK = 0;   // 搴曠洏鐢垫満M1鐘舵€侊紙0x201锛?
uint8_t Chassis_Motor_M2_OK = 0;   // 搴曠洏鐢垫満M2鐘舵€侊紙0x202锛?
uint8_t Chassis_Motor_M3_OK = 0;   // 搴曠洏鐢垫満M3鐘舵€侊紙0x203锛?
uint8_t Chassis_Motor_M4_OK = 0;   // 搴曠洏鐢垫満M4鐘舵€侊紙0x204锛?
uint8_t Chassis_IMU_OK = 0;        // 搴曠洏IMU鐘舵€?
uint8_t Chassis_REF_OK = 0;        // 瑁佸垽绯荤粺杩炴帴鐘舵€?

// 瓒呯數鐢靛帇鐩稿叧
float V_Cap_Real = 0.0f;           // 瓒呯數鐢靛帇瀹為檯鍊?
uint8_t V_Cap_OK_flag = 0;         // 瓒呯數鐢靛帇妫€鏌ユ爣蹇楋紙0=鏁呴殰锛岄潪0=姝ｅ父锛?

// RGB Debug API 閰嶇疆鍙橀噺
RGB_Debug_Config_t rgb_debug_config;
LED_Mapping_t led_mappings[5];

// 搴曠洏璁惧鐘舵€佹爣蹇椾綅瀹氫箟锛堟墿灞旸P_Tx_static_Flag锛岀敤浜嶤AN閫氫俊锛?
#define CHASSIS_M1_OK_FLAG    ((uint16_t)0x0001 << 5)   // 搴曠洏鐢垫満0x201
#define CHASSIS_M2_OK_FLAG    ((uint16_t)0x0001 << 6)   // 搴曠洏鐢垫満0x202
#define CHASSIS_M3_OK_FLAG    ((uint16_t)0x0001 << 7)   // 搴曠洏鐢垫満0x203
#define CHASSIS_M4_OK_FLAG    ((uint16_t)0x0001 << 8)   // 搴曠洏鐢垫満0x204
#define CHASSIS_IMU_OK_FLAG   ((uint16_t)0x0001 << 9)   // 搴曠洏IMU
#define CHASSIS_CAP_OK_FLAG   ((uint16_t)0x0001 << 10)  // 瓒呯骇鐢靛
#define CHASSIS_REF_OK_FLAG   ((uint16_t)0x0001 << 11)  // 瑁佸垽绯荤粺

/********  閫氫俊鏍囧織锟?   **********/
uint16_t YT_Tx_static_Flag = 0, // 浜戝彴鍙戯拷?锟介潤鎬佹爣蹇椾綅bool
    DP_Tx_static_Flag = 0;      // 搴曠洏鍙戯拷?锟介潤鎬佹爣蹇椾綅bool 0鍏抽棴 1锟?鍚簳鐩樿窡锟?
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

//锟?铻轰华鐘讹拷??
uint8_t Pitch_088_state = BMI088_ERROR;

// 鐢ㄤ簬鍒ゆ柇鐢垫満鏄惁閮芥帴鏀跺埌鏁版嵁锛岀敤浜庢帶鍒跺彂閫侀锟?
uint8_t MCL_4_motorflag = 0, MCL_2_motorflag = 0;
uint8_t can2_bjtx010 = 0, can2_bjtx011 = 0, can2bjtx013 = 0, can2bjtx014 = 0;
uint8_t BP_protect_cansend_flag = 0, MCL_protect_cansend_200_flag = 0, MCL_protect_cansend_1FF_flag = 0;

// 鎷ㄧ洏鐩稿叧
union
{
  float f_speed;
  uint16_t u16[2];
  uint8_t c[4];
} Shoot_speed_u;
float shoot_sp = 11.9;
int16_t L_targe_sp, R_targe_sp, UP_targe_sp, RR_targe_sp, LL_targe_sp, UPUP_targe_sp;
uint8_t MCL_Start_flag = 0;
int16_t MCL_MID;                                                                       // 鎽╂摝杞腑锟?
int16_t MCL_MAX_Speed_Near = 5250, MCL_MAX_Speed_Far = 5200, MCL_MAX_Speed_Now = 3700; // 5170鏀逛负3750,3700锛?5200
uint8_t MCL_ON_flag = 0;

// 鎷ㄧ洏
#define int_shot 26219
uint8_t BP_MODE = PROTECT_MODE;
int32_t BP_targe = 0, BP_calc_targe = 0;
uint8_t BP_ON_flag = 0;
bool Shoot_flag = 0; // 灏勫嚮鏍囧織锛岀敤浜庢帶鍒舵嫧鐩樿浆鍔ㄤ负0
uint16_t Shoot_time = 0;
float BP_Islow_incbuf = 0;
int32_t targe_inc;
UpDown_check_class UD_BoPan_ON(0), UD_BP_1(0);

// Mini_Pitch_2006鎺у埗鍙橀噺
uint8_t Mini_Pitch_MODE = PROTECT_MODE;
int32_t Mini_Pitch_targe = 0;              // 褰撳墠鐩爣浣嶇疆
int32_t Mini_Pitch_calc_targe = 0;         // 璁＄畻鐩爣浣嶇疆锛堢敤浜庡钩婊戣繃娓★級
uint8_t Mini_Pitch_preset_flag = 0;        // 棰勭疆瑙﹀彂鏍囧織
float Mini_Pitch_pwm_to_mang_ratio = 1.0f; // PWM鍒拌搴︾殑鏄犲皠姣斾緥锛堝彲璋冿級
float Mini_Pitch_manual_speed = 5.0f;     // 鎵嬪姩鎺у埗閫熷害锛堢敤浜庤皟鍙傛ā寮忥級
float Mini_Pitch_Islow_incbuf = 0;         // 骞虫粦杩囨浮缂撳啿鍙橀噺
uint8_t Mini_Pitch_first_enter_diaoshe = 0; // 棣栨杩涘叆鍚婂皠妯″紡鏍囧織
uint16_t Mini_Pitch_duzhuan_cnt = 0;
uint8_t Mini_Pitch_E_key_flag = 0;
// uint8_t Mini_Pitch_tune_enable = 0;         // 璋冨弬浣胯兘锛?=鐢垫満涓嶈緭鍑虹數娴侊紝闇€涓婁綅鏈哄懡浠ゅ紑鍚?

// huart1璋冨弬鍛戒护鎺ユ敹锛圖MA + IDLE + 涔掍箵缂撳啿锛屼豢VD_2rx锛?
#define TUNE_RX_NUM 64
uint8_t tune_rx_buf[2][TUNE_RX_NUM] = {0};
uint8_t tune_FIFO = 0;
uint16_t tune_rx_byte = 0;

// YAW
#define REAL_YAW_REF yaw_cont.continuous_yaw
// #define REAL_YAW_REF Pitch_088.realAngle.yaw
const int YT_Erro = 44480;
#define Yaw_Mouse_Speed 700.0f // Yaw榧犳爣閫熷害
#define Yaw_YK_GYRO_Speed 6000.0f
#define Yaw_YK_MANG_Speed 28000.0f
#define YAW_LEVEL_MANG 28660
#define YAW_LEVEL_MANG_2 28600 + 32768
// 鎷ㄧ洏灏勫嚮鍓嶉
#define YAW_SHOOT_FF -400.0f

float YAW_PID_OUT = 0;                   // PID杈撳嚭锛岀敤浜庡彂閫丆AN鏁版嵁
float Yaw_goal = 0;
uint8_t yaw_control_mode = PROTECT_MODE; // 鎺у埗妯″紡锛屽崟浣嶅害
float Erro_Yaw = 0;                      // 璇樊锛屾帶鍒舵ā锟?6锛岀敤浜庤搴﹁锟?
float Buf_Yaw = 0;
float vision_yaw = -2.9;
float yaw_angle_mang;
int32_t diff_2, diff_1;
float Yaw_ab;
float diaoshe_yaw_offset = 0;
float diff_diaoshe;
float YAW_0 = 0.0f;//璁板綍yaw0锟?
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
#define MINI_PITCH_BASE_POSITION 32468  // Mini_Pitch鍩哄噯浣嶇疆锛氬搴旇埖鏈虹殑1800
#define DIAO_SHE_ENCODER_OFFSET  36700   // 鍚婂皠妯″紡鐩爣缂栫爜鍣ㄥ€硷紙鐩稿鍫佃浆闆剁偣锛屽彲璋冿級
#define DUZHUAN_DIANLIU      (-3000)
#define DUZHUAN_SUDU_YUZHI   30
#define DUZHUAN_QUEREN_CISHU 400
#define MINI_PITCH_TUNE_MIN 0      // 璋冨弬琛岀▼涓嬮檺锛堝牭杞浂鐐癸級
#define MINI_PITCH_TUNE_MAX 40000  // 璋冨弬琛岀▼涓婇檺锛堝悐灏勭洰鏍?6700锛岀暀瑁曢噺锛?

float PITCH_PID_OUT = 0;                   // PID杈撳嚭锛岀敤浜庡彂閫丆AN鏁版嵁
float Pitch_goal = 0;                      // 鐩爣瑙掑害锛屽崟浣嶅害
uint8_t pitch_control_mode = PROTECT_MODE; // 鎺у埗妯″紡锛屽崟浣嶅害
float pitch_mang_goal = 50000;             // 淇话瑙掑害妯″紡鐩爣
float kk_pitch_mang_lp = 1;
float PitchGyro_AngleError = 0.0f;
float pitch_angle;
float pitch_angle_mang;
float vision_pitch = 33.7;

float vision_distance = 0;
uint8_t Pitch_arr_flag = 0;
float Pitch_X;
// 鍧愭爣鍙樻崲
float theta_angle = 0, theta_rad = 0;
float sin_theta = 0, cos_theta = 0;

// 灞曞紑妯″紡鏍囧織锟?
uint8_t deploy_flag = 0; // 0=鍏抽棴 1=灞曞紑妯″紡 2=灞曞紑鏃嬭浆
int32_t mini_pitch_deploy_offset = 0;

// 灏忛檧铻烘爣锟?
uint8_t SP_Turn_Flag = 0;

// 鎽╂摝杞埌鍦伴潰璺濈璁＄畻
float Pitch_MCL_length = 133.0f;
float Pitch_Ground_length = 397.0f;
float current_mcl_height = 0;

// 鑷瀯
uint8_t Anti_Time_Flag = 2;
int16_t Anti_Time_1 = 0, Anti_Time_2 = 0;
int16_t ZM_Fire_delay = 834;
float Yaw_ZM,Pitch_ZM;
float receive_freq = 0;  // 鎺ユ敹棰戠巼
uint8_t mouse_press_type = 0;          // 0=锟? 1=鍗曞嚮闀挎寜 2=鍙屽嚮
uint16_t mouse_press_counter = 0;
uint8_t aim_mode = 0x00;
uint8_t is_mouse_single_clicked = 0;

// 閲嶅姏琛ュ伩鐩稿叧
#define PITCH_GRAVITY_COMP_K 70.0f // 70
#define PITCH_GRAVITY_ANGLE_OFFSET 10.0f
#define PITCH_ENCODER_MID 32768 // 缂栫爜鍣ㄤ腑锟?
float gravity_comp;
float gravity_compensation = 0.0f;
float gravity_comp_filtered;

// 鏈涜繙锟?
uint8_t telescope_ON = 0;
// 鑸垫満纭鍔ㄤ綔鐩稿叧鍙橀噺
uint8_t servo_confirm_flag = 0;
uint16_t servo_confirm_timer = 0;
uint16_t servo_original_ccr = 0;
//鏈哄櫒浜轰綅缃俊锟?
union
{
  float f_pos;
  uint16_t u16[2];
  uint8_t c[4];
} Robot_pos_u;
#define GYRO_COMBO_DEADZONE 1.5f   // 锟?铻轰华姝诲尯
#define GYRO_COMBO_LPF_ALPHA 0.4f  // 浣庯拷?锟芥护锟?

float gyro_combo_lpf_last = 0.0f;
float gyro_filtered;

// 鑷瀯鐩稿叧瀹氫箟
uint16_t press_cnt = 0;
uint8_t PRESS_TYPE = 0;
#define NO_ZM_MODE 0
#define OWN_FIRE_MODE 1
#define ZM_FIRE_MODE 2

// 通用缓变函数 I16_slow / I_slow / I_slow_ease 已下沉到公版库 RM_Lib(RM_Lib.cpp/.h),声明见 RM_Lib.h;调用点不变。
// 妯″紡搴旂敤锟?
void MYMODE_while_application_layer(void)
{
  if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_MID) // 鍗曚簯鍙帮紝搴曠洏璺熼殢(涓嶅姩)
  {
    MYmode = DAN_YUN_TAI_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_DOWN) // 涓婁笅妯″紡
  {
    MYmode = SHANG_XIA_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_UP) // 搴曠洏浣庯拷?锟藉簳鐩樿窡锟?
  {
    MYmode = DI_PAN_L_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_MID) // 鍙岄噸锛屽簳鐩樿窡闅忥紝锟?铻轰华妯″紡
  {
    MYmode = SHUANG_ZHONG_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_MID && YK.yaogan.s2 == YK_SW_DOWN) // 鍚婂皠浣庯拷?锟藉簳鐩樿窡闅忥紝pitch
  {
    MYmode = DIAO_SHE_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_UP) // 搴曠洏楂橈拷?锟藉簳鐩樿窡锟?
  {
    MYmode = DI_PAN_H_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_MID) // 灏忛檧锟?
  {
    MYmode = XIAO_TUO_LUO_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_DOWN && YK.yaogan.s2 == YK_SW_DOWN) // 鍙岄噸鎴樻枟
  {
    MYmode = ZHAN_DOU_MODE;
  }
  else if (YK.yaogan.s1 == YK_SW_UP && YK.yaogan.s2 == YK_SW_UP) // 鍙屼笂淇濇姢
  {
    MYmode = PROTECT_MODE;
  }
  else // 鍏朵粬鎯呭喌淇濇姢妯″紡
  {
    MYmode = PROTECT_MODE;
  }
}
// 鎽╂摝杞拷?锟借緫锛岃繑鍥炴懇鎿﹁疆锟?鍏崇姸锟?
uint8_t MCL_Logic()
{
  static int16_t MCL_slow_speed = 0;
  static int16_t Buf_MCL_Speed = 0;
  static float MCL_I16slow_incbuf = 0;
  static UpDown_check_class UD_MCL(0), UD_C(0), UD_X(0), UD_TURN(0);
  static uint8_t now_state;
  //鏍规嵁妯″紡璁剧疆鎽╂摝杞拷?锟藉害
  if (MYmode == DIAO_SHE_MODE || deploy_flag)
  {
    MCL_MAX_Speed_Now = MCL_MAX_Speed_Far;  // 5050
  }
  else if (MYmode != PROTECT_MODE) 
  {
    MCL_MAX_Speed_Now = MCL_MAX_Speed_Near;  // 3685
  }
  //淇濇姢妯″紡鍒囨崲鎽╂摝杞拷?锟藉害
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
  // 閿洏鎺у埗鎽╂摝杞紑锟?
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    if (UD_MCL.updata(YK.Pressed_Check(KEY_PRESSED_B)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      MCL_ON_flag = !MCL_ON_flag;
    if (UD_C.updata(YK.Pressed_Check(KEY_PRESSED_C)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed += 50;
    else if (UD_X.updata(YK.Pressed_Check(KEY_PRESSED_X)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
      Buf_MCL_Speed -= 50;
  }
  // 鎽╂摝杞拷?锟藉害闄愬埗鍦ㄥ畨鍏ㄨ寖鍥村唴
  if (MYmode == PROTECT_MODE)
  {
    MCL_protect_cansend_200_flag = 1;
    MCL_protect_cansend_1FF_flag = 1;
    MCL_4_motorflag = 0; // 娓呴櫎鐢垫満鎺ユ敹鏍囧織锟?
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
    MCL_slow_speed = 0;  // 淇濇姢妯″紡娓呴浂缂撳彉閫熷害
    MCL_I16slow_incbuf = 0;  // 娓呴浂缂撳彉澧為噺缂撳啿锟?
  }
  else
  {
    MCL_Start_flag = 1;
  }
  // 璁＄畻鐩爣閫熷害
  int16_t MCL_targe_sp = MCL_ON_flag ? (MCL_MAX_Speed_Now + Buf_MCL_Speed) : 0;
  I16_slow(&MCL_slow_speed, MCL_targe_sp, 200, 200, 300, &MCL_I16slow_incbuf);

  // 璁剧疆鍚勭數鏈虹洰鏍囷拷?锟藉害
  if (MCL_Start_flag)
  {
    L_targe_sp = MCL_slow_speed - 14;
    R_targe_sp = MCL_slow_speed - 6;
    UP_targe_sp = MCL_slow_speed - 20;
    RR_targe_sp = MCL_slow_speed - 15;
    LL_targe_sp = MCL_slow_speed - 7;
    UPUP_targe_sp = MCL_slow_speed - 14;
  }
  else // 淇濇姢妯″紡鏃剁洰鏍囷拷?锟藉害=缂撳彉閫熷害
  {
      MCL_slow_speed = 0;
  }

  return MCL_ON_flag;
}
// 鎷ㄧ洏閫昏緫
void BP_Logic(uint8_t MCL_ON_flag)
{
  static int32_t BP_adjest_mang = 0;
  // 妯″紡鍒囨崲
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
  // 鍚婂皠妯″紡鎷ㄧ洏褰掗浂
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
  // 榧犳爣灏勫嚮
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DAN_YUN_TAI_MODE)
  {
    UpDown_check_state UD_BoPan_ON_buf;
    UD_BoPan_ON_buf = UD_BoPan_ON.updata(YK.shubiao.press_l);
    if (UD_BoPan_ON_buf == UpDown_check_rising && MCL_ON_flag && mouse_press_type != 2) // 榧犳爣宸﹂敭鎸変笅涓旀懇鎿﹁疆锟?锟?
    {
      Shoot_flag = 1;
      Shoot_time = 0; // 灏勫嚮璁℃椂鍣ㄦ竻锟?
    }
    else if (UD_BoPan_ON_buf == UpDown_check_falling) // 鏉惧紑
    {
      Shoot_time = 0; // 娓呴浂
    }
  }
  // 閬ユ帶鍣ㄦ嫧鐩樻帶锟?
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
// 鎷ㄧ洏鍗″脊锟?娴嬪拰锟?寮瑰锟? 1khz璋冪敤
void BP_KD_TIM(void)
{
  static uint16_t kadan_tui_mang = 6000;
  static int16_t Kadan_cnt = 0; // 鍗″脊璁℃暟锟?
  static uint16_t kadan_tim = 0;

  if (BP_MODE == MANG_MODE)
  {
    /*锟?娴嬪崱寮瑰苟锟?锟?*/
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


// Mini_Pitch_2006鎺у埗閫昏緫
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
    // 缂撳嚭锛氳繙绂荤洰鏍囧揩閫熷寑閫熼€艰繎锛屾帴杩?DIAO_SHE_ENCODER_OFFSET 鏃跺钩婊戝噺閫熷埌浣?
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
    // 棰勭疆瑙﹀彂锛氫繚鎶ゆā寮?+ 鍙虫憞鏉嗗彸涓?
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

// 鏉块棿閫氫俊澶勭悊 1khz璋冪敤
void Communication_boards(void)
{
  /*CAN2鍙屾澘閫氫俊锛屽簳鐩樻澘閫氳繃can鎺ユ敹閬ユ帶鍣ㄦ暟锟?,CAN1CAN2鍙戯拷?锟芥懇鎿﹁疆*/
  if (can2_bjtx010) // 鍙戯拷?锟介仴鎺у櫒鎽囨潌鏁版嵁x
  {
    CAN_2.Send_RM(0x010, YK.yaogan.ch0, YK.yaogan.ch1, YK.yaogan.ch2, YK.yaogan.ch3);
    can2_bjtx010 = 0;
  }
  else if (can2_bjtx011) // 鍙戯拷?锟介紶鏍噛z鍜岄敭鐩樻暟锟?
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
    can2bjtx014 = 0; // 鍙戯拷?锟藉畬鎴愭爣蹇椾綅
  }
  else if (BP_protect_cansend_flag)
  {
    CAN_2.Send_RM(0x200, 0, 0, 0, 0); // 淇濇姢pitch鐢垫満
    BP_protect_cansend_flag = 0;
  }
  /* CAN1鎽╂摝杞彂锟?*/
  if (MCL_protect_cansend_200_flag)
  {
    CAN_1.Send_RM(0x200, 0, 0, 0, 0); // 淇濇姢6涓懇鎿﹁疆
    MCL_protect_cansend_200_flag = 0;
  }
  else if (MCL_protect_cansend_1FF_flag)
  {
    CAN_1.Send_RM(0x1FF, 0, 0, 0, 0); // 淇濇姢6涓懇鎿﹁疆
    MCL_protect_cansend_1FF_flag = 0;
  }
}
// 瀹氭椂锟?12鍥炶皟
void TIM12_Callback(void)
{
 Pitch_088.analyse();
 timeout_imu = 0;  // BMI088鏁版嵁鏇存柊锛屾竻闆惰秴鏃惰鏁?
 if (abs(Pitch_088.realAngle.pitch) < 2.0f)
  {
   Pitch_088.realAngle.pitch = 0;
  }
}
// Mini PITCH鑸垫満鑾峰彇PWM锟?
uint16_t Mini_PITCH_Get_PWM()
{
  static uint8_t mini_pitch_ctrl_flag = 0;
  static uint8_t mini_pitch_anjian_flag = 0;
  static float mini_pitch_buf_ccr = 0;
  static uint16_t mini_pitch_ccr = 2700;
  static const uint16_t mini_pitch_downmang = 1800;

  // 鎺у埗妯″紡澶勭悊
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
  // PWM鍊艰锟?
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
// 鏈涜繙闀滆埖鏈鸿幏鍙朠WM锟?
uint16_t Telescope_Get_PWM()
{
  static uint16_t telescope_ccr = 0;
  static const uint16_t bj_en_ccr = 4900;
  static UpDown_check_class UD_E(0);

  // 鏍囧織澶勭悊
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

  // // E閿帶鍒舵湜杩滈暅
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

    if (request.zimiao_status)  // 鑷瀯锟?锟?
    {
      PID_Yaw_mang.Integral = 0;
      PID_Yaw_mang.OUT_I = 0;
      PID_Yaw_sp.Integral = 0;
      PID_Yaw_sp.OUT_I = 0;

      PID_Yaw_mang_zm.PID_update_LP(Yaw_goal, yaw_cont.continuous_yaw, kk_yaw_mang_lp);
      PID_Yaw_sp_zm.PID_update_LP(PID_Yaw_mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[2], kk_yaw_sp_lp);
      YAW_PID_OUT = -PID_Yaw_sp_zm.OUT_PID;
    }
    else  // 鑷瀯鍏抽棴
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
        // 璁＄畻褰撳墠杩炵画yaw瑙掑害锟?-180~+180鑼冨洿
        float current_angle = fmodf(yaw_cont.continuous_yaw, 360.0f);
        if (current_angle > 180.0f) current_angle -= 360.0f;
        if (current_angle < -180.0f) current_angle += 360.0f;

        // 鐩爣瑙掑害
        float target_angle = YAW_0 - vision_yaw;
        if (target_angle > 180.0f) target_angle -= 360.0f;
        if (target_angle < -180.0f) target_angle += 360.0f;

        // 璁＄畻锟?鐭棆杞亸绉婚噺
        float offset = target_angle - current_angle;
        if (offset > 180.0f) offset -= 360.0f;
        if (offset < -180.0f) offset += 360.0f;

        // 鍗曞嚮鍋忕Щ閲忓簲鐢ㄥ埌杩炵画yaw瑙掑害
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
  else//28215锟?61050 
  {
    if(yaw_0_flag && (Motor_LK6010_Yaw.mang_inf % 28215 == 0 || Motor_LK6010_Yaw.mang_inf % 61050 == 0))
    {
      YAW_0 = hipnuc_raw.hi91.yaw;
      yaw_0_flag = 0;
      // 瑙﹀彂鑸垫満纭鍔ㄤ綔
      servo_confirm_flag = 1;
      servo_confirm_timer = 0;
      servo_original_ccr = Telescope_Get_PWM(); // 淇濆瓨褰撳墠鑸垫満锟?
    }
    Yaw_goal = yaw_cont.continuous_yaw;
    YAW_PID_OUT = 0;
  }
}
// YAW瑙掑害鏇存柊锛屾帴鏀禖AN鏁版嵁鍚庤皟锟?
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
// 閲嶅姏琛ュ伩璁＄畻锛屾牴鎹畃itch瑙掑害璁＄畻
float PITCH_Gravity_Compensation(float pitch_angle)
{
  float pitch_rad = (pitch_angle + PITCH_GRAVITY_ANGLE_OFFSET) * 0.0174533f; // 瑙掑害杞姬锟?
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

    if (request.zimiao_status)  // 鑷瀯锟?锟?
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
    else  // 鑷瀯鍏抽棴
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
// PITCH瑙掑害鏇存柊锛屾帴鏀禖AN鏁版嵁鍚庤皟锟?
void PITCH_Angle_Update(void) { Motor_LK6010_Pitch.update_65535mang_inf_basic_zeromang(); }
// 鎺夊ご澶勭悊鍑芥暟
void YAW_Turn_Handle(void)
{
  static UpDown_check_class UD_DiaoTou_No(0);
  if (UD_DiaoTou_No.updata(turn_flag) == UpDown_check_rising && MYmode == ZHAN_DOU_MODE && !deploy_flag)
  {
    Yaw_goal -= 180.0f;
  }
}
// 鑸垫満PWM杈撳嚭
void Servo_PWM_Output()
{
  uint16_t telescope_ccr = Telescope_Get_PWM();
  // uint16_t mini_pitch_ccr = Mini_PITCH_Get_PWM();  // 宸茬鐢細鐜板湪浣跨敤2006鐢垫満

  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, telescope_ccr);
  // __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, mini_pitch_ccr);  // 宸茬鐢?
}
// 閿洏鐗规畩鍔熻兘澶勭悊
void Keyboard_Special_Func()
{
  static UpDown_check_class UD_XTL(0), UD_R(0), UD_Deploy_Turn(0), UD_Arr_PY(0), UD_Boost(0), UD_MCL_Speed_Turn(0),UD_Z(0),UD_X(0),UD_C(0);
  static int targe_f = YT_Erro;
  static uint8_t mini_pitch_anjian_flag = 0;

  /*閿洏鎺у埗浣胯兘鏍囧織*/ // 鏍规嵁妯″紡鍐冲畾鏄惁鍚敤閿洏鎺у埗鍔熻兘(闃叉璇Е瀵艰嚧鎰忓鎿嶄綔)
  if (MYmode == ZHAN_DOU_MODE || MYmode == SHUANG_ZHONG_MODE || MYmode == DIAO_SHE_MODE || MYmode == DAN_YUN_TAI_MODE || MYmode == SHANG_XIA_MODE)
  {
    jianshu_ctrl_flag = ENABLE_MODE;
  }
  else
  {
    jianshu_ctrl_flag = DISABLE_MODE;
  }
  /**鎸夐敭鍔熻兘 */
  if (jianshu_ctrl_flag == ENABLE_MODE)
  {
    // Q锟? - 灏忛檧锟?
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
    // G锟? - boost
    if (UD_Boost.updata(boost_flag) == UpDown_check_rising)
    {
      mini_pitch_anjian_flag = 0;
      Pitch_goal = 0;
      telescope_ON = 0;
    }
    // R锟? - 灞曞紑妯″紡鍒囨崲
    if (UD_R.updata(YK.Pressed_Check(KEY_PRESSED_R)) == UpDown_check_rising && !YK.Pressed_Check(KEY_PRESSED_CTRL))
    {
      deploy_flag = !deploy_flag;
      mini_pitch_anjian_flag = 0;
      telescope_ON = deploy_flag ? 1 : 0;
      // Pitch_goal = -36.5f;
    }
    // F锟? - 灞曞紑妯″紡鏃嬭浆180锟?
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
    // 锟?娴嬫棆杞埌浣嶏紝鎭㈠灞曞紑鏍囧織
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
// Mini pitch榧犳爣璋冩暣锟?160Hz璋冪敤
void Mini_PITCH_Mouse_Adjust(void)
{
  static float mini_pitch_buf_ccr = 0;
  static uint8_t mini_pitch_ctrl_flag = 0;

  if (mini_pitch_ctrl_flag == 1)
  {
    mini_pitch_buf_ccr -= YK.shubiao.y / 40.0f;
  }
}
// 灞曞紑妯″紡瓒呮椂锟?鏌ワ紝160Hz璋冪敤
void Deploy_Timeout_Check(void)
{
  static int16_t deploy_out_tim = 0;
  if (deploy_flag == 2)
  {
    deploy_out_tim++;
    if (deploy_out_tim > 160)
    {
      deploy_flag = 0; // 淇濇姢锛岃秴鏃舵仮澶嶆爣锟?
      deploy_out_tim = 0;
    }
  }
  else
  {
    deploy_out_tim = 0;
  }
}
// 澶嶄綅鍑芥暟
void Reset(void)
{
  if (YK.Pressed_Check(KEY_PRESSED_Z) && YK.Pressed_Check(KEY_PRESSED_CTRL)) // 鎸変綇z鍜宑trl閿繘琛岃蒋浠跺锟?
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
    __set_FAULTMASK(1); // 鍏抽棴锟?鏈変腑锟?
    NVIC_SystemReset(); // 澶嶄綅
  }
}
//鑷瀯锟?锟?
void ZM_Check(void)
{
  static UpDown_check_class UD_ZM_Fire(0), UD_FIRE(0);
  static uint16_t mouse_release_counter = 0;

  #define MOUSE_SHORT_CLICK_TIME 300
  #define MOUSE_LONG_PRESS_TIME 700
  #define MOUSE_CLICK_GAP_TIME 700

  UpDown_check_state mouse_edge = UD_ZM_Fire.updata(YK.shubiao.press_r);

  // 榧犳爣鍙抽敭涓婂崌娌匡紝锟?娴嬪崟鍑绘垨鍙屽嚮
  if (mouse_edge == UpDown_check_rising)
  {
    mouse_press_counter = 0;

    if (is_mouse_single_clicked && mouse_release_counter < MOUSE_CLICK_GAP_TIME)
    {
      mouse_press_type = 2;  // 鍙屽嚮
    }
    else
    {
      mouse_press_type = 1;  // 鍗曞嚮闀挎寜
      is_mouse_single_clicked = 0;  // 娓呴櫎鍗曞嚮鏍囧織
    } 
  }   

  // 鏉惧紑榧犳爣涓婂崌娌匡紝锟?娴嬬煭锟?
  else if (mouse_edge == UpDown_check_falling)
  {
    mouse_release_counter = 0;

    // 鍒ゆ柇鏄惁涓虹煭鎸夊崟锟?
    if (mouse_press_counter < MOUSE_SHORT_CLICK_TIME)
    {
      is_mouse_single_clicked = 1;  // 鏍囪鍗曞嚮鐘讹拷??
    }
    
    mouse_press_type = 0;
  }
  //璁℃暟锟?
  if (YK.shubiao.press_r)
  {
    mouse_press_counter++;
    mouse_release_counter = 0;
  }
  else
  {
    mouse_release_counter++;
    mouse_press_counter = 0;

    // 瓒呮椂娓呴櫎鍗曞嚮鏍囪
    if (is_mouse_single_clicked && mouse_release_counter >= MOUSE_CLICK_GAP_TIME)
    {
      is_mouse_single_clicked = 0;
    }
  }

  if (MYmode == ZHAN_DOU_MODE)
  {
    if (YK.shubiao.press_r)
    {
      if (mouse_press_type == 1 && mouse_press_counter >= MOUSE_LONG_PRESS_TIME) // 闀挎寜鍚敤鑷瀯
      {
        request.zimiao_status = 1;
        zm_press1++;
      }
      else if (mouse_press_type == 2 && mouse_press_counter >= 100)  // 鍙屽嚮灏勫嚮
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
    if(SuperPower.mode == 0 && request.zimiao_status) // 娌℃湁璇嗗埆鍒?
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
      
      if (SuperPower.mode == 2 && !ZM_Fire_delay && YK.yaogan.ch0 < -500 && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL))) //鑷瀯鐏帶
      // if (SuperPower.mode == 2 && !ZM_Fire_delay)
      {
        Shoot_flag = 1;
        ZM_Fire_delay = 834;
      }
      // else if(SuperPower.mode == 1 && !ZM_Fire_delay && YK.yaogan.ch0 > 500)
      else if(!ZM_Fire_delay && YK.yaogan.ch0 > 500 && (heat_flag || YK.Pressed_Check(KEY_PRESSED_CTRL))) //鎵嬫墦
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
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, Laser_State); // 锟?鍏夊紑鍏虫帶锟?
}
// 鑷瀯淇濇姢妯″紡澶勭悊
void ZM_Protect_Mode_Handle(void)
{
  if (MYmode == PROTECT_MODE)
  {
    request.zimiao_status = 0;
    // 鑷瀯淇濇姢锛岄仴鎺у櫒鐗瑰畾鎿嶄綔鎺у埗PY锟?锟?
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 > 500)
    {
      Laser_deal(GPIO_PIN_SET);
    }
    if (YK.yaogan.ch0 < -500 && YK.yaogan.ch1 < -500)
    {
      Laser_deal(GPIO_PIN_RESET); // 鍏抽棴锟?锟?
    }
  }
}
// 鑷瀯鎺у埗
void ZM_Control(void)
{
  ZM_Check();
  ZM_Protect_Mode_Handle(); // 鑷瀯淇濇姢妯″紡
}
#define PI 3.1415926
// 涓插彛3涓柇鑷瀯(瑙嗚)
void UART3_IT_ZM(void)
{
  static int aa;
  uint32_t temp;
//  extern BMI088 Pitch_088;
  extern float Pitch_goal, Yaw_goal;
  if ((__HAL_UART_GET_FLAG(&MINI_PC_USART_HANDLE, UART_FLAG_IDLE) != RESET)) // 锟?娴嬪埌绌洪棽涓柇锛孶ART鎺ユ敹瀹屾垚锛屼覆鍙ｇ┖闂蹭腑鏂紝锟?娴嬪埌锟?甯ф暟鎹帴鏀跺畬锟?
  {

    __HAL_UART_CLEAR_IDLEFLAG(&MINI_PC_USART_HANDLE); // 娓呴櫎绌洪棽涓柇鏍囧織锟?
    temp = MINI_PC_USART_HANDLE.Instance->SR;
    temp = MINI_PC_USART_HANDLE.Instance->DR;
    HAL_UART_DMAStop(&MINI_PC_USART_HANDLE);                                     // 鍋滄DMA浼犺緭锛岄槻姝㈡暟鎹锟?
    // getReceiveData_ZM(Mini_PC_rx_buf);
    GetReceive_SP(Mini_PC_rx_buf);                                         // 澶勭悊鎺ユ敹鍒扮殑鏁版嵁锛屼粠Mini_PC_rx_buf涓彁鍙栨湁鏁堜俊锟?
    HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_rx_buf, 128); // 閲嶆柊鍚姩DMA鎺ユ敹锛屽噯澶囨帴鏀朵笅锟?甯ф暟鎹紝锟?锟?128瀛楄妭
    // if (isnan(response.yaw.f))
    //   response.yaw.f = 0.0; // 濡傛灉瑙嗚鏁版嵁涓篘aN锛屽垯灏嗗叾璁剧疆锟?0锛岄槻姝㈠紓甯革拷?锟藉奖鍝嶆帶锟?
    // if (isnan(response.pitch.f))
    //   response.pitch.f = 0.0; // 鍚屼笂
    /*************** Your code *****************/

    if (Mini_PC_rx_buf[0] == 0x66 && Mini_PC_rx_buf[28] == 0x11) // 锟?鏌ユ暟鎹抚澶村熬鏄惁锟?0x66锛岄獙璇佹暟鎹湁鏁堬拷??
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
        // 1. 璁＄畻褰撳墠杩炵画yaw瑙掑害锟?-180~+180鑼冨洿
        float current_mod = fmodf(yaw_cont.continuous_yaw, 360.0f);
        if (current_mod > 180.0f)  current_mod -= 360.0f;
        if (current_mod < -180.0f) current_mod += 360.0f;
        // 2. 璁＄畻瑙嗚鐩爣涓庡綋鍓嶈锟?-180~+180鑼冨洿鍐呯殑锟?鐭亸锟?
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

// ========== RGB Debug 鍒濆鍖栫浉鍏冲嚱鏁?==========

/**
 * @brief LED 4 鑷瀯鐘舵€佹洿鏂板嚱鏁帮紙鑷畾涔夊洖璋冿級
 * @note 鑷瀯鏈変笁绉嶇姸鎬侊細缁胯壊锛堝叧闂級銆佽摑鑹诧紙鑷瀯鐏帶锛夈€佺孩鑹诧紙鎿嶄綔鎵嬬伀鎺э級
 */
void RGB_LED4_Zimiao_Update(void)
{
    if (request.zimiao_status == 0)
    {
        RGB_UI.WS281x_SetPixelRGB(4, 0, RGB_goal, 0);   // 缁胯壊锛氳嚜鐬勫叧闂?
    }
    else if (PRESS_TYPE == ZM_FIRE_MODE)
    {
        RGB_UI.WS281x_SetPixelRGB(4, 0, 0, RGB_goal);   // 钃濊壊锛氳嚜鐬勭伀鎺фā寮?
    }
    else if (PRESS_TYPE == OWN_FIRE_MODE)
    {
        RGB_UI.WS281x_SetPixelRGB(4, RGB_goal, 0, 0);   // 绾㈣壊锛氭搷浣滄墜鐏帶妯″紡
    }
}

/**
 * @brief RGB Debug 鍒濆鍖栧嚱鏁?
 * @note 閰嶇疆5涓狶ED鐨勮澶囨槧灏勫拰鍔熻兘鐘舵€佹樉绀?
 */
void RGB_Debug_Setup(void)
{
    // ========== 閰嶇疆 LED 鏄犲皠琛?==========

    // LED 0: Pitch/Yaw鐢垫満銆佷簯鍙癐MU
    led_mappings[0].led_index = 0;
    led_mappings[0].device_count = 3;
    // Pitch鐢垫満锛圠K6010锛?
    led_mappings[0].devices[0].timeout_ptr = &timeout_pitch;
    led_mappings[0].devices[0].timeout_threshold = 10;  // 200ms瓒呮椂
    led_mappings[0].devices[0].status_flag_ptr = NULL;
    // Yaw鐢垫満锛圠K6010锛?
    led_mappings[0].devices[1].timeout_ptr = &timeout_yaw;
    led_mappings[0].devices[1].timeout_threshold = 10;
    led_mappings[0].devices[1].status_flag_ptr = NULL;
    // 浜戝彴IMU锛圔MI088锛?
    led_mappings[0].devices[2].timeout_ptr = &timeout_imu;
    led_mappings[0].devices[2].timeout_threshold = 10;
    led_mappings[0].devices[2].status_flag_ptr = NULL;
    // 鏁呴殰棰滆壊閰嶇疆
    led_mappings[0].color_modes[0] = RED_MODE;        // Pitch鏁呴殰锛氱孩闂?
    led_mappings[0].color_modes[1] = GREEN_MODE;      // Yaw鏁呴殰锛氱豢闂?
    led_mappings[0].color_modes[2] = RED_GREEN_MODE;  // IMU鏁呴殰锛氱孩缁夸氦鏇?

    // LED 1: 鎽╂摝杞紙6涓紝浣哃ED鏄犲皠鏈€澶?涓級+ 鎷ㄧ洏
    // 浼樺厛鐩戞帶锛氫笂杞€佸乏杞€佸彸杞€佹嫧鐩?
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

    // LED 2: 搴曠洏3508鐢垫満锛圡1-M4锛?
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

    // LED 3: 搴曠洏IMU銆佽秴鐢?
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

    // LED 4: 瑁佸垽绯荤粺
    led_mappings[4].led_index = 4;
    led_mappings[4].device_count = 1;
    led_mappings[4].devices[0].timeout_ptr = NULL;
    led_mappings[4].devices[0].timeout_threshold = 0;
    led_mappings[4].devices[0].status_flag_ptr = &Chassis_REF_OK;
    led_mappings[4].color_modes[0] = RED_MODE;

    // ========== 鍒濆鍖?RGB Debug ==========
    rgb_debug_config.rgb_ui = &RGB_UI;
    rgb_debug_config.led_mappings = led_mappings;
    rgb_debug_config.led_count = 5;
    rgb_debug_config.blink_period = 50;  // 50娆¤鏁?1绉扏50Hz
    RGB_Debug_Init(&rgb_debug_config);

    // ========== 娉ㄥ唽鍔熻兘鐘舵€佹樉绀猴紙鏃犻敊璇椂鏄剧ず锛?==========

    // LED 0: 灏忛檧铻虹姸鎬?
    Function_Status_t xtl_status;
    xtl_status.led_index = 0;
    xtl_status.flag_ptr = &XTL_flag;
    xtl_status.on_color[0] = RGB_goal;  // 寮€鍚細绾㈣壊
    xtl_status.on_color[1] = 0;
    xtl_status.on_color[2] = 0;
    xtl_status.off_color[0] = 0;        // 鍏抽棴锛氱豢鑹?
    xtl_status.off_color[1] = RGB_goal;
    xtl_status.off_color[2] = 0;
    RGB_Control_Register_Function(xtl_status);

    // LED 1: 鎽╂摝杞姸鎬?
    Function_Status_t mcl_status;
    mcl_status.led_index = 1;
    mcl_status.flag_ptr = &MCL_ON_flag;
    mcl_status.on_color[0] = RGB_goal;  // 寮€鍚細绾㈣壊
    mcl_status.on_color[1] = 0;
    mcl_status.on_color[2] = 0;
    mcl_status.off_color[0] = 0;        // 鍏抽棴锛氱豢鑹?
    mcl_status.off_color[1] = RGB_goal;
    mcl_status.off_color[2] = 0;
    RGB_Control_Register_Function(mcl_status);

    // LED 2: 鍚婂皠鐘舵€侊紙鏇挎崲26鑻遍泟鐨勪笂鍙伴樁锛?
    Function_Status_t diaoshe_status;
    diaoshe_status.led_index = 2;
    diaoshe_status.flag_ptr = &deploy_flag;  // 浣跨敤deploy_flag
    diaoshe_status.on_color[0] = RGB_goal;   // 寮€鍚細绾㈣壊
    diaoshe_status.on_color[1] = 0;
    diaoshe_status.on_color[2] = 0;
    diaoshe_status.off_color[0] = 0;         // 鍏抽棴锛氱豢鑹?
    diaoshe_status.off_color[1] = RGB_goal;
    diaoshe_status.off_color[2] = 0;
    RGB_Control_Register_Function(diaoshe_status);

    // LED 3: 鐖嗗彂鐘舵€?
    Function_Status_t boost_status;
    boost_status.led_index = 3;
    boost_status.flag_ptr = &boost_flag;
    boost_status.on_color[0] = RGB_goal;  // 寮€鍚細绾㈣壊
    boost_status.on_color[1] = 0;
    boost_status.on_color[2] = 0;
    boost_status.off_color[0] = 0;        // 鍏抽棴锛氱豢鑹?
    boost_status.off_color[1] = RGB_goal;
    boost_status.off_color[2] = 0;
    RGB_Control_Register_Function(boost_status);

    // LED 4: 鑷瀯鐘舵€侊紙鐗规畩澶勭悊锛屼娇鐢ㄨ嚜瀹氫箟鍥炶皟锛?
    RGB_Control_Register_LED4_Callback(RGB_LED4_Zimiao_Update);
}


/* ===== main.c USER CODE 4: ISR callbacks + business functions (verbatim) ===== */

/*******************************************************************************************************
 * can1 涓柇 6涓懇鎿﹁疆鐢垫満
 * ************************************************************************************************/

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // 8khz
{
     if (CAN_1.Receive(CAN_RX_FIFO0) == HAL_OK && MCL_Start_flag) //
  //    鎺ユ敹鍒癱an1鏁版嵁鍚庤繘琛屽锟?// 锟?鍚悗浼氬鑷存嫧鐩樹笉杞紝鍙兘鏄洜涓烘嫧鐩樼數鏈哄湪杩欓噷鏇存柊浜唒id
  // if (CAN_1.Receive(CAN_RX_FIFO0) == HAL_OK)
  {
    if (Motor_MCL_UP_up.update() == HAL_OK) // 1khz
    {
      timeout_mcl_upup = 0;  // 娓呴浂瓒呮椂璁℃暟
      MCL_4_motorflag |= 0x1;
      PID_MCL_UP_sp.PID_update(-UP_targe_sp, Motor_MCL_UP_up.sp);
    } //-

    else if (Motor_MCL_R.update() == HAL_OK) // 1khz
    {
      timeout_mcl_r = 0;  // 娓呴浂瓒呮椂璁℃暟
      MCL_4_motorflag |= 0x2;
      PID_MCL_R_sp.PID_update(-R_targe_sp, Motor_MCL_R.sp);
    } //-

    else if (Motor_MCL_L.update() == HAL_OK)
    {
      timeout_mcl_l = 0;  // 娓呴浂瓒呮椂璁℃暟
      MCL_4_motorflag |= 0x4;
      PID_MCL_L_sp.PID_update(L_targe_sp, Motor_MCL_L.sp);
    } //+

    else if (Motor_MCL_RR.update() == HAL_OK)
    {
      timeout_mcl_rr = 0;  // 娓呴浂瓒呮椂璁℃暟
      MCL_4_motorflag |= 0x8;
      PID_MCL_RR_sp.PID_update(-RR_targe_sp, Motor_MCL_RR.sp);
    } //-

    else if (Motor_MCL_UP.update() == HAL_OK)
    {
      timeout_mcl_up = 0;  // 娓呴浂瓒呮椂璁℃暟
      MCL_2_motorflag |= 0x1;
      PID_MCL_UPUP_sp.PID_update(-UPUP_targe_sp, Motor_MCL_UP.sp);
    } //-

    else if (Motor_MCL_LL.update() == HAL_OK)
    {
      timeout_mcl_ll = 0;  // 娓呴浂瓒呮椂璁℃暟
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
     MCL_MID = (Motor_MCL_L.sp - Motor_MCL_R.sp - Motor_MCL_UP_up.sp + Motor_MCL_LL.sp - Motor_MCL_UP.sp - Motor_MCL_RR.sp) / 6; // 鎽╂摝杞钩鍧囪浆閫燂紝鐢ㄤ簬鍒ゆ柇鎽╂摝锟?
   }
  }
}

/*******************************************************************************************************
 * can2  涓柇鎺ユ敹 yaw pitch  鎷ㄧ洏tx
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
      timeout_yaw = 0;  // 娓呴浂瓒呮椂璁℃暟
      YAW_Angle_Update();
    }
    else if (Motor_LK6010_Pitch.LK_Broadcast_update() == HAL_OK)
    {
      timeout_pitch = 0;  // 娓呴浂瓒呮椂璁℃暟
      PITCH_Angle_Update();
    }
    else if (BoPan.update() == HAL_OK)
    {
      timeout_bopan = 0;  // 娓呴浂瓒呮椂璁℃暟
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
      timeout_mini_pitch = 0;  // 娓呴浂瓒呮椂璁℃暟
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

    case (0x120): // 搴曠洏鐘舵€佸弽棣?
    {
      // 搴曠洏鐘舵€佹爣蹇椾綅鍦╮x_buf[0]鍜宺x_buf[1]
      uint16_t Chassis_Status_Flag_Received = CAN_2.rx_buf[0] << 8 | CAN_2.rx_buf[1];
      Chassis_Motor_M1_OK = (Chassis_Status_Flag_Received & CHASSIS_M1_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M2_OK = (Chassis_Status_Flag_Received & CHASSIS_M2_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M3_OK = (Chassis_Status_Flag_Received & CHASSIS_M3_OK_FLAG) ? 1 : 0;
      Chassis_Motor_M4_OK = (Chassis_Status_Flag_Received & CHASSIS_M4_OK_FLAG) ? 1 : 0;
      Chassis_IMU_OK = (Chassis_Status_Flag_Received & CHASSIS_IMU_OK_FLAG) ? 1 : 0;
      Chassis_REF_OK = (Chassis_Status_Flag_Received & CHASSIS_REF_OK_FLAG) ? 1 : 0;

      // 瓒呯數鐢靛帇鍦╮x_buf[2]鍜宺x_buf[3]锛坕nt16_t鏍煎紡锛?
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
      // YAW鍜孭ITCH鐢垫満PID璁＄畻
      YAW_PID_Calc();
      PITCH_PID_Calc();

      // 鍙戯拷?锟芥懇鎿﹁疆鏁版嵁
//      CAN_2.Broadcast_Send_LK(0, YAW_PID_OUT, 0, 0);
      CAN_2.Broadcast_Send_LK(PITCH_PID_OUT, YAW_PID_OUT, 0, 0);			
      can_scan = 0;
    }
    else
    {
      // 鑷瀯鎺у埗
       ZM_Control();
      // 鎷ㄧ洏閫昏緫澶勭悊锛屾牴鎹ā寮忚皟锟?
      // 鎷ㄧ洏瑙掑害妯″紡鏃讹紝鏍规嵁灞曞紑鐘讹拷?锟借皟鏁村閲忕紦鍙橈拷?锟藉害锛岄槻姝㈢獊鍙樺鑷村崱锟?
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
    // Mini pitch榧犳爣璋冩暣
    Mini_PITCH_Mouse_Adjust();

    // 灞曞紑妯″紡瓒呮椂锟?锟?
    Deploy_Timeout_Check();

    // 鑸垫満纭鍔ㄤ綔鐩稿叧
    if(servo_confirm_flag)
    {
        servo_confirm_timer++;

        if(servo_confirm_timer < 30)  // 锟?30涓懆鏈燂紝锟?0.19锟? - 鎶捣鑸垫満
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr + 900);
        }
        else if(servo_confirm_timer < 60)  // 30-60涓懆鏈燂紝锟?0.19锟? - 鎭㈠鍘熶綅
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, servo_original_ccr);
        }
        else  // 缁撴潫鍔ㄤ綔
        {
            servo_confirm_flag = 0;
            servo_confirm_timer = 0;
        }
    }

    // 閬ユ帶鍣ㄦ暟锟?(浜ゆ浛)
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


    // 鏇存柊鎵€鏈夎秴鏃惰鏁板櫒
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

    // 鏇存柊瓒呯數鐢靛帇鏍囧織
    V_Cap_OK_flag = (V_Cap_Real != 0);

    // 璋冪敤 RGB Debug API 鏇存柊
    RGB_Debug_Update();

    /**淇濇姢妯″紡 */
    if (BP_MODE == PROTECT_MODE)
    {
        BP_protect_cansend_flag = 1;
        BP_ON_flag = 0;
        BP_targe = BoPan.mang_inf;
        BP_calc_targe = BoPan.mang_inf;
    }
    // YAW淇濇姢妯″紡
    if (yaw_control_mode == PROTECT_MODE)
    {
      YAW_PID_OUT = 0;
      Yaw_goal = REAL_YAW_REF;
      Erro_Yaw = Motor_LK6010_Yaw.mang_inf;
      PID_YAW_Erro_IMU_MANG.Integral = 0;
      PID_YAW_Erro_IMU_MANG.OUT_I = 0;
    }

    // PITCH淇濇姢妯″紡
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

      request.Yaw_Angle.f = V_Yaw.Vision_Low_Pass_Filter(yaw_cont.continuous_yaw);
      request.Pitch_Angle.f = V_Pitch.Vision_Low_Pass_Filter(-hipnuc_raw.hi91.pitch);
      request.Yaw_Anglespeed.f = hipnuc_raw.hi91.gyr[1];
      //q4

      if (MYmode == ZHAN_DOU_MODE && YK.shubiao.press_r)
      {
          if (mouse_press_type == 1 && mouse_press_counter >= MOUSE_LONG_PRESS_TIME)
          {
              aim_mode = 0x01;  // 鍗曞嚮闀挎寜 = 鍚敤鑷瀯妯″紡
          }
          else if (mouse_press_type == 2 && mouse_press_counter >= 100)
          {
              aim_mode = 0x01;  // 鍙屽嚮 = 鍚敤鑷瀯妯″紡
          }
      }
      else if(MYmode == ZHAN_DOU_MODE && !YK.shubiao.press_r)
      {
        aim_mode = 0x00;
      }

      Mini_PC_SendData_ZM2(aim_mode);  // 鍙戯拷?锟借嚜鐬勬ā寮忔暟锟?
  }
}

/* ===== application entry ===== */
extern "C" void My_Setup(void)
{
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
	RGB_Debug_Setup();  // 鍒濆鍖?RGB Debug API
  HAL_TIM_Base_Start_IT(&htim1); // 鑸垫満瀹氭椂锟?
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim9); // 2khz涓绘帶锛宑an2鍙戯拷??
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim7);
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim8); // 40hz 0x13id鍙戯拷??
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim6);//10hz閬ユ帶锟?
  HAL_Delay(0);
  HAL_TIM_Base_Start_IT(&htim5);//800hz
  HAL_Delay(0);	
  HAL_TIM_Base_Start_IT(&htim12); /**********锟?铻轰华璇诲彇************/
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  Mini_PC_UART_Init();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); // 浣胯兘鐢垫満椹卞姩
//	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(1500);
  Mini_Pitch_MODE = DUZHUAN_MODE;
	IMU_UART_Init();
}

extern "C" void My_Loop(void)
{
/*閬ユ帶鍣ㄦ暟鎹?*/
//     INFO("%d,%d,%d,%d,%d,%d\r\n",YK.yaogan.ch0,YK.yaogan.ch1,YK.yaogan.ch2,YK.yaogan.ch3,YK.yaogan.s1,YK.yaogan.s2);
/*pitch鍐呭鐜洰鏍囧疄闄?*/
  //  INFO("%.2f,%.2f\r\n", Pitch_goal, hipnuc_raw.hi91.pitch);
      // INFO("%.2f,%.2f\r\n", PID_LK_Pitch_Mang.OUT_PID, hipnuc_raw.hi91.gyr[0]);
    // INFO("%.2f,%.2f\r\n", PID_LK_Pitch_Mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[1]);
    // INFO("%.2f,%.2f\r\n", PID_LK_Erro_Pitch_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[0]);  
/*pitch缂栫爜鍣?*/
//   	 INFO("%d\r\n",Motor_LK6010_Pitch.mang);
/*鎷ㄧ洏鐢垫満鍐呭鐜洰鏍囧疄闄?*/
    //  INFO("%d,%d\r\n",BP_targe, BoPan.mang_inf);    
//     INFO("%.2f,%d\r\n", PID_BP_mang.OUT_PID, BoPan.sp);
/*鎷ㄧ洏鐢垫満缂栫爜鍣?*/
//     INFO("%d\r\n", BoPan.mang_inf);   
/*bmi088铻轰华涓夎酱瑙掑害瑙掗€熷害*/
    // INFO("%.2f,%.2f,%.2f\r\n", Pitch_088.realAngle.yaw, Pitch_088.realAngle.roll,Pitch_088.realAngle.pitch);
//		 INFO("%.2f,%.2f,%.2f\r\n",Pitch_088.Anglespeed.Deal_yaw, Pitch_088.Anglespeed.Deal_roll,Pitch_088.realAngle.pitch);
//				 INFO("%.2f,%.2f,%.2f,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%d\r\n",response.pitch.f, response.yaw.f,response.distance.f,a,b,c,d,e,f,g,h,o,p,counter);
/*yaw鍐呭鐜洰鏍囧疄闄?*/    
   INFO("%.2f,%.2f\r\n", Yaw_goal, yaw_cont.continuous_yaw);
//     INFO("%.2f,%.2f\r\n",PID_Yaw_mang.OUT_PID, hipnuc_raw.hi91.gyr[2]);   
    // INFO("%.2f,%.2f\r\n",PID_YAW_Erro_IMU_MANG.OUT_PID, hipnuc_raw.hi91.gyr[2]);
    // INFO("%.2f,%.2f\r\n",PID_Yaw_mang_zm.OUT_PID, hipnuc_raw.hi91.gyr[2]);hipnuc_raw.hi91.pitch
/*yaw缂栫爜鍣?*/         
//		 INFO("%d\r\n",Motor_LK6010_Yaw.mang);
/*鎽╂摝杞數鏈哄弽棣堣浆閫?*/
//     INFO("%d,%d,%d,%d,%d,%d\r\n", Motor_MCL_UP_up.sp, Motor_MCL_R.sp, -Motor_MCL_L.sp, Motor_MCL_RR.sp, -Motor_MCL_LL.sp, Motor_MCL_UP.sp);
/*鎽╂摝杞崟涓數鏈哄弽棣堣浆閫?*/
    // INFO("%d\r\n", Motor_MCL_UP_up.sp);
/*灏勯€?*/
//      INFO("%.2f\r\n", shoot_sp);
/*鑷瀯*/
	  // INFO("%.2f,%.2f,%.2f,%.2f,%d,%d,%d\r\n", Yaw_ZM,yaw_cont.continuous_yaw,Pitch_ZM,hipnuc_raw.hi91.pitch,SuperPower.mode,response.Fire_Flag,is_mouse_single_clicked);
/*CH010闄€铻轰华涓夎酱瑙掑害*/    
//     INFO("%.2f,%.2f,%.2f,%.2f\r\n",  hipnuc_raw.hi91.roll, hipnuc_raw.hi91.yaw, hipnuc_raw.hi91.pitch,yaw_cont.continuous_yaw);
/*CH010闄€铻轰华鍥涘厓鏁?*/    
		// INFO("%.2f,%.2f,%.2f,%.2f\r\n", hipnuc_raw.hi91.quat[0],hipnuc_raw.hi91.quat[1],hipnuc_raw.hi91.quat[2],hipnuc_raw.hi91.quat[3]);
/*CH010闄€铻轰华瑙掗€熷害*/    
    // INFO("%.2f\r\n",hipnuc_raw.hi91.gyr[0]);
/*榧犳爣鏁版嵁*/
    // INFO("%d,%d\r\n", YK.shubiao.x, YK.shubiao.y);
/*鐑噺鏍囧織*/
    // INFO("%d\r\n", heat_flag);
/*鏋彛浣嶇疆淇℃伅*/
	  // INFO("%.2f\r\n", Robot_pos_u.f_pos);锛?
/*2006灏弍itch鍐呭鐜洰鏍囧疄闄?*/
  //  INFO("%d,%d\r\n",Mini_Pitch_targe, Mini_Pitch_2006.mang_inf);
		//  INFO("%.2f,%d\r\n",PID_Mini_Pitch_2006_mang.OUT_PID, Mini_Pitch_2006.sp);
/*绾㈣摑鏂?*/
    // INFO("%d\r\n", mine_flag);
//    INFO("%.2f,%.2f\r\n", hipnuc_raw.hi91.pitch, yaw_cont.continuous_yaw);
//    INFO("ok\r\n");
    // INFO("%d\r\n", duoji);
    HAL_Delay(10);

    // 妯″紡搴旂敤锟?
    MYMODE_while_application_layer();

    // 锟?鍚懇鎿﹁疆閫昏緫锛岃繑鍥炴懇鎿﹁疆鏄惁鍚姩鏍囧織
    uint8_t mcl_on = MCL_Logic();

    // 鎷ㄧ洏閫昏緫锛屾牴鎹懇鎿﹁疆鐘讹拷?锟藉喅瀹氭槸鍚︽嫧锟?
    BP_Logic(mcl_on);

    // Mini_Pitch_2006鎺у埗閫昏緫
    Mini_Pitch_2006_Logic();

    // 鑸垫満PWM杈撳嚭锛堜粎telescope锛宮ini_pitch宸叉敼鐢?006鐢垫満锛?
    Servo_PWM_Output();

    // 閿洏鐗规畩鍔熻兘澶勭悊
    Keyboard_Special_Func();

    // PITCH杞存帶鍒讹拷?锟借緫
    PITCH_Logic();

    // YAW杞存帶鍒讹拷?锟借緫
    YAW_Logic();

    // 鎺夊ご澶勭悊鍑芥暟
    YAW_Turn_Handle();

    // 澶嶄綅鍑芥暟
    Reset();

    // 浜戝彴鏉垮彂閫佹爣蹇椾綅缁欏簳鐩樻澘
    YT_Tx_static_Flag = (bool)deploy_flag ? (YT_Tx_static_Flag | 0x0001) : (YT_Tx_static_Flag & (uint16_t)~1);
    YT_Tx_static_Flag = XTL_flag ? (YT_Tx_static_Flag | 0x0002) : (YT_Tx_static_Flag & (uint16_t)~2);
    YT_Tx_static_Flag = SP_Turn_Flag ? (YT_Tx_static_Flag | SP_TURN_FLAG) : (YT_Tx_static_Flag & ~SP_TURN_FLAG);
}


/* ===== 从 stm32f4xx_it.c 抽离:USART3 图传接收 PM 区(逐字迁入) ===== */
volatile uint8_t VD_rxcnt = 0;
volatile uint8_t VD_tx_state = 0;
volatile uint8_t u6_tx_cnt = 0;
volatile uint8_t rxcnt_sum = 0;

// rxcnt_sum-VD_rxcnt
uint8_t VD_2rx_buf[2][VD_RX_NUM] = {0};
uint8_t VD_FIFO = 0;
uint16_t VD_rx_byte = 0;
#define VD_DATA_NUM 300
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
    if (VD_2rx_buf[!VD_FIFO][0] == 'V' && VD_2rx_buf[!VD_FIFO][1] == 'D' && VD_rx_byte == VD_DATA_NUM + 4)
    {
      memcpy(NSQD_De_video_buffer, VD_2rx_buf[!VD_FIFO] + 4, VD_DATA_NUM);
      TC.Data_Concatenation(NSQD_De_video_buffer, TX_VD_buf, VD_DATA_NUM, 0x310);
      DMATX = HAL_UART_Transmit_DMA(&huart6, TX_VD_buf, FRAME_HEADER_LENGTH + CMD_ID_LENGTH + VD_DATA_NUM + FRAME_TAIL_LENGTH);
      //   for (uint8_t i = 0; i < sizeof(NSQD_De_video_buffer); i++) {
      //     NSQD_De_video_buffer[i] = VD_2rx_buf[!VD_FIFO][i + 4];
      //   }

      VD_2rx_buf[!VD_FIFO][0] = 0;
      VD_2rx_buf[!VD_FIFO][1] = 0;
      VD_rxcnt++;

      //   if (!video_send_ready) // 上一次发送已完成
      //   {
      //     memcpy(video_buffer, (void *)vision_data.video, 300); // 复制300字节视频数据
      //     video_send_ready = 1;                                 // 启动发送标志
      //     video_send_index = 0;                                 // 从第0包开始
      //   }
      VD_rx_state = 1;
    }
    /*deal*/
  }
}

void TX_VD_Deal(void)
{
  //   if (VD_rx_state && !VD_tx_state) {
  //     TC.Data_Concatenation(NSQD_De_video_buffer, TX_VD_buf, VD_DATA_NUM, 0x310);
  //     HAL_UART_Transmit_DMA(&huart6, TX_VD_buf, FRAME_HEADER_LENGTH + CMD_ID_LENGTH + VD_DATA_NUM + FRAME_TAIL_LENGTH);
  //     VD_rx_state = 0;
  //     VD_tx_state = 1;
  //   }

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


/* ===== 从 stm32f4xx_it.c 抽离:遥控器 YK 中断处理(YK 定义见上方 PTD 区) ===== */
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
