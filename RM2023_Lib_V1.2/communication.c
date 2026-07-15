#include "communication.h"
#include "my_math.h" 
#include "RM_Lib.h" 
#include "crc.h"
uint8_t Mini_PC_info_ubuf[MINI_PC_BUF_SIZE];
uint8_t Mini_PC_tx_buf[128];
uint8_t Mini_PC_rx_buf[128];
// ������
uint8_t Nav_rx_buf[64]; // �������ջ�������
// �����շ�����
volatile _request request;
volatile _response response;
volatile _response response_zm;
extern uint8_t uart4_recbuf[32];
volatile _type_zm   AS;              // 四元数姿态数据
volatile _SuperPower SuperPower;     // SuperPower接收数据
//_request_union request_union;        // 发送数据联合体
BulletSpeed_u bullet_speed_u;

extern uint8_t a,b,c,d,e,f,g,h,o,p;
extern union {
float f_speed;
uint16_t u16[2];
uint8_t c[4];
} Shoot_speed_u;
//#include "RM_Lib.h"
#include "main.h"
#include "stdio.h"

// 计算CRC16校验值
uint16_t get_crc16(const uint8_t * data, uint32_t len)
{
    uint16_t crc16 = CRC16_INIT;
    uint8_t byte;
    uint8_t i;

    while (len--) {
        byte = *data++;
        i = (crc16 ^ byte) & 0x00ff;
        crc16 = (crc16 >> 8) ^ CRC16_TABLE[i];
    }

    return crc16;
}

// 校验CRC16
bool check_crc16(const uint8_t * data, uint32_t len)
{
    // CRC16存储在倒数第2和第3字节(小端序)
    uint16_t crc16 = (data[len - 2] << 8) | data[len - 3];
    return get_crc16(data, len - 3) == crc16;
}

// Mini_PC 旧 CRC8(算法D)已收纳进统一 crc 模块。保留原签名,仅转调 crc_minipc8。
uint8_t cal_crc_table(uint8_t *ptr, uint8_t len)
{
  return crc_minipc8(ptr, len);
}

#if Communication_Mode == Communication_huart

