#ifndef __CP_SYSTEM_H
#define __CP_SYSTEM_H



#ifdef __cplusplus
extern "C" {
#endif
	
#include "main.h"
#include "string.h"
#include "stdio.h"	
#include "usart.h"
#include "stdbool.h"
	
#define CP_RX_BUF_SIZE					 512
#define IF_DUM_NORMAL   				 Judge_IF_DUM_Normal() //判断裁判系统和主控是否连接上  1/0
#define CP_SYSTEM_USART_HANDLE   huart3
#define Send_ID   							(Robot_ID_tpyedef)CP.ext_game_robot_status_t.robot_id
#define Receiver_ID 						(Server_ID_tpyedef)(CP.ext_game_robot_status_t.robot_id+256)
/******************************       Robot ID      ************************************/
typedef enum
{
  Red_Hero = 1,
  Red_Engineer,
  Red_Standard1,
  Red_Standard2,
  Red_Standard3,
  Red_Aerial,
  Red_Sentry,
  Red_Radar = 9,

  Blue_Hero = 101,
  Blue_Engineer,
  Blue_Standard1,
  Blue_Standard2,
  Blue_Standard3,
  Blue_Aerial,
  Blue_Sentry,
  Blue_Radar = 109,
}Robot_ID_tpyedef;

/******************************       Server ID      ************************************/
typedef enum
{
	Red_Hero_Server = 0x0101,
	Red_Engineer_Server,
	Red_Standard1_Server,
	Red_Standard2_Server,
	Red_Standard3_Server,
	Red_Aerial_Server,

	Blue_Hero_Server = 0x0165,
	Blue_Engineer_Server,
	Blue_Standard1_Server,
	Blue_Standard2_Server,
	Blue_Standard3_Server,
	Blue_Aerial_Server,
}Server_ID_tpyedef;

/******************************     operate_tpye     ************************************/
typedef enum
{
	None = 0,
	Increase_Graphic,
	Modify_Graphic,
	Delete_Graphic,		
}operate_tpyedef;

/******************************     graphic_tpye     ************************************/
typedef enum 
{
	Graphic_Line = 0,					// 直线
	Graphic_Rectangle, 				// 矩形
	Graphic_Circle,						// 圆
	Graphic_Ellipse, 					// 椭圆
	Graphic_Circular_arc, 		// 圆弧
	Graphic_Float_number, 		// 浮点数
	Graphic_Int_number,				// 整形数
	Graphic_Characters, 			// 字符
}graphic_tpyedef;

/**********************************     Color     ****************************************/
typedef enum 
{
	Color_Red_Or_Bule = 0,
	Color_Yellow, 			
	Color_Green,	
	Color_Orange, 				
	Color_Purple_red, 		// 紫红色
	Color_Pink, 					// 粉色
	Color_Cyan,						// 青色
	Color_Black, 		
	Color_White,			
}Color_tpyedef;


typedef struct CP_typeStruct
{
	struct { 
		uint8_t SOF;
		uint16_t data_length;
		uint8_t seq; 
		uint8_t CRC8; 
	} frame_header;
	uint16_t cmd_id;
	struct { 
		uint8_t game_type : 4;
		uint8_t game_progress : 4;
		uint16_t stage_remain_time;
	} ext_game_status_t;		// 比赛状态数据
	struct { uint8_t winner; } ext_game_result_t;	// 比赛结果数据
	struct
	{
		uint16_t red_1_robot_HP;	// 红 1 英雄机器人血量，未上场以及罚下血量为 0
		uint16_t red_2_robot_HP;	// 红 2 工程机器人血量
		uint16_t red_3_robot_HP;	// 红 3 步兵机器人血量
		uint16_t red_4_robot_HP;	// 红 4 步兵机器人血量
		uint16_t red_5_robot_HP;	// 红 5 步兵机器人血量
		uint16_t red_7_robot_HP;	// 红 7 哨兵机器人血量
		uint16_t red_outpost_HP;	// 红方前哨站血量
		uint16_t red_base_HP;		// 红方基地血量
		uint16_t blue_1_robot_HP;
		uint16_t blue_2_robot_HP;
		uint16_t blue_3_robot_HP;
		uint16_t blue_4_robot_HP;
		uint16_t blue_5_robot_HP;
		uint16_t blue_7_robot_HP;
		uint16_t blue_outpost_HP;
		uint16_t blue_base_HP;
	} ext_game_robot_HP_t;		// 机器人血量数据
	struct
	{
		uint8_t dart_belong;	// 发射飞镖的队伍：1：红方飞镖;  2：蓝方飞镖
		uint16_t stage_remaining_time;	// 发射时的剩余比赛时间，单位 s
	} ext_dart_status_t;		// 飞镖发射状态
	struct { uint32_t event_type; } ext_event_data_t;	// 场地事件数据（己方停机坪占领状态，己方能量机关状态，己方能量机关状态）
	struct { 
		uint8_t supply_projectile_id;	// 补给站口 ID
		uint8_t supply_robot_id;		// 补弹机器人 ID
		uint8_t supply_projectile_step;	// 出弹口开闭状态
		uint8_t supply_projectile_num;	// 补弹数量
	} ext_supply_projectile_action_t;	// 补给站动作标识
	struct { 
		uint8_t supply_projectile_id; 
		uint8_t supply_robot_id;
		uint8_t supply_num; 
	} ext_supply_projectile_booking_t;	//请求补给站补弹数据（RM对抗赛尚未开放）
	struct
	{
		uint8_t level;	// 警告等级
		uint8_t foul_robot_id;	// 犯规机器人 ID
	} ext_referee_warning_t;	// 裁判警告信息
	struct
	{
		uint8_t dart_remaining_time;	// 15s 倒计时
	} ext_dart_remaining_time_t;	// 飞镖发射口倒计时
	struct { 
		uint8_t robot_id; 		// 机器人 ID
		uint8_t robot_level; 	// 机器人等级：1：一级； 2：二级； 3：三级。
		uint16_t remain_HP; 	// 机器人剩余血量
		uint16_t max_HP; 		// 机器人上限血量
		
		uint16_t shooter_id1_17mm_cooling_rate; 	// 机器人 1 号 17mm 枪口每秒冷却值
		uint16_t shooter_id1_17mm_cooling_limit; 	// 机器人 1 号 17mm 枪口热量上限
		uint16_t shooter_id1_17mm_speed_limit; 	// 机器人 1 号 17mm 枪口上限速度 单位 m/s
		
		uint16_t shooter_id2_17mm_cooling_rate;  // 机器人 2 号 17mm 枪口每秒冷却值
		uint16_t shooter_id2_17mm_cooling_limit;  // 机器人 2 号 17mm 枪口热量上限
		uint16_t shooter_id2_17mm_speed_limit;  // 机器人 2 号 17mm 枪口上限速度 单位 m/s
		
		uint16_t shooter_id1_42mm_cooling_rate; 	// 机器人 42mm 枪口每秒冷却值
		uint16_t shooter_id1_42mm_cooling_limit; 	// 机器人 42mm 枪口上限速度 单位 m/s
		uint16_t shooter_id1_42mm_speed_limit; 	// 机器人 42mm 枪口上限速度，单位 m/s
		
		uint16_t chassis_power_limit; 		// 机器人最大底盘功率，单位 W
		uint8_t mains_power_gimbal_output : 1; 	// gimbal口输出：1为有 24V 输出；0为无 24v 输出；
		uint8_t mains_power_chassis_output : 1; // chassis口输出：1为有 24V 输出；0为无 24v 输出；
		uint8_t mains_power_shooter_output : 1; // shooter口输出：1为有 24V 输出；0为无 24v 输出；
	} ext_game_robot_status_t;	// 比赛机器人状态
	struct { 
		uint16_t chassis_volt;			// 底盘输出电压 单位 毫伏
		uint16_t chassis_current;		// 底盘输出电流 单位 毫安
		float chassis_power;			// 底盘输出功率 单位 W 
		uint16_t chassis_power_buffer;	// 底盘功率缓冲，单位 J 焦耳，备注：飞坡根据规则增加至 250J
		uint16_t shooter_id1_17mm_cooling_heat;			// 1 号 17mm 枪口热量
		uint16_t shooter_id2_17mm_cooling_heat;			// 2 号 17mm 枪口热量
		uint16_t shooter_id1_42mm_cooling_heat;			// 42mm 枪口热量
	} ext_power_heat_data_t;	// 实时功率热量数据
	struct {
		float x; 		// 位置 x 坐标，单位 m
		float y; 		// 位置 y 坐标，单位 m
		float z; 		// 位置 z 坐标，单位 m
		float yaw; 		// 位置枪口，单位度
	} ext_game_robot_pos_t;		// 机器人位置
	struct {
		uint8_t power_rune_buff; 
	}ext_buff_t;	// 机器人增益，bit0：机器人血量补血状态； bit1：枪口热量冷却加速； bit2：机器人防御加成； bit3：机器人攻击加成
	struct { 
		uint8_t attack_time; 	// 可攻击时间，单位s，30s递减至0。
	} aerial_robot_energy_t;	// 空中机器人能量状态
	struct { 
		uint8_t armor_id : 4;	// 当血量变化类型为装甲伤害，代表装甲ID，其中数值为 0-4 号代表机器人的五个装甲片，其他血量变化类型，该变量数值为 0
		uint8_t hurt_type : 4;	// 血量变化类型
	} ext_robot_hurt_t;		// 伤害状态
	struct { 
		uint8_t bullet_type; 	// 子弹类型: 1：17mm 弹丸； 2：42mm 弹丸
		uint8_t shooter_id;		// 发射机构ID： 1：1 号 17mm 发射机构； 2：2 号 17mm 发射机构； 3：42mm 发射机构
		uint8_t bullet_freq; 	// 子弹射频 单位 Hz
		float bullet_speed; 	// 子弹射速 单位 m/s
	} ext_shoot_data_t;		// 实时射击信息
	struct { 
		uint16_t bullet_remaining_num_17mm;  // 17mm 子弹剩余发射数目
		uint16_t bullet_remaining_num_42mm;	 // 42mm 子弹剩余发射数目
		uint16_t coin_remaining_num;		// 剩余金币数量
	} ext_bullet_remaining_t;	// 子弹剩余发射数
	struct {
		uint32_t rfid_status; 
	}ext_rfid_status_t;	// 机器人 RFID 状态
	struct
	{
		uint8_t dart_launch_opening_status;		// 当前飞镖发射口的状态
		uint8_t dart_attack_target;		// 飞镖的打击目标，默认为前哨站；1：前哨站；2：基地
		uint16_t target_change_time;	// 切换打击目标时的比赛剩余时间，单位秒，从未切换默认为 0
		uint16_t operate_launch_cmd_time; // 最近一次操作手确定发射指令时的比赛剩余时间，单位秒, 初始值为 0
		
//		uint8_t first_dart_speed;		// 检测到的第一枚飞镖速度，单位 0.1m/s/LSB, 未检测是为 0     //2021年1.0版本有，但2021年1.2版本没有
//		uint8_t second_dart_speed;		// 检测到的第二枚飞镖速度，单位 0.1m/s/LSB, 未检测是为 0
//		uint8_t third_dart_speed;		// 检测到的第三枚飞镖速度，单位 0.1m/s/LSB, 未检测是为 0
//		uint8_t fourth_dart_speed;		// 检测到的第四枚飞镖速度，单位 0.1m/s/LSB, 未检测是为 0
//		uint16_t last_dart_launch_time;	// 最近一次的发射飞镖的比赛剩余时间，单位秒，初始值为 0		
	} ext_dart_client_cmd_t;	// 飞镖机器人客户端指令数据
	struct { 
		uint16_t data_cmd_id; 	// 数据段ID
		uint16_t send_ID; 		// 发送者ID
		uint16_t receiver_ID; 	// 接收者ID
	} ext_student_interactive_header_data_t;	// 交互数据段头
	struct {
		uint8_t data[128];
	} robot_interactive_data_t;		// 机器人间交互数据内容
	struct {
		uint8_t operate_tpye;
		uint8_t layer;
	} ext_client_custom_graphic_delete_t;	// 删除图形
	struct graphic_data_struct_t{
		uint8_t graphic_name[3];	// 图形名
		uint32_t operate_tpye:3;
		uint32_t graphic_tpye:3;
		uint32_t layer:4;
		uint32_t color:4;
		uint32_t start_angle:9;
		uint32_t end_angle:9;	// 图形配置 1
		uint32_t width:10;
		uint32_t start_x:11;
		uint32_t start_y:11;	// 图形配置 2
		uint32_t radius:10;
		uint32_t end_x:11;
		uint32_t end_y:11;		// 图形配置 3
	} graphic_data_struct_temp;	// 图形数据
	struct { struct graphic_data_struct_t grapic_data_struct; } ext_client_custom_graphic_single_t;	// 客户端绘制一个图形
	struct { struct graphic_data_struct_t grapic_data_struct[2]; } ext_client_custom_graphic_double_t; // 客户端绘制二个图形
	struct { struct graphic_data_struct_t grapic_data_struct[5]; } ext_client_custom_graphic_five_t; // 客户端绘制五个图形
	struct { struct graphic_data_struct_t grapic_data_struct[7]; } ext_client_custom_graphic_seven_t; // 客户端绘制七个图形
	struct { 
		struct graphic_data_struct_t grapic_data_struct;
		uint8_t data[30];
	} ext_client_custom_character_t;	// 客户端绘制字符
	struct {
		float target_position_x;
		float target_position_y;
		float target_position_z;
		uint8_t commd_keyboard;
		uint16_t target_robot_ID;
	} ext_robot_command_t;	 // 小地图下发信息
	struct{
		uint16_t target_robot_ID;
		float target_position_x;
		float target_position_y;
	}ext_client_map_command_t;	//	小地图接收信息
	struct {
		int16_t mouse_x;
		int16_t mouse_y;
		int16_t mouse_z;
		int8_t left_button_down;
		int8_t right_button_down;
		uint16_t keyboard_value;
	} ext_robot_vision_command_t;    // 图传遥控信息
	struct{
	float data1;
	float data2;
	float data3;
	uint8_t masks;
	}client_custom_data_t;
	struct { 
		uint16_t CRC16;
	} frame_tail;
  struct{
	 float hero_x;
	 float hero_y;
	 float engineer_x;
	 float engineer_y;
	 float standard_3_x;
	 float standard_3_y;
	 float standard_4_x;
	 float standard_4_y;
	 float standard_5_x;
	 float standard_5_y;
	}ground_robot_position_t;
	struct{
	 uint8_t mark_hero_progress;
	uint8_t mark_engineer_progress;
	 uint8_t mark_standard_3_progress;
	 uint8_t mark_standard_4_progress;
	 uint8_t mark_standard_5_progress;
	 uint8_t mark_sentry_progress;
	}radar_mark_data_t;	
	struct{
	 uint16_t key_value;
	 uint16_t x_position:12;
	 uint16_t mouse_left:4;
	 uint16_t y_position:12;
	 uint16_t mouse_right:4;
	 uint16_t reserved;
	}custom_client_data_t;	
	struct{
	 uint8_t intention;
	 uint16_t start_position_x;
	 uint16_t start_position_y;
	 int8_t delta_x[49];
	 int8_t delta_y[49];
	}map_sentry_data_t;
}CP_typedef;


#define CP_mouse_x						CP.ext_robot_vision_command_t.mouse_x
#define CP_mouse_y						CP.ext_robot_vision_command_t.mouse_y
#define CP_mouse_z						CP.ext_robot_vision_command_t.mouse_z
#define CP_press_left					CP.ext_robot_vision_command_t.mouse_y
#define CP_press_right				CP.ext_robot_vision_command_t.mouse_z
#define CP_jianpan            CP.ext_robot_vision_command_t.keyboard_value
#define CP_KEY_PRESSED_W 			((uint16_t)(CP_jianpan&0x0001)>>0)
#define CP_KEY_PRESSED_S 			((uint16_t)(CP_jianpan&0x0002)>>1)
#define CP_KEY_PRESSED_A 			((uint16_t)(CP_jianpan&0x0004)>>2)
#define CP_KEY_PRESSED_D 			((uint16_t)(CP_jianpan&0x0008)>>3)
#define CP_KEY_PRESSED_SHIFT 	((uint16_t)(CP_jianpan&0x0010)>>4)
#define CP_KEY_PRESSED_CTRL 	((uint16_t)(CP_jianpan&0x0020)>>5)
#define CP_KEY_PRESSED_Q 			((uint16_t)(CP_jianpan&0x0040)>>6)
#define CP_KEY_PRESSED_E 			((uint16_t)(CP_jianpan&0x0080)>>7)
#define CP_KEY_PRESSED_R 			((uint16_t)(CP_jianpan&0x0100)>>8)
#define CP_KEY_PRESSED_F 			((uint16_t)(CP_jianpan&0x0200)>>9)
#define CP_KEY_PRESSED_G 			((uint16_t)(CP_jianpan&0x0400)>>10)
#define CP_KEY_PRESSED_Z 			((uint16_t)(CP_jianpan&0x0800)>>11)
#define CP_KEY_PRESSED_X 			((uint16_t)(CP_jianpan&0x1000)>>12)
#define CP_KEY_PRESSED_C 			((uint16_t)(CP_jianpan&0x2000)>>13)
#define CP_KEY_PRESSED_V 			((uint16_t)(CP_jianpan&0x4000)>>14)
#define CP_KEY_PRESSED_B 			((uint16_t)(CP_jianpan&0x8000)>>15)

extern uint8_t CP_rev_buf[2][CP_RX_BUF_SIZE];
extern uint8_t CP_send_buf[256];
extern CP_typedef CP;

static uint16_t CP_data_deal(void);
static unsigned char Get_CRC8_Check_Sum(unsigned char *pchMessage,unsigned int dwLength);
static void Append_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength);
static uint16_t Get_CRC16_Check_Sum(uint8_t *pchMessage,uint32_t dwLength);
static void Append_CRC16_Check_Sum(uint8_t * pchMessage,uint32_t dwLength);

