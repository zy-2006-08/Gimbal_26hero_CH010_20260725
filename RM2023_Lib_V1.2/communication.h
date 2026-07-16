#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "main.h"
#include "string.h"
#include <stdint.h>
#include <stdbool.h>
#include "my_math.h"

#define MINI_PC_BUF_SIZE 		  128		// 数组大小，可修改

#define Communication_Mode  1
	 
#define Communication_huart 1
#define Communication_USB_VCP 2
#define Communication_USB_HID 3	 
	 
	 
#if Communication_Mode == Communication_huart
	 
#include "usart.h"
	 
#define MINI_PC_USART_HANDLE 		huart3// 串口号，可修改
extern uint8_t Mini_PC_info_ubuf[MINI_PC_BUF_SIZE];
#define Mini_PC_INFO(...) HAL_UART_Transmit (&MINI_PC_USART_HANDLE,\
													(uint8_t *)Mini_PC_info_ubuf,\
													sprintf((char *)Mini_PC_info_ubuf,__VA_ARGS__),\
													0xffff)
// SuperPower协议帧格式定义
#define F_Byte              4        // 浮点数字节数

#define SP_HEADER           0x66     // SuperPower帧头标识
#define SP_TAIL             0x11     // SuperPower帧尾标识  
#define SP_TOTAL_LEN        29       // SuperPower数据包总长度

// SuperPower数据偏移量定义
#define SP_OFFSET_YAW       2                           // YAW角度偏移
#define SP_OFFSET_YAW_VEL   (SP_OFFSET_YAW + F_Byte * 1)   // YAW角速度偏移
#define SP_OFFSET_YAW_ACC   (SP_OFFSET_YAW + F_Byte * 2)   // YAW角加速度偏移
#define SP_OFFSET_PITCH     (SP_OFFSET_YAW + F_Byte * 3)   // PITCH角度偏移
#define SP_OFFSET_PITCH_VEL (SP_OFFSET_YAW + F_Byte * 4)   // PITCH角速度偏移
#define SP_OFFSET_PITCH_ACC (SP_OFFSET_YAW + F_Byte * 5)   // PITCH角加速度偏移

#elif Communication_Mode == Communication_USB_VCP 

#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#elif Communication_Mode == Communication_USB_HID

#include "usbd_customhid.h"
#include "usbd_custom_hid_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

#endif

extern uint8_t Mini_PC_tx_buf[128];
extern uint8_t Mini_PC_rx_buf[128];												
typedef uint8_t u8; 

typedef union
{
    float f;
    uint8_t c[4];
} _angle;

typedef union
{
    float f;
    uint8_t c[4];
} _data;

// 16位数据类型联合体,用于CRC16校验
typedef union
{
    uint16_t typeMum;  // 16位整数形式
    uint8_t c[2];      // 字节数组形式,方便传输
}_d16_t;

// 四元数姿态发送结构体
typedef struct{
   _angle Q_info_0;   // 四元数w分量
   _angle Q_info_1;   // 四元数x分量
   _angle Q_info_2;   // 四元数y分量
   _angle Q_info_3;   // 四元数z分量
   _d16_t crc_Num;    // CRC16校验值
}_type_zm;

typedef struct
{
    uint8_t mine;                            //己方颜色: 红/蓝  1/0
		uint8_t buff_status;								      //符/普通      1    0 
	  uint8_t zimiao_status;                   //自瞄      1  0
		uint8_t close_PC_status;                 //关小电脑  1 0
		uint8_t adjust_camera;										//调整相机    1 0
	  uint8_t shooter_speed_limit;	            //上限射速
    uint8_t deploy_flag;                      //部署模式
		_data Pitch_Angle;
		_data	Roll_Angle;
		_data	Yaw_Anglespeed;
	  _data Yaw_Angle;         //Y轴角度
	  _data pitch_mang;
		_data yaw_mang;
}_request;

