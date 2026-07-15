
/*
 * Copyright (c) 2006-2024, HiPNUC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "hipnuc_dec.h"
#include <math.h>
  
// 在文件开头全局变量定义部分添加
Hipnuc_Anglespeed_t hipnuc_anglespeed = {0};
Hipnuc_LastAngle_t hipnuc_lastangle = {0};

// 滤波计数器和上次值
static float last_speed_pitch = 0;
static float last_speed_yaw = 0; 
static float last_speed_roll = 0;
static uint8_t filter_count_pitch = 0;
static uint8_t filter_count_yaw = 0;
static uint8_t filter_count_roll = 0;

// 死区阈值，可以根据实际调整
#define HIPNUC_DEAD_ZOOM 0.5f

/**
 * @brief  计算hipnuc角速度（通过欧拉角微分）
 * @note   在数据更新后调用，频率与数据更新频率一致
 */
void Hipnuc_Analyse_Speed(void)
{
    // 计算角速度 = 角度变化 / 时间间隔
    // 假设更新频率为1kHz，dt = 0.001s
    hipnuc_anglespeed.pitch = (hipnuc_raw.hi91.pitch - hipnuc_lastangle.pitch) / 0.001f;
    hipnuc_anglespeed.yaw   = (yaw_cont.continuous_yaw - hipnuc_lastangle.yaw) / 0.001f;  // 使用连续yaw角
    hipnuc_anglespeed.roll  = (hipnuc_raw.hi91.roll - hipnuc_lastangle.roll) / 0.001f;
    
    // Pitch轴滤波处理
    if(fabs(hipnuc_anglespeed.pitch - last_speed_pitch) > 7.0f && 
       filter_count_pitch < 2)
    {
        filter_count_pitch++;
        // 突变时保持上次值
        hipnuc_anglespeed.Deal_pitch = last_speed_pitch;
    }
    else 
    {
        if(fabs(hipnuc_anglespeed.pitch) < HIPNUC_DEAD_ZOOM)
            hipnuc_anglespeed.Deal_pitch = 0;
        else 
            hipnuc_anglespeed.Deal_pitch = hipnuc_anglespeed.pitch;
        
        last_speed_pitch = hipnuc_anglespeed.pitch;
        filter_count_pitch = 0;
    }
    
    // Yaw轴滤波处理  
    if(fabs(hipnuc_anglespeed.yaw - last_speed_yaw) > 7.0f && 
       filter_count_yaw < 2)
    {
        filter_count_yaw++;
        hipnuc_anglespeed.Deal_yaw = last_speed_yaw;
    }
    else 
    {
        if(fabs(hipnuc_anglespeed.yaw) < HIPNUC_DEAD_ZOOM)
            hipnuc_anglespeed.Deal_yaw = 0;
        else 
            hipnuc_anglespeed.Deal_yaw = hipnuc_anglespeed.yaw;
        
        last_speed_yaw = hipnuc_anglespeed.yaw;
        filter_count_yaw = 0;
    }
    
    // Roll轴滤波处理
    if(fabs(hipnuc_anglespeed.roll - last_speed_roll) > 7.0f && 
       filter_count_roll < 2)
    {
        filter_count_roll++;
        hipnuc_anglespeed.Deal_roll = last_speed_roll;
    }
    else 
    {
        if(fabs(hipnuc_anglespeed.roll) < HIPNUC_DEAD_ZOOM)
            hipnuc_anglespeed.Deal_roll = 0;
        else 
            hipnuc_anglespeed.Deal_roll = hipnuc_anglespeed.roll;
        
        last_speed_roll = hipnuc_anglespeed.roll;
        filter_count_roll = 0;
    }
    
    // 更新上次角度值供下次计算使用
    hipnuc_lastangle.pitch = hipnuc_raw.hi91.pitch;
    hipnuc_lastangle.yaw   = yaw_cont.continuous_yaw;
    hipnuc_lastangle.roll  = hipnuc_raw.hi91.roll;
}




