#include "communication.h"
#include "crc.h"
uint8_t Mini_PC_info_ubuf[MINI_PC_BUF_SIZE];
uint8_t Mini_PC_tx_buf[128];
uint8_t Mini_PC_rx_buf[128];

uint8_t Nav_rx_buf[64]; 

volatile _request request;
volatile _response response;
volatile _response response_zm;
extern uint8_t uart4_recbuf[32];
volatile _type_zm   AS;              // 四元数姿态数据
volatile _SuperPower SuperPower;     // SuperPower接收数据
BulletSpeed_u bullet_speed_u;

extern uint8_t a,b,c,d,e,f,g,h,o,p;
extern union {
float f_speed;
uint16_t u16[2];
uint8_t c[4];
} Shoot_speed_u;
#include "main.h"
#include "stdio.h"

uint16_t get_crc16(const uint8_t * data, uint32_t len)
{
    return crc_dji16(data, len);
}
// 校验CRC16
bool check_crc16(const uint8_t * data, uint32_t len)
{
    // CRC16存储在倒数第2和第3字节(小端序)
    uint16_t crc16 = (data[len - 2] << 8) | data[len - 3];
    return get_crc16(data, len - 3) == crc16;
}

uint8_t cal_crc_table(uint8_t *ptr, uint8_t len)
{
  return crc_minipc8(ptr, len);
}

#if Communication_Mode == Communication_huart

void Mini_PC_UART_Init(void)
{
  __HAL_UART_ENABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_rx_buf, 320);

}
// 发送四元数姿态数据
void Mini_PC_SendData_ZM2(uint8_t aim_mode)
{
    Mini_PC_tx_buf[0] = 0x5a;  // 帧头S
    Mini_PC_tx_buf[1] = 0x53;  // 帧头A
    Mini_PC_tx_buf[2] = aim_mode;  // 模式标识
	
		Mini_PC_tx_buf[3] = (mine_flag >> 8) & 0xFF; // 高8位
		Mini_PC_tx_buf[4] = mine_flag & 0xFF;        // 低8位
    
    // 四元数q0 (w分量)
    Mini_PC_tx_buf[5]  = AS.Q_info_0.c[0];
    Mini_PC_tx_buf[6]  = AS.Q_info_0.c[1];	
    Mini_PC_tx_buf[7]  = AS.Q_info_0.c[2];
    Mini_PC_tx_buf[8]  = AS.Q_info_0.c[3];	

    // 四元数q1 (x分量)
    Mini_PC_tx_buf[9]  = AS.Q_info_1.c[0];
    Mini_PC_tx_buf[10]  = AS.Q_info_1.c[1];	
    Mini_PC_tx_buf[11]  = AS.Q_info_1.c[2];
    Mini_PC_tx_buf[12] = AS.Q_info_1.c[3];
    
    // 四元数q2 (y分量)
    Mini_PC_tx_buf[13] = AS.Q_info_2.c[0];
    Mini_PC_tx_buf[14] = AS.Q_info_2.c[1];	
    Mini_PC_tx_buf[15] = AS.Q_info_2.c[2];
    Mini_PC_tx_buf[16] = AS.Q_info_2.c[3];	

    // 四元数q3 (z分量)
    Mini_PC_tx_buf[17] = AS.Q_info_3.c[0];
    Mini_PC_tx_buf[18] = AS.Q_info_3.c[1];	
    Mini_PC_tx_buf[19] = AS.Q_info_3.c[2];
    Mini_PC_tx_buf[20] = AS.Q_info_3.c[3];

    Mini_PC_tx_buf[21] = bullet_speed_u.c[0];
    Mini_PC_tx_buf[22] = bullet_speed_u.c[1];
    Mini_PC_tx_buf[23] = bullet_speed_u.c[2];
    Mini_PC_tx_buf[24] = bullet_speed_u.c[3];
		
		Mini_PC_tx_buf[25] = 0x00;
		Mini_PC_tx_buf[26] = 0x00;
   
    // 计算并添加CRC16校验
    AS.crc_Num.typeMum = get_crc16(Mini_PC_tx_buf, 27);
    Mini_PC_tx_buf[27] = AS.crc_Num.c[0];
    Mini_PC_tx_buf[28] = AS.crc_Num.c[1];
    
    HAL_UART_Transmit_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_tx_buf, 29);		
}
// 接收SuperPower数据
void GetReceive_SP(uint8_t (*buf))
{
    // 检查帧头帧尾
    if(buf[0] == SP_HEADER && buf[28] == SP_TAIL)
    {
            SuperPower.mode = buf[1];  // 工作模式
            UnpackFloatSafe(buf, SP_OFFSET_YAW, &SuperPower.yaw);
            UnpackFloatSafe(buf, SP_OFFSET_YAW_VEL, &SuperPower.yaw_vel);
            UnpackFloatSafe(buf, SP_OFFSET_YAW_ACC, &SuperPower.yaw_acc);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH, &SuperPower.pitch);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH_VEL, &SuperPower.pitch_vel);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH_ACC, &SuperPower.pitch_acc);
            
    }
}
#elif Communication_Mode == Communication_USB_VCP

