#include "rgb_debug.h"
#include "RM_Lib.h"  // 包含 RGB_UI 完整定义
#include <stddef.h>  // 包含 NULL 定义

/*************************************************************************
 *                          内部全局变量
 *************************************************************************/

static RGB_Debug_Config_t *g_config = NULL;           // 配置指针
static uint16_t g_rgb_cnt = 0;                        // RGB 计数器（0-49循环）
static bool g_has_error = false;                      // 错误标志
static Function_Status_t g_function_status[8];        // 功能状态数组（最多8个）
static uint8_t g_function_count = 0;                  // 功能状态数量
static RGB_LED4_Callback_t g_led4_callback = NULL;    // LED 4 自定义回调

// RGB_goal 定义（与原代码一致）
#define RGB_goal 225

/*************************************************************************
 *                          核心 API 实现
 *************************************************************************/

/**
 * @brief 初始化 RGB Debug 模块
 */
void RGB_Debug_Init(RGB_Debug_Config_t *config)
{
    // 保存配置指针
    g_config = config;

    // 重置内部状态
    g_rgb_cnt = 0;
    g_has_error = false;
    g_function_count = 0;
    g_led4_callback = NULL;

    // 初始化 RGB_UI（如果还没初始化）
    if (g_config->rgb_ui != NULL)
    {
        g_config->rgb_ui->RGB_UI_Init();
    }

    // 关闭所有 LED
    RGB_Debug_Clear_All();
}

/**
 * @brief 注册设备监控（动态添加）
 */
void RGB_Debug_Register_Device(uint8_t led_index,
                                Device_Status_t device,
                                uint8_t color_mode)
{
    if (g_config == NULL || led_index >= g_config->led_count)
        return;

    LED_Mapping_t *mapping = &g_config->led_mappings[led_index];

    if (mapping->device_count >= 4)  // 最多4个设备
        return;

    // 添加设备
    mapping->devices[mapping->device_count] = device;
    mapping->color_modes[mapping->device_count] = color_mode;
    mapping->device_count++;
}

/**
 * @brief 更新 RGB Debug 状态（50Hz 调用）
 */
void RGB_Debug_Update(void)
{
    if (g_config == NULL)
        return;

    // 1. 更新计数器
    g_rgb_cnt++;
    if (g_rgb_cnt >= g_config->blink_period)
        g_rgb_cnt = 0;

    // 2. 检查所有设备状态
    g_has_error = false;
    for (uint8_t i = 0; i < g_config->led_count; i++)
    {
        LED_Mapping_t *mapping = &g_config->led_mappings[i];
        bool led_has_error = false;
        uint8_t error_color = 0;

        // 检查该 LED 对应的所有设备
        for (uint8_t j = 0; j < mapping->device_count; j++)
        {
            Device_Status_t *device = &mapping->devices[j];
            bool device_error = false;

            // 检查超时
            if (device->timeout_ptr != NULL &&
                *device->timeout_ptr > device->timeout_threshold)
            {
                device_error = true;
            }

            // 检查状态标志
            if (device->status_flag_ptr != NULL &&
                (*device->status_flag_ptr) == 0)  // 状态标志为 0 表示故障
            {
                device_error = true;
            }

            // 如果设备有错误，记录颜色模式
            if (device_error)
            {
                led_has_error = true;
                error_color = mapping->color_modes[j];
                g_has_error = true;
                break;  // 优先显示第一个错误
            }
        }

        // 3. 设置 LED 状态
        if (led_has_error)
        {
            RGB_Debug_Set_Blink(i, error_color);
        }
        else
        {
            RGB_Debug_Clear_LED(i);
        }
    }

    // 4. 如果没有错误，显示功能状态
    if (!g_has_error)
    {
        RGB_Control_Update();
    }
}

/**
 * @brief 设置 LED 闪烁效果（与原代码完全一致）
 */