/* CH010 IMU*/
hipnuc_raw_t hipnuc_raw = {0};
hi91_t imu_data = {0};
/*DMA buff*/
uint8_t imu_dma_rxbuf[2][IMU_DMA_BUF_SIZE]; // 数据双缓存数组
uint16_t IMU_rx_size = 0;
volatile uint8_t IMU_FIFO = 0;
extern DMA_HandleTypeDef hdma_uart4_rx;
void IMU_UART_Init(void)
{
    /* 清零结构体 */
    memset(&hipnuc_raw, 0, sizeof(hipnuc_raw_t));
    memset(&imu_data, 0, sizeof(hi91_t));
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_IDLE);
    // __HAL_DMA_DISABLE_IT(&hdma_uart4_rx, DMA_IT_HT); // 可选：禁用半传输中断
    HAL_UART_Receive_DMA(&huart4, imu_dma_rxbuf[0], IMU_DMA_BUF_SIZE);
}

Yaw_Continuous_t yaw_cont = {0, 0, 0, 1};
/**
 * @brief  Yaw角度过圈处理函数
 * @param  raw_yaw: 原始yaw角度 (-180 ~ +180)
 * @retval 连续角度（可无限大）
 */
float Yaw_Continuous_Update(float raw_yaw)
{
    float delta;
    
    // 首次运行初始化
    if (yaw_cont.first_run)
    {
        yaw_cont.first_run = 0;
        yaw_cont.last_raw_yaw = raw_yaw;
        yaw_cont.continuous_yaw = raw_yaw;
        yaw_cont.round_count = 0;
        return yaw_cont.continuous_yaw;
    }
    
    // 计算角度变化量
    delta = raw_yaw - yaw_cont.last_raw_yaw;
    
    // 检测过圈（跳变超过180°认为是过圈）
    if (delta > 180.0f)
    {
        // 从正到负的跳变，例如: +179 -> -179，实际是逆时针转了2度
        yaw_cont.round_count--;
    }
    else if (delta < -180.0f)
    {
        // 从负到正的跳变，例如: -179 -> +179，实际是顺时针转了2度
        yaw_cont.round_count++;
    }
    
    // 计算连续角度 = 圈数 * 360 + 当前原始角度
    yaw_cont.continuous_yaw = yaw_cont.round_count * 360.0f + raw_yaw;
    
    // 保存当前值供下次使用
    yaw_cont.last_raw_yaw = raw_yaw;
    
    return yaw_cont.continuous_yaw;
}

/**
 * @brief  重置连续角度（上电或需要归零时调用）
 */
void Yaw_Continuous_Reset(void)
{
    yaw_cont.first_run = 1;
    yaw_cont.round_count = 0;
    yaw_cont.continuous_yaw = 0;
    yaw_cont.last_raw_yaw = 0;
}

/**
 * @brief  获取当前连续角度 
 */
float Get_Continuous_Yaw(void)
{
    return yaw_cont.continuous_yaw;
}

/**
 * @brief  获取圈数
 */
int32_t Get_Yaw_Round_Count(void)
{
    return yaw_cont.round_count;
}

void CrossRound(void)
{

}
void IMU_UART_IRQHandler(DMA_HandleTypeDef *hdma)
{
    uint32_t tmp_flag = 0;
    uint32_t temp;
    uint32_t get_counter=0;
    tmp_flag = __HAL_UART_GET_FLAG(&IMU_UART_HANDLE, UART_FLAG_IDLE);
    if (tmp_flag != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&IMU_UART_HANDLE);
        temp = IMU_UART_HANDLE.Instance->SR;
        temp = IMU_UART_HANDLE.Instance->DR;
        HAL_UART_DMAStop(&IMU_UART_HANDLE);
        // temp = hdma->Instance->NDTR;
        get_counter =__HAL_DMA_GET_COUNTER(&hdma_uart4_rx);
        IMU_rx_size = IMU_DMA_BUF_SIZE - get_counter;

        IMU_FIFO = !IMU_FIFO;

        // CP_data_deal();
        // if()
        HAL_UART_Receive_DMA(&IMU_UART_HANDLE, imu_dma_rxbuf[IMU_FIFO], IMU_DMA_BUF_SIZE);
    }
    

    if (tmp_flag != RESET)
    {
        for (uint16_t i = 0; i < IMU_rx_size; i++)
        {
            if (hipnuc_input(&hipnuc_raw, imu_dma_rxbuf[!IMU_FIFO][i]))
            {
                Yaw_Continuous_Update(hipnuc_raw.hi91.yaw);
                if (fabsf(hipnuc_raw.hi91.pitch) < 2.0f)
                {
                    hipnuc_raw.hi91.pitch = 0;
                }
                if (fabsf(hipnuc_raw.hi91.yaw) < 2.0f)
                {
                    hipnuc_raw.hi91.yaw = 0;
                }                
            }
            // uart_rx_buf[uart_rx_index++] = dma_rx_buf[i];

            // if (uart_rx_index >= UART_RX_BUF_SIZE)
            // {
            //     uart_rx_index = 0; // Prevent buffer overflow 防止数组越界
            // }
        }
    }
}