void Mini_PC_UART_Init(void)
{
  __HAL_UART_ENABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_rx_buf, 320);
  //	__HAL_UART_DISABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_RXNE);
  //	__HAL_UART_DISABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_TXE);
  //	__HAL_UART_DISABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_TC);
  //	__HAL_UART_DISABLE_IT(&MINI_PC_USART_HANDLE, UART_IT_ERR);
}
extern uint8_t bb;
void Mini_PC_SendData()
{
  Mini_PC_tx_buf[0] = 0x38; // SOF
  Mini_PC_tx_buf[1] = 0x00; // ����

  // Mini_PC_tx_buf[1] |= (request.deploy_flag << 4) & 0x10;
  Mini_PC_tx_buf[1] |= (request.close_PC_status << 3) & 0x08; // 1   �ػ�
  Mini_PC_tx_buf[1] |= (request.buff_status << 2) & 0x04;     // 1   ��
  Mini_PC_tx_buf[1] |= (request.adjust_camera << 1) & 0x02;   // 1    �궨���
  Mini_PC_tx_buf[1] |= (request.mine << 0) & 0x01;            //    ������

  Mini_PC_tx_buf[2] = 0x1e; // ��ʮ����   //����ʡ

  Mini_PC_tx_buf[3] = request.Yaw_Angle.c[0];
  Mini_PC_tx_buf[4] = request.Yaw_Angle.c[1];
  Mini_PC_tx_buf[5] = request.Yaw_Angle.c[2];
  Mini_PC_tx_buf[6] = request.Yaw_Angle.c[3];

  Mini_PC_tx_buf[7] = request.pitch_mang.c[0];
  Mini_PC_tx_buf[8] = request.pitch_mang.c[1];
  Mini_PC_tx_buf[9] = request.pitch_mang.c[2];
  Mini_PC_tx_buf[10] = request.pitch_mang.c[3];

  Mini_PC_tx_buf[11] = request.Pitch_Angle.c[0];
  Mini_PC_tx_buf[12] = request.Pitch_Angle.c[1];
  Mini_PC_tx_buf[13] = request.Pitch_Angle.c[2];
  Mini_PC_tx_buf[14] = request.Pitch_Angle.c[3];

  Mini_PC_tx_buf[15] = request.Yaw_Anglespeed.c[0];
  Mini_PC_tx_buf[16] = request.Yaw_Anglespeed.c[1];
  Mini_PC_tx_buf[17] = request.Yaw_Anglespeed.c[2];
  Mini_PC_tx_buf[18] = request.Yaw_Anglespeed.c[3];

  Mini_PC_tx_buf[19] = request.yaw_mang.c[0];
  Mini_PC_tx_buf[20] = request.yaw_mang.c[1];
  Mini_PC_tx_buf[21] = request.yaw_mang.c[2];
  Mini_PC_tx_buf[22] = request.yaw_mang.c[3];

//   Mini_PC_tx_buf[23] = cal_crc_table(Mini_PC_tx_buf, 23); // 16
//   HAL_UART_Transmit(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_tx_buf, 24, 0x1ffff);
HAL_UART_Transmit_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_tx_buf, 20);
  //		AAAAAAA += 0.001;
}
// ========== 同济自瞄协议发送函数 ==========

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
// 发送四元数姿态数据
void Mini_PC_SendData_ZM()
{
    Mini_PC_tx_buf[0] = 0x5a;  // 帧头S
    Mini_PC_tx_buf[1] = 0x53;  // 帧头A
    Mini_PC_tx_buf[2] = 0x01;  // 模式标识
    
    // 四元数q0 (w分量)
    Mini_PC_tx_buf[3]  = AS.Q_info_0.c[0];
    Mini_PC_tx_buf[4]  = AS.Q_info_0.c[1];	
    Mini_PC_tx_buf[5]  = AS.Q_info_0.c[2];
    Mini_PC_tx_buf[6]  = AS.Q_info_0.c[3];	

    // 四元数q1 (x分量)
    Mini_PC_tx_buf[7]  = AS.Q_info_1.c[0];
    Mini_PC_tx_buf[8]  = AS.Q_info_1.c[1];	
    Mini_PC_tx_buf[9]  = AS.Q_info_1.c[2];
    Mini_PC_tx_buf[10] = AS.Q_info_1.c[3];
    
    // 四元数q2 (y分量)
    Mini_PC_tx_buf[11] = AS.Q_info_2.c[0];
    Mini_PC_tx_buf[12] = AS.Q_info_2.c[1];	
    Mini_PC_tx_buf[13] = AS.Q_info_2.c[2];
    Mini_PC_tx_buf[14] = AS.Q_info_2.c[3];	

    // 四元数q3 (z分量)
    Mini_PC_tx_buf[15] = AS.Q_info_3.c[0];
    Mini_PC_tx_buf[16] = AS.Q_info_3.c[1];	
    Mini_PC_tx_buf[17] = AS.Q_info_3.c[2];
    Mini_PC_tx_buf[18] = AS.Q_info_3.c[3];
    
    // 保留字节(19-24)
    for(int i = 19; i < 25; i++)
    {
        Mini_PC_tx_buf[i] = 0x00;
    }
   
    // 计算并添加CRC16校验
    AS.crc_Num.typeMum = get_crc16(Mini_PC_tx_buf, 25);
    Mini_PC_tx_buf[25] = AS.crc_Num.c[0];
    Mini_PC_tx_buf[26] = AS.crc_Num.c[1];
    
    HAL_UART_Transmit_DMA(&MINI_PC_USART_HANDLE, (uint8_t *)Mini_PC_tx_buf, 27);		
}

// ========== 同济自瞄协议接收函数 ==========

// 接收SuperPower数据
void GetReceive_SP(uint8_t (*buf))
{
    // 检查帧头帧尾
    if(buf[0] == SP_HEADER && buf[28] == SP_TAIL)
    {
        // 可选:启用CRC校验
        // if(check_crc16(buf, 29))
        // {
            SuperPower.mode = buf[1];  // 工作模式
            
            // 安全解包各个浮点数数据
            UnpackFloatSafe(buf, SP_OFFSET_YAW, &SuperPower.yaw);
            UnpackFloatSafe(buf, SP_OFFSET_YAW_VEL, &SuperPower.yaw_vel);
            UnpackFloatSafe(buf, SP_OFFSET_YAW_ACC, &SuperPower.yaw_acc);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH, &SuperPower.pitch);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH_VEL, &SuperPower.pitch_vel);
            UnpackFloatSafe(buf, SP_OFFSET_PITCH_ACC, &SuperPower.pitch_acc);
            
            // 可以在这里添加更多数据处理
            // 例如:
            // - 数据范围检查
            // - 数据滤波
            // - 状态更新
        // }
    }
}