extern void CP_System_Init();
extern void CP_System_IRQHandler(DMA_HandleTypeDef* hdma);
extern void CP_System_DMACplt_DataDeal();
extern bool Judge_IF_DUM_Normal(void);

extern void CP_Delete_Graphic(uint8_t operate_tpye,uint8_t layer);
extern void CP_DrawOrDelete_One_Graphic(uint32_t graphic_name,operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye,uint8_t layer,Color_tpyedef color,
																				uint16_t start_angle,uint16_t end_angle,uint16_t width,uint16_t start_x,uint16_t start_y,
																				uint16_t radius,uint16_t end_x,uint16_t end_y);
extern void CP_DrawOrDelete_Two_Graphic(uint32_t graphic_name[2],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[2],uint8_t layer,Color_tpyedef color[2],
																				uint16_t start_angle[2],uint16_t end_angle[2],uint16_t width[2],uint16_t start_x[2],uint16_t start_y[2],
																				uint16_t radius[2],uint16_t end_x[2],uint16_t end_y[2]);
extern void CP_DrawOrDelete_Five_Graphic(uint32_t graphic_name[5],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[5],uint8_t layer,Color_tpyedef color[5],
																				 uint16_t start_angle[5],uint16_t end_angle[5],uint16_t width[5],uint16_t start_x[5],uint16_t start_y[5],
																				 uint16_t radius[5],uint16_t end_x[5],uint16_t end_y[5]);