void process_data(void)
{
}
/* The driver file for decoding HiPNUC protocol, DO NOT MODIFTY
HiPNUC协议解码驱动文件，请勿修改*/

/* HiPNUC protocol constants
协议常数*/
#define CHSYNC1 (0x5A)     /* CHAOHE message sync code 1 CHAOHE消息同步码1 */
#define CHSYNC2 (0xA5)     /* CHAOHE message sync code 2 CHAOHE消息同步码2*/
#define CH_HDR_SIZE (0x06) /* CHAOHE protocol header size CHAOHE协议头大小*/

/* new HiPNUC standard packet
新的HiPNUC标准包*/
#define HIPNUC_ID_HI91 (0x91)
#define HIPNUC_ID_HI81 (0x81)
#define HIPNUC_ID_HI83 (0x83)

#ifndef D2R /*角度转弧度*/
#define D2R (0.0174532925199433F)
#endif

#ifndef R2D /*弧度转角度*/
#define R2D (57.2957795130823F)
#endif

#ifndef GRAVITY
#define GRAVITY (9.8F)
#endif

static void hipnuc_crc16(uint16_t *inital, const uint8_t *buf, uint32_t len);

/* common type conversion
通用类型转换*/
#define I2(p) (*((int16_t *)(p)))
static uint16_t U2(uint8_t *p)
{
    uint16_t u;
    memcpy(&u, p, 2);
    return u;
}

static float R4(uint8_t *p)
{
    float r;
    memcpy(&r, p, 4);
    return r;
}

static uint32_t U4(uint8_t *p)
{
    uint32_t u;
    memcpy(&u, p, 4);
    return u;
}

static double D8(uint8_t *p)
{
    double d;
    memcpy(&d, p, 8);
    return d;
}