void Mini_PC_SendData()
{
  Mini_PC_tx_buf[0] = 0x38; // SOF
  Mini_PC_tx_buf[1] = 0x00; // ����

  Mini_PC_tx_buf[1] |= (request.close_PC_status << 3) & 0x08; // 1   �ػ�
  Mini_PC_tx_buf[1] |= (request.buff_status << 2) & 0x04;     // 1   ��
  Mini_PC_tx_buf[1] |= (request.adjust_camera << 1) & 0x02;   // 1    �궨���
  Mini_PC_tx_buf[1] |= (request.mine << 0) & 0x01;            //    ������

  Mini_PC_tx_buf[2] = request.shooter_speed_limit;

  Mini_PC_tx_buf[3] = request.Yaw_Angle.c[0];
  Mini_PC_tx_buf[4] = request.Yaw_Angle.c[1];
  Mini_PC_tx_buf[5] = request.Yaw_Angle.c[2];
  Mini_PC_tx_buf[6] = request.Yaw_Angle.c[3];

  Mini_PC_tx_buf[7] = request.pitch_mang.c[0];
  Mini_PC_tx_buf[8] = request.pitch_mang.c[1];
  Mini_PC_tx_buf[9] = request.pitch_mang.c[2];
  Mini_PC_tx_buf[10] = request.pitch_mang.c[3];

  Mini_PC_tx_buf[11] = cal_crc_table(Mini_PC_tx_buf, 11);
  CDC_Transmit_FS(Mini_PC_tx_buf, 12);
}

#elif Communication_Mode == Communication_USB_HID

void Mini_PC_SendData()
{

  Mini_PC_tx_buf[0] = 0x38; // SOF
  Mini_PC_tx_buf[1] = 0x00; // ����

  Mini_PC_tx_buf[1] |= (request.close_PC_status << 3) & 0x08; // 1   �ػ�
  Mini_PC_tx_buf[1] |= (request.buff_status << 2) & 0x04;     // 1   ��
  Mini_PC_tx_buf[1] |= (request.adjust_camera << 1) & 0x02;   // 1    �궨���
  Mini_PC_tx_buf[1] |= (request.mine << 0) & 0x01;            //    ������

  Mini_PC_tx_buf[2] = request.shooter_speed_limit;

  Mini_PC_tx_buf[3] = request.Yaw_Angle.c[0];
  Mini_PC_tx_buf[4] = request.Yaw_Angle.c[1];
  Mini_PC_tx_buf[5] = request.Yaw_Angle.c[2];
  Mini_PC_tx_buf[6] = request.Yaw_Angle.c[3];

  Mini_PC_tx_buf[7] = request.pitch_mang.c[0];
  Mini_PC_tx_buf[8] = request.pitch_mang.c[1];
  Mini_PC_tx_buf[9] = request.pitch_mang.c[2];
  Mini_PC_tx_buf[10] = request.pitch_mang.c[3];

  Mini_PC_tx_buf[11] = cal_crc_table(Mini_PC_tx_buf, 11);
  USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, Mini_PC_tx_buf, 12);
}

#endif