// void Mini_PC_newSendData(float pitchAngle,float YawAngle,uint8_t color,uint8_t buff)
//{
//	request_union.request_new.head = 0x38;
//
//	request_union.request_new.mine = color;
//
//	request_union.request_new.PitchAngle = pitchAngle;
//	request_union.request_new.YawAngle = YawAngle;
//
//	request_union.request_new.end = cal_crc_table(request_union.TX_buf,10);//16
//
//	HAL_UART_Transmit_DMA(&MINI_PC_USART_HANDLE,(uint8_t *)request_union.TX_buf,sizeof(request_union.TX_buf));
// }

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
extern uint32_t counter;
void getReceiveData(uint8_t(*buf))
{
	if(buf[12] == 0x56  && buf[13] == 0x44)
	{
		counter++;
    response.pitch.c[0] = buf[0];
    response.pitch.c[1] = buf[1];
    response.pitch.c[2] = buf[2];
    response.pitch.c[3] = buf[3];

    response.yaw.c[0] = buf[4];
    response.yaw.c[1] = buf[5];
    response.yaw.c[2] = buf[6];
    response.yaw.c[3] = buf[7];

    response.distance.c[0] = buf[8];
    response.distance.c[1] = buf[9];
    response.distance.c[2] = buf[10];
    response.distance.c[3] = buf[11];
		
		    a = buf[12];
		    b = buf[13];
		
			  c = buf[14];
		    d = buf[15];
				g = buf[16];
		    h = buf[17];
			  o = buf[18];
		    p = buf[19];				
				e = buf[88];
		    f = buf[90];
		
	}
	
//   if (buf[0] == 0x88 && buf[15] == 0x22)
//   {

//     response.pitch.c[0] = buf[1];
//     response.pitch.c[1] = buf[2];
//     response.pitch.c[2] = buf[3];
//     response.pitch.c[3] = buf[4];

//     response.yaw.c[0] = buf[5];
//     response.yaw.c[1] = buf[6];
//     response.yaw.c[2] = buf[7];
//     response.yaw.c[3] = buf[8];

//     response.distance.c[0] = buf[9];
//     response.distance.c[1] = buf[10];
//     response.distance.c[2] = buf[11];
//     response.distance.c[3] = buf[12];
//   }
}
void getReceiveData_ZM(uint8_t(*buf))
  {
    if (buf[0] == 0x66 && buf[15] == 0x11)
    {
      response.pitch.c[0] = buf[1];
      response.pitch.c[1] = buf[2];
      response.pitch.c[2] = buf[3];
      response.pitch.c[3] = buf[4];

      response.yaw.c[0] = buf[5];
      response.yaw.c[1] = buf[6];
      response.yaw.c[2] = buf[7];
      response.yaw.c[3] = buf[8];

      response.fly_time.c[0] = buf[9];
      response.fly_time.c[1] = buf[10];
      response.fly_time.c[2] = buf[11];
      response.fly_time.c[3] = buf[12];

      response.distance_zm = buf[13] / 10.0; // m

      response.Fire_Flag = buf[14];
    }
  }

  /************************************************Communication_KalmanFilter*****************************************************/
  /**
   *	@brief Put the following code in funtion(USARTx_IRQHandler || CDC_Receive_FS || CUSTOM_HID_OutEvent_FS) of file(stm32fxxx_it.c || usbd_cdc_if.c || usbd_custom_hid_if.c):
   **/
  /*
                  #include "my_math.h"   // USER CODE BEGIN INCLUDE


                  extern Vision_process_t Vision_process;
                  extern Kf  kalman_speedYaw1,kalman_accel1,kalman_distend1;
                  extern float lastupdate_cloud_yaw,update_cloud_yaw;	//��¼�Ӿ���������ʱ����̨���ݣ����´ν�����


                  extern float Pitch_goal,Yaw_goal;
                  extern BMI088 BMI088_Yaw,BMI088_Pitch;
                  extern uint16_t active_cnt, lost;

                  getReceiveData(Buf);//or Mini_PC_rx_buf

                  if(isnan(response.yaw.f)) response.yaw.f=0.0;
                  if(isnan(response.pitch.f)) response.pitch.f=0.0;

                  lastupdate_cloud_yaw=update_cloud_yaw;
                  update_cloud_yaw =BMI088_Yaw.sensor_data.mang.z-response.yaw.f;

                  active_cnt++;
                  if(Vision_process.eeror==1)
                  {
                          lost++;
                          active_cnt=0;
                          Vision_process.feedforwaurd_angle = 0;
                          Vision_process.predict_angle = 0;//��0Ԥ���

                          Vision_process.accel_get=0;
                          Vision_process.speed_get_last=0;
                          Vision_process.speed_get=0;
                          Vision_process.distend_get =0;
                          Vision_process.speed_get = kalman_speedYaw1.KalmanFilter(Vision_process.speed_get_last,0,0,0);
                          Vision_process.accel_get = kalman_accel1.KalmanFilter(Vision_process.accel_get,0,0,0);
                          Vision_Normal(lastupdate_cloud_yaw);
                  }
                  if(Buf[0]==0x66)//or Mini_PC_rx_buf[0]==0x66
                  {
                          if(request.zimiao_status)
                          {
                                  if(YK.yaogan.s2==YK_SW_MID||YK.yaogan.s2==YK_SW_DOWN)
                                  {
                                          if(request.buff_status)  //��
                                          {
                                                  Yaw_goal = BMI088_Yaw.sensor_data.mang.z-response.yaw.f;	//BMI088����ADXRS453����������PID�ջ��ĵ�ǰ��һ�£����ſ�ʵ�����������¶��ǣ���������
                                                  Pitch_goal = BMI088_Pitch.sensor_data.mang.y-response.pitch.f;
                                          }
                                          else //����
                                          {
                                                  if(active_cnt>150)
                                                  {
                                                          Vision_Normal(update_cloud_yaw);
                                                  }
                                                  else if(lost>100)
                                                  {
                                                          active_cnt=0;
                                                          lost=0;
                                                          Vision_process.eeror=0;
                                                  }

                                                  Yaw_goal = BMI088_Yaw.sensor_data.mang.z-response.yaw.f + Vision_process.predict_angle;//Vision_process.predict_angle��Ԥ���
                                                  Pitch_goal = BMI088_Pitch.sensor_data.mang.y-response.pitch.f;
                                          }
                                  }
                          }
                  }

  // ********************************

  //				Your code

  // ********************************
          }

  */
  /**
   *
   **/

  /***************************************************Communication_huart*********************************************************/
  /**
   *	@brief Put the following code in funtion(USARTx_IRQHandler) of file(stm32fxxx_it.c):
   **/
  /*
          uint32_t tmp_flag = 0;
          uint32_t temp;
          tmp_flag =__HAL_UART_GET_FLAG(&MINI_PC_USART_HANDLE,UART_FLAG_IDLE);
          if((tmp_flag != RESET))
          {
                  __HAL_UART_CLEAR_IDLEFLAG(&MINI_PC_USART_HANDLE);
                  temp = MINI_PC_USART_HANDLE.Instance->SR;
                  temp = MINI_PC_USART_HANDLE.Instance->DR;
                  HAL_UART_DMAStop(&MINI_PC_USART_HANDLE);
                  getReceiveData(Mini_PC_rx_buf);
                  HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE,Mini_PC_rx_buf,128);

  // ********************************

  //				Your code

  // ********************************
          }

  */
  /**
   *
   **/

  /*************************************************Communication_USB_HID********************************************************/
  /**
   *	@brief Put the following code in funtion(CUSTOM_HID_ReportDesc_FS) of file(usbd_custom_hid_if.c):
   **/
  /*

          0x05,0x8c, // USAGE_PAGE (ST Page)
          0x09,0x01, // USAGE (Demo Kit)
          0xa1,0x01, // COLLECTION (Application

          // The Input report
          0x09,0x03, // USAGE ID - Vendor defined
          0x15,0x00, // LOGICAL_MINIMUM (0)
          0x26,0x00, 0xFF, // LOGICAL_MAXIMUM (255)
          0x75,0x08, // REPORT_SIZE (8bit)
          0x95,0x40, // REPORT_COUNT (64Byte)
          0x81,0x02, // INPUT (Data,Var,Abs)

          // The Output report
          0x09,0x04, // USAGE ID - Vendor defined
          0x15,0x00, // LOGICAL_MINIMUM (0)
          0x26,0x00,0xFF, // LOGICAL_MAXIMUM (255)
          0x75,0x08, // REPORT_SIZE (8bit)
          0x95,0x40, // REPORT_COUNT (64Byte)
          0x91,0x02, // OUTPUT (Data,Var,Abs)

  // ********************************

  //				Your code

  // ********************************

  */
  /**
   *
   **/

  /**
   *	@brief Put the following code in funtion(CUSTOM_HID_OutEvent_FS) of file(usbd_custom_hid_if.c):
   **/
  /*
          unsigned char USB_Received_Count;
          uint8_t i; //�鿴�������ݳ�
          USB_Received_Count = USBD_GetRxCount( &hUsbDeviceFS,CUSTOM_HID_EPOUT_ADDR );  //��һ������USB������ڶ����������ǽ��յ�ĩ�˵�ַ��Ҫ��ȡ���͵����ݳ��ȵĻ��Ͱѵڶ���������Ϊ����ĩ�˵�ַ����
          USBD_CUSTOM_HID_HandleTypeDef   *hhid; //����һ��ָ��USBD_CUSTOM_HID_HandleTypeDef�ṹ���ָ��
          hhid = (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;//�õ�USB�������ݵĴ����ַ

          for(i=0;i<USB_Received_Count;i++)
          {
                          Mini_PC_rx_buf[i]=hhid->Report_buf[i];  //�ѽ��յ��������͵��Զ���Ļ��������棨Report_buf[i]ΪUSB�Ľ��ջ�������
          }
  // ********************************

  //				Your code

  // ********************************

  */
  /**
   *
   **/