typedef struct
{
    _angle pitch;
    _angle yaw;
		_angle distance;
    _angle fly_time;
    float distance_zm;    //射击模式  ：自瞄/能量机关
	uint8_t Fire_Flag;
	uint8_t a;
	uint8_t b;
}_response;


// CRC 表(算法B DJI-CRC16 / 算法C DJI-CRC8 / 算法D Mini_PC-CRC8)已全部迁入统一 crc 模块(crc.c/crc.h),
// 此处原有的重复表定义(CRC8_TABLE / CRC16_TABLE 及其 INIT 常量,均已确认零引用)已删除,不再重复定义。
typedef union {
    float f;
    volatile uint8_t c[4];
} FloatBytes_t;
typedef struct
{
        u8 mode;
    FloatBytes_t yaw;
    FloatBytes_t yaw_vel;
    FloatBytes_t yaw_acc;
    FloatBytes_t pitch;
    FloatBytes_t pitch_vel;
    FloatBytes_t pitch_acc;
    _d16_t crc_Num;   
}_SuperPower;
typedef union {
    float f;
    uint8_t c[4];
} BulletSpeed_u;

extern BulletSpeed_u bullet_speed_u;
extern uint16_t mine_flag;
extern float shoot_sp;
// 标准的云台到视觉通讯数据包
struct __attribute__((packed)) GimbalToVision
{
  uint8_t head[2];               // 帧头 "SP"(纯C兼容:不在结构体内默认初始化;该结构体全工程零实例化)
  uint8_t mode;                   // 0:空闲, 1:自瞄, 2:小符, 3:大符
  float q[4];                     // 四元数 wxyz顺序
  float bullet_speed;             // 弹速
  uint8_t color;                   // 己方颜色
  uint16_t crc16;                 // CRC16校验
};
//带volatile 不然数据类型报错
static inline void AssignBytes(volatile uint8_t *dest, 
                               volatile const uint8_t *src, 
                               uint32_t offset, 
                               uint32_t len) {
    for(uint32_t i = 0; i < len; i++) {
        dest[i] = src[offset + i];
    }
}

static inline void UnpackFloat(volatile const uint8_t *buf, 
                               uint32_t offset, 
                               volatile FloatBytes_t *target) {
    AssignBytes(target->c, buf, offset, 4);
}
//数据有效性检测 inline调用函数体 减少性能开支                              
static inline void UnpackFloatSafe(volatile const uint8_t *buf, uint32_t offset, volatile FloatBytes_t *target)
{
    UnpackFloat(buf,offset,target);
    if(target->f > 360.0f || target->f < -360.0f || isnan(target->f)) {
        target->f = 0.0f;
    }
}

extern volatile _type_zm  AS;           // 四元数姿态数据
//extern _request_union request_union;    // 优化的发送数据
extern volatile _SuperPower SuperPower; // SuperPower接收数据
// SuperPower数据接收处理函数
void GetReceive_SP(uint8_t (*buf));

// CRC16相关函数
uint16_t get_crc16(const uint8_t * data, uint32_t len);
bool check_crc16(const uint8_t * data, uint32_t len);


//数据收发单体
extern volatile _request request;
extern volatile _response  response;
extern uint8_t Nav_rx_buf[64];
//extern _request_union request_union;

extern uint8_t cal_crc_table(uint8_t *ptr, uint8_t len);
extern void Mini_PC_Init(void);
extern void Mini_PC_UART_Init(void);
extern void Mini_PC_SendData();
extern void Mini_PC_SendData_ZM();  
extern void Mini_PC_SendData_ZM2(uint8_t aim_mode);
extern void getReceiveData(uint8_t (*buf));
extern uint8_t cal_crc_table(uint8_t *ptr, uint8_t len);
extern void Mini_PC_newSendData(float pitchAngle,float YawAngle,uint8_t color,uint8_t buff);
extern void Nav_UART_Init(void);
extern void getNavPitchData(uint8_t *buf);
extern void getReceiveData_ZM(uint8_t(*buf));
#ifdef __cplusplus
}
#endif

#endif