void RGB_Debug_Set_Blink(uint8_t led_index, uint8_t color_mode)
{
    if (g_config == NULL || g_config->rgb_ui == NULL)
        return;

    // 闪烁逻辑：每 13 个计数切换一次亮/灭
    // 在 50Hz 下，13 个计数 = 260ms
    uint8_t time1 = (g_rgb_cnt / 13) % 2;
    if (time1 == 0)  // 灭
    {
        g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, 0, 0);
        return;
    }

    // 亮的时候根据颜色模式设置
    switch (color_mode)
    {
        case RED_MODE:  // 红色闪烁
            g_config->rgb_ui->WS281x_SetPixelRGB(led_index, RGB_goal, 0, 0);
            break;
        case GREEN_MODE:  // 绿色闪烁
            g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, RGB_goal, 0);
            break;
        case BLUE_MODE:  // 蓝色闪烁
            g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, 0, RGB_goal);
            break;
        case RED_GREEN_MODE:  // 红绿交替闪烁
        {
            // 红绿交替逻辑：每 13 个计数切换颜色
            // 0->灭, 1->红亮, 2->灭, 3->绿亮, 循环
            uint8_t color = (g_rgb_cnt / 13) % 4;
            if (color == 1)  // 红亮
                g_config->rgb_ui->WS281x_SetPixelRGB(led_index, RGB_goal, 0, 0);
            else if (color == 3)  // 绿亮
                g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, RGB_goal, 0);
            else  // 灭
                g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, 0, 0);
            break;
        }
        default:
            g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, 0, 0);
            break;
    }
}

/**
 * @brief 设置 LED 常亮
 */
void RGB_Debug_Set_Solid(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b)
{
    if (g_config == NULL || g_config->rgb_ui == NULL)
        return;

    g_config->rgb_ui->WS281x_SetPixelRGB(led_index, r, g, b);
}

/**
 * @brief 关闭指定 LED
 */
void RGB_Debug_Clear_LED(uint8_t led_index)
{
    if (g_config == NULL || g_config->rgb_ui == NULL)
        return;

    g_config->rgb_ui->WS281x_SetPixelRGB(led_index, 0, 0, 0);
}

/**
 * @brief 关闭所有 LED
 */
void RGB_Debug_Clear_All(void)
{
    if (g_config == NULL || g_config->rgb_ui == NULL)
        return;

    for (uint8_t i = 0; i < g_config->led_count; i++)
    {
        g_config->rgb_ui->WS281x_SetPixelRGB(i, 0, 0, 0);
    }
    g_config->rgb_ui->WS_Load();  // 刷新显示
}

/**
 * @brief 检查是否有错误
 */
bool RGB_Debug_Has_Error(void)
{
    return g_has_error;
}

/*************************************************************************
 *                      功能状态显示 API 实现
 *************************************************************************/

/**
 * @brief 注册功能状态显示
 */
void RGB_Control_Register_Function(Function_Status_t func_status)
{
    if (g_function_count >= 8)  // 最多支持 8 个功能
    {
        // 错误处理：超出最大数量
        return;
    }

    // 保存功能状态配置
    g_function_status[g_function_count] = func_status;
    g_function_count++;
}

/**
 * @brief 注册 LED 4 自定义回调
 */
void RGB_Control_Register_LED4_Callback(RGB_LED4_Callback_t callback)
{
    g_led4_callback = callback;
}

/**
 * @brief 更新功能状态显示（在无错误时调用）
 */
void RGB_Control_Update(void)
{
    if (g_config == NULL || g_config->rgb_ui == NULL)
        return;

    // 遍历所有注册的功能状态
    for (uint8_t i = 0; i < g_function_count; i++)
    {
        Function_Status_t *func = &g_function_status[i];

        // 检查功能标志
        if (*(func->flag_ptr))  // 功能开启
        {
            g_config->rgb_ui->WS281x_SetPixelRGB(func->led_index,
                func->on_color[0], func->on_color[1], func->on_color[2]);
        }
        else  // 功能关闭
        {
            g_config->rgb_ui->WS281x_SetPixelRGB(func->led_index,
                func->off_color[0], func->off_color[1], func->off_color[2]);
        }
    }

    // LED 4 特殊处理（自瞄状态，如果注册了回调）
    if (g_led4_callback != NULL)
    {
        g_led4_callback();
    }
}