/* parse the payload of a frame and feed into data section
解析帧的有效负载并将其馈送到数据部分*/
static int parse_data(hipnuc_raw_t *raw)
{
    int ofs = 0;
    uint8_t *p = &raw->buf[CH_HDR_SIZE];

    /* ignore all previous data 忽略之前的所有数据*/
    raw->hi91.tag = 0;
    raw->hi81.tag = 0;
    raw->hi83.tag = 0;

    while (ofs < raw->len)
    {
        switch (p[ofs])
        {
        case HIPNUC_ID_HI91:
            memcpy(&raw->hi91, p + ofs, sizeof(hi91_t));
            ofs += sizeof(hi91_t);
            break;
        case HIPNUC_ID_HI81:
            memcpy(&raw->hi81, p + ofs, sizeof(hi81_t));
            ofs += sizeof(hi81_t);
            break;
        case HIPNUC_ID_HI83:
        {
            raw->hi83.tag = 0x83;
            raw->hi83.main_status = U2(p + ofs + 1);
            raw->hi83.ins_status = p[ofs + 3];
            raw->hi83.data_bitmap = U4(p + ofs + 4);
            int idx = ofs + 8;
            uint32_t bm = raw->hi83.data_bitmap;

            if (bm & HI83_BMAP_ACC_B)
            {
                raw->hi83.acc_b[0] = R4(p + idx + 0);
                raw->hi83.acc_b[1] = R4(p + idx + 4);
                raw->hi83.acc_b[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_GYR_B)
            {
                raw->hi83.gyr_b[0] = R4(p + idx + 0);
                raw->hi83.gyr_b[1] = R4(p + idx + 4);
                raw->hi83.gyr_b[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_MAG_B)
            {
                raw->hi83.mag_b[0] = R4(p + idx + 0);
                raw->hi83.mag_b[1] = R4(p + idx + 4);
                raw->hi83.mag_b[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_RPY)
            {
                raw->hi83.rpy[0] = R4(p + idx + 0);
                raw->hi83.rpy[1] = R4(p + idx + 4);
                raw->hi83.rpy[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_QUAT)
            {
                raw->hi83.quat[0] = R4(p + idx + 0);
                raw->hi83.quat[1] = R4(p + idx + 4);
                raw->hi83.quat[2] = R4(p + idx + 8);
                raw->hi83.quat[3] = R4(p + idx + 12);
                idx += 16;
            }
            if (bm & HI83_BMAP_SYSTEM_TIME)
            {
                raw->hi83.system_time = U4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_UTC)
            {
                raw->hi83.utc.year = p[idx + 0];
                raw->hi83.utc.month = p[idx + 1];
                raw->hi83.utc.day = p[idx + 2];
                raw->hi83.utc.hour = p[idx + 3];
                raw->hi83.utc.min = p[idx + 4];
                raw->hi83.utc.sec_ms = U2(p + idx + 5);
                raw->hi83.utc.rev = p[idx + 7];
                idx += 8;
            }
            if (bm & HI83_BMAP_AIR_PRESSURE)
            {
                raw->hi83.air_pressure = R4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_TEMPERATURE)
            {
                raw->hi83.temperature = R4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_INCLINATION)
            {
                raw->hi83.inclination[0] = R4(p + idx + 0);
                raw->hi83.inclination[1] = R4(p + idx + 4);
                raw->hi83.inclination[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_HSS)
            {
                raw->hi83.hss[0] = R4(p + idx + 0);
                raw->hi83.hss[1] = R4(p + idx + 4);
                raw->hi83.hss[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_HSS_FRQ)
            {
                raw->hi83.hss_frq[0] = R4(p + idx + 0);
                raw->hi83.hss_frq[1] = R4(p + idx + 4);
                raw->hi83.hss_frq[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_VEL_ENU)
            {
                raw->hi83.vel_enu[0] = R4(p + idx + 0);
                raw->hi83.vel_enu[1] = R4(p + idx + 4);
                raw->hi83.vel_enu[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_ACC_ENU)
            {
                raw->hi83.acc_enu[0] = R4(p + idx + 0);
                raw->hi83.acc_enu[1] = R4(p + idx + 4);
                raw->hi83.acc_enu[2] = R4(p + idx + 8);
                idx += 12;
            }
            if (bm & HI83_BMAP_INS_LON_LAT_MSL)
            {
                raw->hi83.ins_lon_lat_msl[0] = D8(p + idx + 0);
                raw->hi83.ins_lon_lat_msl[1] = D8(p + idx + 8);
                raw->hi83.ins_lon_lat_msl[2] = D8(p + idx + 16);
                idx += 24;
            }
            if (bm & HI83_BMAP_GNSS_QUALITY_NV)
            {
                raw->hi83.solq_pos = p[idx + 0];
                raw->hi83.nv_pos = p[idx + 1];
                raw->hi83.solq_heading = p[idx + 2];
                raw->hi83.nv_heading = p[idx + 3];
                idx += 4;
            }
            if (bm & HI83_BMAP_OD_SPEED)
            {
                raw->hi83.od_speed = R4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_UNDULATION)
            {
                raw->hi83.undulation = R4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_DIFF_AGE)
            {
                raw->hi83.diff_age = R4(p + idx);
                idx += 4;
            }
            if (bm & HI83_BMAP_NODE_ID)
            {
                raw->hi83.node.node_id = p[idx + 0];
                raw->hi83.node.reserved[0] = p[idx + 1];
                raw->hi83.node.reserved[1] = p[idx + 2];
                raw->hi83.node.reserved[2] = p[idx + 3];
                idx += 4;
            }
            if (bm & HI83_BMAP_GNSS_LON_LAT_MSL)
            {
                raw->hi83.gnss_lon_lat_msl[0] = D8(p + idx + 0);
                raw->hi83.gnss_lon_lat_msl[1] = D8(p + idx + 8);
                raw->hi83.gnss_lon_lat_msl[2] = D8(p + idx + 16);
                idx += 24;
            }
            if (bm & HI83_BMAP_GNSS_VEL)
            {
                raw->hi83.gnss_vel[0] = R4(p + idx + 0);
                raw->hi83.gnss_vel[1] = R4(p + idx + 4);
                raw->hi83.gnss_vel[2] = R4(p + idx + 8);
                idx += 12;
            }

            ofs = idx;
        }
        break;
        default:
            ofs++;
            break;
        }
    }
    return 1;
}

static int decode_hipnuc(hipnuc_raw_t *raw)
{
    uint16_t crc = 0;

    /* checksum 校验和*/
    hipnuc_crc16(&crc, raw->buf, (CH_HDR_SIZE - 2));
    hipnuc_crc16(&crc, raw->buf + CH_HDR_SIZE, raw->len);
    if (crc != U2(raw->buf + (CH_HDR_SIZE - 2)))
    {
        // NL_TRACE("ch checksum error: frame:0x%X calcuate:0x%X, len:%d\n", U2(raw->buf + 4), crc, raw->len);
        return -1;
    }

    return parse_data(raw);
}

/* sync code
同步码*/
static int sync_hipnuc(uint8_t *buf, uint8_t data)
{
    buf[0] = buf[1];
    buf[1] = data;
    return buf[0] == CHSYNC1 && buf[1] == CHSYNC2;
}

/**
 * @brief     HiPNUC decoder input, read one byte at a time.
 * HiPNUC解码器输入，每次读取一个字节
 *
 * @param    raw is the decoder struct.
 * Raw是解码器结构
 * @param    data is the one byte read from stream.
 * 数据是从流中读取的一个字节。
 * @return   >0: decoder received a frame successfully, else: receiver did not receive a frame successfully.
 * 解码器成功接收了一个帧，否则：接收器未成功接收帧
 */
int hipnuc_input(hipnuc_raw_t *raw, uint8_t data)
{
    /* synchronize frame
    帧同步*/
    if (raw->nbyte == 0)
    {
        if (!sync_hipnuc(raw->buf, data))
            return 0;
        raw->nbyte = 2;
        return 0;
    }

    raw->buf[raw->nbyte++] = data;

    if (raw->nbyte == CH_HDR_SIZE)
    {
        if ((raw->len = U2(raw->buf + 2)) > (HIPNUC_MAX_RAW_SIZE - CH_HDR_SIZE))
        {
            // NL_TRACE("ch length error: len=%d\n",raw->len);
            raw->nbyte = 0;
            return -1;
        }
    }

    if (raw->nbyte < CH_HDR_SIZE || raw->nbyte < (raw->len + CH_HDR_SIZE))
    {
        return 0;
    }

    raw->nbyte = 0;

    return decode_hipnuc(raw);
}

/**
 * 没什么用
 * @brief    Convert packet to string, only dump parts of data
 * 将数据包转换为字符串，仅转储部分数据
 *
 * @param    raw is struct of decoder
 * Raw是解码器的结构
 * @param    buf is the log string buffer, make sure buf is larger than 256
 * Buf是日志字符串缓冲区，确保Buf大于256
 * @param    buf_size is the size of the log buffer
 * Buf_size是日志缓冲区的大小
 * @return   Number of characters written to the buffer
 * 写入缓冲区的字符数
 */
int hipnuc_dump_packet(hipnuc_raw_t *raw, char *buf, size_t buf_size)
{
    int written = 0;
    int ret;

    /* dump 0x91 packet 转储0x91报文*/
    if (raw->hi91.tag == HIPNUC_ID_HI91)
    {
        /* Format:
         * system_time: ms
         * acc: m/s²
         * gyr: deg/s
         * mag: uT
         * pitch/roll/yaw: deg
         * quat: w,x,y,z
         * air_pressure: Pa
         */
        ret = snprintf(buf + written, buf_size - written,
                       "{\n"
                       "  \"type\": \"HI91\",\n"
                       "  \"main_status\": [0x%X],\n"
                       "  \"system_time\": %d,\n"
                       "  \"acc\": [%.3f, %.3f, %.3f],\n"
                       "  \"gyr\": [%.3f, %.3f, %.3f],\n"
                       "  \"mag\": [%.3f, %.3f, %.3f],\n"
                       "  \"pitch\": %.2f,\n"
                       "  \"roll\": %.2f,\n"
                       "  \"yaw\": %.2f,\n"
                       "  \"quat\": [%.3f, %.3f, %.3f, %.3f],\n"
                       "  \"air_pressure\": %.1f\n"
                       "}\n",
                       raw->hi91.main_status,
                       raw->hi91.system_time,
                       raw->hi91.acc[0] * GRAVITY, raw->hi91.acc[1] * GRAVITY, raw->hi91.acc[2] * GRAVITY,
                       raw->hi91.gyr[0], raw->hi91.gyr[1], raw->hi91.gyr[2],
                       raw->hi91.mag[0], raw->hi91.mag[1], raw->hi91.mag[2],
                       raw->hi91.pitch, raw->hi91.roll, raw->hi91.yaw,
                       raw->hi91.quat[0], raw->hi91.quat[1], raw->hi91.quat[2], raw->hi91.quat[3],
                       raw->hi91.air_pressure);
    }

    /* dump 0x81 packet 转储0x81报文*/
    else if (raw->hi81.tag == HIPNUC_ID_HI81)
    {
        /* Format:
         * status: device status
         * ins_status: INS algorithm status
         * gpst_wn/tow: GPS week number and time of week
         * gyr: deg/s
         * acc: m/s²
         * mag: uT
         * air_pressure: Pa
         * temperature: °C
         * utc: YYYY-MM-DD HH:mm:ss.SSS
         * pitch/roll/yaw: deg
         * quat: w,x,y,z
         * ins_lat/lon: deg
         * ins_msl: m
         * pdop/hdop: position/horizontal dilution of precision
         * solq_pos: 0:invalid 1:SPP 2:DGPS 4:RTK-FLOAT 5:RTK-FIXED
         * nv_pos: number of satellites used for position
         * solq_heading: 0:invalid 4:valid
         * nv_heading: number of satellites used for heading
         * diff_age: differential age(s)
         * undulation: geoidal separation(m)
         * vel_enu: east,north,up velocity(m/s)
         * acc_enu: east,north,up acceleration(m/s²)
         */
        ret = snprintf(buf + written, buf_size - written,
                       "{\n"
                       "  \"type\": \"HI81\",\n"
                       "  \"main_status\": %d,\n"
                       "  \"ins_status\": %d,\n"
                       "  \"gpst_wn\": %d,\n"
                       "  \"gpst_tow\": %d,\n"
                       "  \"gyr\": [%.3f, %.3f, %.3f],\n"
                       "  \"acc\": [%.3f, %.3f, %.3f],\n"
                       "  \"mag\": [%.3f, %.3f, %.3f],\n"
                       "  \"air_pressure\": %.1f,\n"
                       "  \"temperature\": %d,\n"
                       "  \"utc\": \"20%02d-%02d-%02d %02d:%02d:%02d.%03d\",\n"
                       "  \"pitch\": %.2f,\n"
                       "  \"roll\": %.2f,\n"
                       "  \"yaw\": %.2f,\n"
                       "  \"quat\": [%.3f, %.3f, %.3f, %.3f],\n"
                       "  \"ins_lat\": %.7f,\n"
                       "  \"ins_lon\": %.7f,\n"
                       "  \"ins_msl\": %.2f,\n"
                       "  \"pdop\": %.1f,\n"
                       "  \"hdop\": %.1f,\n"
                       "  \"solq_pos\": %d,\n"
                       "  \"nv_pos\": %d,\n"
                       "  \"solq_heading\": %d,\n"
                       "  \"nv_heading\": %d,\n"
                       "  \"diff_age\": %d,\n"
                       "  \"undulation\": %.2f,\n"
                       "  \"vel_enu\": [%.2f, %.2f, %.2f],\n"
                       "  \"acc_enu\": [%.2f, %.2f, %.2f],\n"
                       "}\n",
                       raw->hi81.main_status,
                       raw->hi81.ins_status,
                       raw->hi81.gpst_wn,
                       raw->hi81.gpst_tow,
                       raw->hi81.gyr_b[0] * (0.001 * R2D), raw->hi81.gyr_b[1] * (0.001 * R2D), raw->hi81.gyr_b[2] * (0.001 * R2D),
                       raw->hi81.acc_b[0] * 0.0048828, raw->hi81.acc_b[1] * 0.0048828, raw->hi81.acc_b[2] * 0.0048828,
                       raw->hi81.mag_b[0] * 0.030517, raw->hi81.mag_b[1] * 0.030517, raw->hi81.mag_b[2] * 0.030517,
                       (float)raw->hi81.air_pressure,
                       raw->hi81.temperature,
                       raw->hi81.utc_year,
                       raw->hi81.utc_month,
                       raw->hi81.utc_day,
                       raw->hi81.utc_hour,
                       raw->hi81.utc_min,
                       raw->hi81.utc_msec / 1000,
                       raw->hi81.utc_msec % 1000,
                       raw->hi81.pitch * 0.01,
                       raw->hi81.roll * 0.01,
                       raw->hi81.yaw * 0.01,
                       raw->hi81.quat[0] * 0.0001, raw->hi81.quat[1] * 0.0001, raw->hi81.quat[2] * 0.0001, raw->hi81.quat[3] * 0.0001,
                       raw->hi81.ins_lat * 1e-7,
                       raw->hi81.ins_lon * 1e-7,
                       raw->hi81.ins_msl * 1e-3,
                       raw->hi81.pdop * 0.1,
                       raw->hi81.hdop * 0.1,
                       raw->hi81.solq_pos,
                       raw->hi81.nv_pos,
                       raw->hi81.solq_heading,
                       raw->hi81.nv_heading,
                       raw->hi81.diff_age,
                       raw->hi81.undulation * 0.01,
                       raw->hi81.vel_enu[0] * 0.01, raw->hi81.vel_enu[1] * 0.01, raw->hi81.vel_enu[2] * 0.01,
                       raw->hi81.acc_enu[0] * 0.0048828, raw->hi81.acc_enu[1] * 0.0048828, raw->hi81.acc_enu[2] * 0.0048828);
    }

    else if (raw->hi83.tag == HIPNUC_ID_HI83)
    {
        ret = snprintf(buf + written, buf_size - written,
                       "{\n"
                       "  \"type\": \"HI83\",\n"
                       "  \"main_status\": %d,\n"
                       "  \"ins_status\": %u,\n"
                       "  \"data_bitmap\": %u\n",
                       raw->hi83.main_status,
                       (unsigned)raw->hi83.ins_status,
                       (unsigned)raw->hi83.data_bitmap);
        if (ret > 0)
            written += ret;

        if (raw->hi83.data_bitmap & HI83_BMAP_ACC_B)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"acc\": [%.3f, %.3f, %.3f]\n", raw->hi83.acc_b[0], raw->hi83.acc_b[1], raw->hi83.acc_b[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_GYR_B)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"gyr\": [%.3f, %.3f, %.3f]\n", raw->hi83.gyr_b[0], raw->hi83.gyr_b[1], raw->hi83.gyr_b[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_MAG_B)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"mag\": [%.3f, %.3f, %.3f]\n", raw->hi83.mag_b[0], raw->hi83.mag_b[1], raw->hi83.mag_b[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_RPY)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"pitch\": %.2f\n  ,\"roll\": %.2f\n  ,\"yaw\": %.2f\n", raw->hi83.rpy[1], raw->hi83.rpy[0], raw->hi83.rpy[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_QUAT)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"quat\": [%.3f, %.3f, %.3f, %.3f]\n", raw->hi83.quat[0], raw->hi83.quat[1], raw->hi83.quat[2], raw->hi83.quat[3]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_SYSTEM_TIME)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"system_time\": %u\n", (unsigned)raw->hi83.system_time);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_UTC)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"utc\": \"20%02u-%02u-%02u %02u:%02u:%02u.%03u\"\n", (unsigned)raw->hi83.utc.year, (unsigned)raw->hi83.utc.month, (unsigned)raw->hi83.utc.day, (unsigned)raw->hi83.utc.hour, (unsigned)raw->hi83.utc.min, (unsigned)(raw->hi83.utc.sec_ms / 1000), (unsigned)(raw->hi83.utc.sec_ms % 1000));
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_AIR_PRESSURE)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"air_pressure\": %.1f\n", raw->hi83.air_pressure);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_TEMPERATURE)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"temperature\": %.2f\n", raw->hi83.temperature);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_INCLINATION)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"inclination\": [%.2f, %.2f, %.2f]\n", raw->hi83.inclination[0], raw->hi83.inclination[1], raw->hi83.inclination[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_HSS)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"hss\": [%.3f, %.3f, %.3f]\n", raw->hi83.hss[0], raw->hi83.hss[1], raw->hi83.hss[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_HSS_FRQ)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"hss_frq\": [%.3f, %.3f, %.3f]\n", raw->hi83.hss_frq[0], raw->hi83.hss_frq[1], raw->hi83.hss_frq[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_VEL_ENU)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"vel_enu\": [%.3f, %.3f, %.3f]\n", raw->hi83.vel_enu[0], raw->hi83.vel_enu[1], raw->hi83.vel_enu[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_ACC_ENU)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"acc_enu\": [%.3f, %.3f, %.3f]\n", raw->hi83.acc_enu[0], raw->hi83.acc_enu[1], raw->hi83.acc_enu[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_INS_LON_LAT_MSL)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"ins_lon_lat_msl\": [%.7f, %.7f, %.3f]\n", raw->hi83.ins_lon_lat_msl[0], raw->hi83.ins_lon_lat_msl[1], raw->hi83.ins_lon_lat_msl[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_GNSS_QUALITY_NV)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"solq_pos\": %u\n  ,\"nv_pos\": %u\n  ,\"solq_heading\": %u\n  ,\"nv_heading\": %u\n", (unsigned)raw->hi83.solq_pos, (unsigned)raw->hi83.nv_pos, (unsigned)raw->hi83.solq_heading, (unsigned)raw->hi83.nv_heading);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_OD_SPEED)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"od_speed\": %.3f\n", raw->hi83.od_speed);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_UNDULATION)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"undulation\": %.3f\n", raw->hi83.undulation);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_DIFF_AGE)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"diff_age\": %.3f\n", raw->hi83.diff_age);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_NODE_ID)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"node_id\": %u\n", (unsigned)raw->hi83.node.node_id);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_GNSS_LON_LAT_MSL)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"gnss_lon_lat_msl\": [%.7f, %.7f, %.3f]\n", raw->hi83.gnss_lon_lat_msl[0], raw->hi83.gnss_lon_lat_msl[1], raw->hi83.gnss_lon_lat_msl[2]);
            if (ret > 0)
                written += ret;
        }
        if (raw->hi83.data_bitmap & HI83_BMAP_GNSS_VEL)
        {
            ret = snprintf(buf + written, buf_size - written, "  ,\"gnss_vel\": [%.3f, %.3f, %.3f]\n", raw->hi83.gnss_vel[0], raw->hi83.gnss_vel[1], raw->hi83.gnss_vel[2]);
            if (ret > 0)
                written += ret;
        }

        ret = snprintf(buf + written, buf_size - written, "}\n");
        if (ret > 0)
            written += ret;
        ret = 0;
    }

    if (ret > 0)
        written += ret;
    return written;
}

/**
 * @brief    Calculate HiPNUC CRC16
 *
 * @param    inital is initial value
 * @param    buf    is input buffer pointer
 * @param    len    is length of the buffer
 */
static void hipnuc_crc16(uint16_t *inital, const uint8_t *buf, uint32_t len)
{
    uint32_t crc = *inital;
    uint32_t j;
    for (j = 0; j < len; ++j)
    {
        uint32_t i;
        uint32_t byte = buf[j];
        crc ^= byte << 8;
        for (i = 0; i < 8; ++i)
        {
            uint32_t temp = crc << 1;
            if (crc & 0x8000)
            {
                temp ^= 0x1021;
            }
            crc = temp;
        }
    }
    *inital = crc;
}