extern void CP_DrawOrDelete_Seven_Graphic(uint32_t graphic_name[7],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[7],uint8_t layer,Color_tpyedef color[7],
																					uint16_t start_angle[7],uint16_t end_angle[7],uint16_t width[7],uint16_t start_x[7],uint16_t start_y[7],
																					uint16_t radius[7],uint16_t end_x[7],uint16_t end_y[7]);
extern void CP_DrawOrDelete_One_Number(uint32_t graphic_name,operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye,uint8_t layer,Color_tpyedef color,
																			 uint16_t number_size,uint16_t number_digit,uint16_t width,uint16_t start_x,uint16_t start_y,
																			 float number);
extern void CP_DrawOrDelete_Char(uint32_t graphic_name,operate_tpyedef operate_tpye,uint8_t layer,Color_tpyedef color,
																 uint16_t char_size,uint16_t width,uint16_t start_x,uint16_t start_y,
																 uint8_t* data);
extern void CP_Robot_SendBytes(uint16_t data_cmd_id,uint16_t* data,uint8_t size);
extern void CP_Robot_SendByte(uint16_t data_cmd_id,uint16_t data);
extern void CP_Map(Robot_ID_tpyedef target_ID,uint16_t target_x,uint16_t target_y,uint16_t target_dir);

	
#ifdef __cplusplus
}
#endif

#endif
