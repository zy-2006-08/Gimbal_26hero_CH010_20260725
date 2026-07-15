#ifndef RGB_DEBUG_H
#define RGB_DEBUG_H

// 前置声明，避免重复包含 RM_Lib.h
class RGB_UI;

#include <stdint.h>
#include <stdbool.h>

/*************************************************************************
 *                          RGB Debug API
 *
 * 功能：将 RGB LED 调试功能抽象成 API 接口
 * 用途：通过 WS2812 灯带实时显示机器人各模块运行状态和故障信息
 *
 * 使用流程：
 * 1. 定义全局变量（RGB_Debug_Config_t, LED_Mapping_t[]）
 * 2. 配置 LED 映射（每个 LED 监控哪些设备）
 * 3. 调用 RGB_Debug_Init() 初始化
 * 4. 注册功能状态显示（RGB_Control_Register_Function）
 * 5. 在 50Hz 定时器中调用 RGB_Debug_Update()
 *************************************************************************/

// 颜色模式定义
#define RED_MODE         0  // 红色闪烁
#define GREEN_MODE       1  // 绿色闪烁
#define BLUE_MODE        2  // 蓝色闪烁
#define RED_GREEN_MODE   3  // 红绿交替闪烁

/*************************************************************************
 *                          数据结构定义
 *************************************************************************/

/**
 * @brief 设备状态结构体
 * @note 用于配置单个设备的监控方式
 */
typedef struct
{
    uint8_t *timeout_ptr;      // 超时计数器指针（指向超时计数变量，如 &timeout_pitch）
                               // 如果不检查超时，设为 NULL
    uint8_t timeout_threshold; // 超时阈值（单位：计数次数，如 10 表示 100ms @ 50Hz）
                               // 当 *timeout_ptr > timeout_threshold 时判定为超时
    uint8_t *status_flag_ptr;  // 状态标志指针（指向设备状态标志，如 &Chassis_Motor_M1_OK）
                               // 如果不检查状态标志，设为 NULL
                               // 当 *status_flag_ptr == 0 时判定为设备故障
} Device_Status_t;

/**
 * @brief LED 映射配置结构体
 * @note 用于配置单个 LED 监控哪些设备
 */
typedef struct
{
    uint8_t led_index;              // LED 索引（0-N，对应 WS2812 灯带上的第几个灯）
    Device_Status_t devices[4];     // 该 LED 监控的设备列表（最多4个）
                                    // 按优先级排列，第一个设备错误时显示第一个颜色
    uint8_t device_count;           // 实际设备数量（1-4）
    uint8_t color_modes[4];         // 对应设备的错误颜色模式
                                    // RED_MODE=0, GREEN_MODE=1, BLUE_MODE=2, RED_GREEN_MODE=3
} LED_Mapping_t;

/**
 * @brief RGB Debug 配置结构体
 * @note 用于初始化 RGB Debug 模块
 */
typedef struct
{
    RGB_UI *rgb_ui;                 // RGB_UI 实例指针（指向已初始化的 RGB_UI 对象）
    LED_Mapping_t *led_mappings;    // LED 映射数组（指向 LED 配置数组）
    uint8_t led_count;              // LED 数量（实际使用的 LED 个数，如 5）
    uint8_t blink_period;           // 闪烁周期（单位：计数次数，默认 50，对应 1秒 @ 50Hz）
} RGB_Debug_Config_t;

/**
 * @brief 功能状态配置结构体
 * @note 用于在无错误时显示功能开关状态
 */
typedef struct
{
    uint8_t led_index;      // LED 索引（0-N）
    uint8_t *flag_ptr;      // 状态标志指针（指向功能开关标志，如 &XTL_flag）
    uint8_t on_color[3];    // 开启时颜色 [R, G, B]（如 {RGB_goal, 0, 0} 表示红色）
    uint8_t off_color[3];   // 关闭时颜色 [R, G, B]（如 {0, RGB_goal, 0} 表示绿色）
} Function_Status_t;

/**
 * @brief LED 4 自瞄状态回调函数类型
 * @note 用于处理 LED 4 的特殊逻辑（自瞄状态有三种颜色）
 */
typedef void (*RGB_LED4_Callback_t)(void);

/*************************************************************************
 *                          核心 API 函数
 *************************************************************************/

/**
 * @brief 初始化 RGB Debug 模块
 * @param config RGB Debug 配置结构体指针
 * @note 必须在使用其他 API 前调用
 * @note config 指向的数据必须在整个运行期间有效（建议使用全局变量）
 */
void RGB_Debug_Init(RGB_Debug_Config_t *config);

/**
 * @brief 注册设备监控（动态添加）
 * @param led_index LED 索引（0-N）
 * @param device 设备状态结构体
 * @param color_mode 错误时的颜色模式（RED_MODE/GREEN_MODE/BLUE_MODE/RED_GREEN_MODE）
 * @note 可以在初始化后动态添加设备监控
 */
void RGB_Debug_Register_Device(uint8_t led_index,
                                Device_Status_t device,
                                uint8_t color_mode);

/**
 * @brief 更新 RGB Debug 状态（50Hz 调用）
 * @note 必须在定时器中断或主循环中以 50Hz 频率调用
 * @note 该函数会检查所有设备状态，更新 LED 显示
 * @note 如果有错误，显示错误闪烁；无错误则调用 RGB_Control_Update() 显示功能状态
 */
void RGB_Debug_Update(void);

/**
 * @brief 设置 LED 闪烁效果
 * @param led_index LED 索引（0-N）
 * @param color_mode 颜色模式（RED_MODE/GREEN_MODE/BLUE_MODE/RED_GREEN_MODE）
 * @note 闪烁频率由 blink_period 决定，默认每 260ms 切换一次
 */
void RGB_Debug_Set_Blink(uint8_t led_index, uint8_t color_mode);

/**
 * @brief 设置 LED 常亮
 * @param led_index LED 索引（0-N）
 * @param r 红色分量（0-255）
 * @param g 绿色分量（0-255）
 * @param b 蓝色分量（0-255）
 */
void RGB_Debug_Set_Solid(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 关闭指定 LED
 * @param led_index LED 索引（0-N）
 */
void RGB_Debug_Clear_LED(uint8_t led_index);

/**
 * @brief 关闭所有 LED
 */
void RGB_Debug_Clear_All(void);

/**
 * @brief 检查是否有错误
 * @return true=有错误, false=无错误
 */
bool RGB_Debug_Has_Error(void);

/*************************************************************************
 *                      功能状态显示 API
 *************************************************************************/

/**
 * @brief 注册功能状态显示
 * @param func_status 功能状态配置结构体
 * @note 用于在无错误时显示功能开关状态（如小陀螺、摩擦轮等）
 * @note 最多支持 8 个功能状态
 */
void RGB_Control_Register_Function(Function_Status_t func_status);

/**
 * @brief 注册 LED 4 自定义回调（用于自瞄状态的特殊逻辑）
 * @param callback 回调函数指针
 * @note LED 4 的自瞄状态有三种颜色（绿/蓝/红），需要自定义回调处理
 */
void RGB_Control_Register_LED4_Callback(RGB_LED4_Callback_t callback);

/**
 * @brief 更新功能状态显示（在无错误时调用）
 * @note 该函数由 RGB_Debug_Update() 自动调用，无需手动调用
 */
void RGB_Control_Update(void);

#endif // RGB_DEBUG_H
