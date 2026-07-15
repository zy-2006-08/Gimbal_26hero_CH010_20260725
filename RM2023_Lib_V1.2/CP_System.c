/**
  ******************************************************************************
  * File Name			  : CP_System.c
  * Description			: RM����ϵͳ��
	* Version         : 1.2
  * Creation Date		: 2021.7.22
  ******************************************************************************
  */

#include "CP_System.h"
#include "crc.h"

volatile static uint16_t CP_rx_len=0;//���յ������ݳ���
volatile static uint8_t CP_FIFO=0;

uint8_t CP_rev_buf[2][CP_RX_BUF_SIZE]; //����˫��������
uint8_t CP_send_buf[256];
volatile uint32_t sysTickUptime;
volatile uint32_t usTicks = 0;
uint32_t DUM_Connect_time = 0;//����������ϵ

CP_typedef CP;

void CP_System_Init()
{
	__HAL_UART_ENABLE_IT(&CP_SYSTEM_USART_HANDLE, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&CP_SYSTEM_USART_HANDLE,CP_rev_buf[0],CP_RX_BUF_SIZE);
}

void CP_System_IRQHandler(DMA_HandleTypeDef* hdma)
{
	uint32_t tmp_flag = 0;
	uint32_t temp;
	tmp_flag =__HAL_UART_GET_FLAG(&CP_SYSTEM_USART_HANDLE,UART_FLAG_IDLE); 
	if((tmp_flag != RESET))
	{ 
		__HAL_UART_CLEAR_IDLEFLAG(&CP_SYSTEM_USART_HANDLE);
		temp = CP_SYSTEM_USART_HANDLE.Instance->SR;  
		temp = CP_SYSTEM_USART_HANDLE.Instance->DR; 
		HAL_UART_DMAStop(&CP_SYSTEM_USART_HANDLE); 
		temp  = hdma->Instance->NDTR;
		CP_rx_len =  CP_RX_BUF_SIZE - temp; 
		
		if(CP_FIFO) CP_FIFO = 0;
		else CP_FIFO = 1;
		HAL_UART_Receive_DMA(&CP_SYSTEM_USART_HANDLE,CP_rev_buf[CP_FIFO],CP_RX_BUF_SIZE);
		CP_data_deal();
	}
}

void CP_System_DMACplt_DataDeal()
{
	uint16_t ulen=0;
	CP_rx_len = CP_RX_BUF_SIZE;
	if(CP_FIFO) CP_FIFO = 0;
	else CP_FIFO = 1;
	ulen = CP_data_deal();
	HAL_UART_Receive_DMA(&CP_SYSTEM_USART_HANDLE,&CP_rev_buf[CP_FIFO][ulen],CP_rx_len - ulen);
	memcpy((void *)CP_rev_buf[CP_FIFO], (void *)&CP_rev_buf[!CP_FIFO][CP_rx_len-ulen], ulen);
}

static void cycleCounterInit(void)
{
	usTicks = HAL_RCC_GetSysClockFreq() / 1000000; 
}
uint32_t micros(void)
{
	uint32_t ms, cycle_cnt;
	do {
			ms = sysTickUptime;
			cycle_cnt = SysTick->VAL;
	} while (ms != sysTickUptime);
	return (ms * 1000) + (usTicks * 1000 - cycle_cnt) / usTicks;
}

bool Judge_IF_DUM_Normal(void)
{
  bool res = true;
  if(micros() >= DUM_Connect_time)
  {
    res = false;
  }
	return res;
}

void CP_Delete_Graphic(uint8_t operate_tpye,uint8_t layer) //operate_tpye������0Ϊ�ղ�����1Ϊɾ��ĳ��ͼ�㣬2Ϊɾ������
{
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=6+2;
	CP.cmd_id=0x0301;
	CP.ext_student_interactive_header_data_t.data_cmd_id=0x0100;
	CP.ext_student_interactive_header_data_t.send_ID=Send_ID;
	CP.ext_student_interactive_header_data_t.receiver_ID=Receiver_ID;

	CP.ext_client_custom_graphic_delete_t.operate_tpye=operate_tpye;
	CP.ext_client_custom_graphic_delete_t.layer=layer;

	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=CP.ext_student_interactive_header_data_t.data_cmd_id;
	*(uint16_t*)&CP_send_buf[9]=CP.ext_student_interactive_header_data_t.send_ID;
	*(uint16_t*)&CP_send_buf[11]=CP.ext_student_interactive_header_data_t.receiver_ID;

	CP_send_buf[13]=CP.ext_client_custom_graphic_delete_t.operate_tpye;
	CP_send_buf[14]=CP.ext_client_custom_graphic_delete_t.layer;

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 17, 0xffff);
}

void CP_DrawOrDelete_One_Graphic(uint32_t graphic_name,operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye,uint8_t layer,Color_tpyedef color,
																 uint16_t start_angle,uint16_t end_angle,uint16_t width,uint16_t start_x,uint16_t start_y,
																 uint16_t radius,uint16_t end_x,uint16_t end_y)
{
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=6+15;
	CP.cmd_id=0x0301;
	CP.ext_student_interactive_header_data_t.data_cmd_id=0x0101;
	CP.ext_student_interactive_header_data_t.send_ID=Send_ID;
	CP.ext_student_interactive_header_data_t.receiver_ID=Receiver_ID;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[0]=graphic_name&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[1]=(graphic_name>>8)&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[2]=(graphic_name>>16)&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.operate_tpye=operate_tpye;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_tpye=graphic_tpye;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.layer=layer;	// bit 6-9,max=9
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.color=color;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_angle=start_angle;	// bit 14-22,max=360
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_angle=end_angle;	// bit 23-31,max=360
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.width=width;	// bit 0-9,max=1023,typical value=10
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_x=start_x;	// bit 10-20,max=2047
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_y=start_y;	// bit 21-31,max=2047
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.radius=radius;	// bit 0-9,max=1023
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_x=end_x;	// bit 10-20
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_y=end_y;	// bit 21-31
	
	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=CP.ext_student_interactive_header_data_t.data_cmd_id;
	*(uint16_t*)&CP_send_buf[9]=CP.ext_student_interactive_header_data_t.send_ID;
	*(uint16_t*)&CP_send_buf[11]=CP.ext_student_interactive_header_data_t.receiver_ID;

	CP_send_buf[13]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[0];
	CP_send_buf[14]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[1];
	CP_send_buf[15]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[2];
	
	*(uint32_t*)&CP_send_buf[16]=
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.operate_tpye|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_tpye<<3|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.layer<<6|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.color<<10|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_angle<<14|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_angle<<23;

	*(uint32_t*)&CP_send_buf[20]=
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.width|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_x<<10|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_y<<21;

	*(uint32_t*)&CP_send_buf[24]=
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.radius|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_x<<10|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_y<<21;

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 30, 0xffff);

}

void CP_DrawOrDelete_Two_Graphic(uint32_t graphic_name[2],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[2],uint8_t layer,Color_tpyedef color[2],
																 uint16_t start_angle[2],uint16_t end_angle[2],uint16_t width[2],uint16_t start_x[2],uint16_t start_y[2],
																 uint16_t radius[2],uint16_t end_x[2],uint16_t end_y[2])
{
	uint8_t j;
	
	CP_send_buf[0] = CP.frame_header.SOF = 0xA5;
	*(uint16_t*)&CP_send_buf[1] = CP.frame_header.data_length = 6+30;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5] = CP.cmd_id = 0x0301;
	*(uint16_t*)&CP_send_buf[7] = CP.ext_student_interactive_header_data_t.data_cmd_id = 0x0102;
	*(uint16_t*)&CP_send_buf[9] = CP.ext_student_interactive_header_data_t.send_ID = Send_ID;
	*(uint16_t*)&CP_send_buf[11] = CP.ext_student_interactive_header_data_t.receiver_ID = Receiver_ID;
	
	for(j=0;j<2;j++)
	{
		CP_send_buf[j*15+13] = CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].graphic_name[0] = graphic_name[j]&0xff;
		CP_send_buf[j*15+14] = CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].graphic_name[1] = (graphic_name[j]>>8)&0xff;
		CP_send_buf[j*15+15] = CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].graphic_name[2] = (graphic_name[j]>>16)&0xff;
		
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].operate_tpye=operate_tpye;
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].graphic_tpye=graphic_tpye[j];
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].layer=layer;	// bit 6-9,max=9
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].color=color[j];
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_angle=start_angle[j];	// bit 14-22,max=360
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_angle=end_angle[j];	// bit 23-31,max=360
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].width=width[j];	// bit 0-9,max=1023,typical value=10
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_x=start_x[j];	// bit 10-20,max=2047
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_y=start_y[j];	// bit 21-31,max=2047
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].radius=radius[j];	// bit 0-9,max=1023
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_x=end_x[j];	// bit 10-20
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_y=end_y[j];	// bit 21-31
		
		*(uint32_t*)&CP_send_buf[j*15+16]=
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].operate_tpye|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].graphic_tpye<<3|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].layer<<6|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].color<<10|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_angle<<14|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_angle<<23;

		*(uint32_t*)&CP_send_buf[j*15+20]=
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].width|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_x<<10|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].start_y<<21;

		*(uint32_t*)&CP_send_buf[j*15+24]=
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].radius|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_x<<10|
		CP.ext_client_custom_graphic_double_t.grapic_data_struct[j].end_y<<21;
	}

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 45, 0xffff);

}

void CP_DrawOrDelete_Five_Graphic(uint32_t graphic_name[5],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[5],uint8_t layer,Color_tpyedef color[5],
																	uint16_t start_angle[5],uint16_t end_angle[5],uint16_t width[5],uint16_t start_x[5],uint16_t start_y[5],
																	uint16_t radius[5],uint16_t end_x[5],uint16_t end_y[5])
{
	uint8_t j;
	
	CP_send_buf[0] = CP.frame_header.SOF = 0xA5;
	*(uint16_t*)&CP_send_buf[1] = CP.frame_header.data_length = 6+75;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5] = CP.cmd_id = 0x0301;
	*(uint16_t*)&CP_send_buf[7] = CP.ext_student_interactive_header_data_t.data_cmd_id = 0x0103;
	*(uint16_t*)&CP_send_buf[9] = CP.ext_student_interactive_header_data_t.send_ID = Send_ID;
	*(uint16_t*)&CP_send_buf[11] = CP.ext_student_interactive_header_data_t.receiver_ID = Receiver_ID;
	
	for(j=0;j<5;j++)
	{
		CP_send_buf[j*15+13] = CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].graphic_name[0] = graphic_name[j]&0xff;
		CP_send_buf[j*15+14] = CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].graphic_name[1] = (graphic_name[j]>>8)&0xff;
		CP_send_buf[j*15+15] = CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].graphic_name[2] = (graphic_name[j]>>16)&0xff;
		
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].operate_tpye=operate_tpye;
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].graphic_tpye=graphic_tpye[j];
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].layer=layer;	// bit 6-9,max=9
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].color=color[j];
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_angle=start_angle[j];	// bit 14-22,max=360
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_angle=end_angle[j];	// bit 23-31,max=360
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].width=width[j];	// bit 0-9,max=1023,typical value=10
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_x=start_x[j];	// bit 10-20,max=2047
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_y=start_y[j];	// bit 21-31,max=2047
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].radius=radius[j];	// bit 0-9,max=1023
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_x=end_x[j];	// bit 10-20
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_y=end_y[j];	// bit 21-31
		
		*(uint32_t*)&CP_send_buf[j*15+16]=
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].operate_tpye|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].graphic_tpye<<3|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].layer<<6|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].color<<10|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_angle<<14|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_angle<<23;

		*(uint32_t*)&CP_send_buf[j*15+20]=
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].width|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_x<<10|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].start_y<<21;

		*(uint32_t*)&CP_send_buf[j*15+24]=
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].radius|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_x<<10|
		CP.ext_client_custom_graphic_five_t.grapic_data_struct[j].end_y<<21;
	}

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 90, 0xffff);

}

void CP_DrawOrDelete_Seven_Graphic(uint32_t graphic_name[7],operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye[7],uint8_t layer,Color_tpyedef color[7],
																	 uint16_t start_angle[7],uint16_t end_angle[7],uint16_t width[7],uint16_t start_x[7],uint16_t start_y[7],
																	 uint16_t radius[7],uint16_t end_x[7],uint16_t end_y[7])
{
	uint8_t j;
	
	CP_send_buf[0] = CP.frame_header.SOF = 0xA5;
	*(uint16_t*)&CP_send_buf[1] = CP.frame_header.data_length = 6+105;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5] = CP.cmd_id = 0x0301;
	*(uint16_t*)&CP_send_buf[7] = CP.ext_student_interactive_header_data_t.data_cmd_id = 0x0104;
	*(uint16_t*)&CP_send_buf[9] = CP.ext_student_interactive_header_data_t.send_ID = Send_ID;
	*(uint16_t*)&CP_send_buf[11] = CP.ext_student_interactive_header_data_t.receiver_ID = Receiver_ID;
	
	for(j=0;j<7;j++)
	{
		CP_send_buf[j*15+13] = CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].graphic_name[0] = graphic_name[j]&0xff;
		CP_send_buf[j*15+14] = CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].graphic_name[1] = (graphic_name[j]>>8)&0xff;
		CP_send_buf[j*15+15] = CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].graphic_name[2] = (graphic_name[j]>>16)&0xff;
		
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].operate_tpye=operate_tpye;
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].graphic_tpye=graphic_tpye[j];
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].layer=layer;	// bit 6-9,max=9
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].color=color[j];
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_angle=start_angle[j];	// bit 14-22,max=360
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_angle=end_angle[j];	// bit 23-31,max=360
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].width=width[j];	// bit 0-9,max=1023,typical value=10
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_x=start_x[j];	// bit 10-20,max=2047
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_y=start_y[j];	// bit 21-31,max=2047
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].radius=radius[j];	// bit 0-9,max=1023
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_x=end_x[j];	// bit 10-20
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_y=end_y[j];	// bit 21-31
		
		*(uint32_t*)&CP_send_buf[j*15+16]=
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].operate_tpye|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].graphic_tpye<<3|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].layer<<6|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].color<<10|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_angle<<14|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_angle<<23;

		*(uint32_t*)&CP_send_buf[j*15+20]=
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].width|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_x<<10|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].start_y<<21;

		*(uint32_t*)&CP_send_buf[j*15+24]=
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].radius|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_x<<10|
		CP.ext_client_custom_graphic_seven_t.grapic_data_struct[j].end_y<<21;
	}

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 120, 0xffff);

}

void CP_DrawOrDelete_One_Number(uint32_t graphic_name,operate_tpyedef operate_tpye,graphic_tpyedef graphic_tpye,uint8_t layer,Color_tpyedef color,
																uint16_t number_size,uint16_t number_digit,uint16_t width,uint16_t start_x,uint16_t start_y,
																float number)
{
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=6+15;
	CP.cmd_id=0x0301;
	CP.ext_student_interactive_header_data_t.data_cmd_id=0x0101;
	CP.ext_student_interactive_header_data_t.send_ID=Send_ID;
	CP.ext_student_interactive_header_data_t.receiver_ID=Receiver_ID;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[0]=graphic_name&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[1]=(graphic_name>>8)&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[2]=(graphic_name>>16)&0xff;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.operate_tpye=operate_tpye;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_tpye=graphic_tpye;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.layer=layer;	// bit 6-9,max=9
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.color=color;
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_angle=number_size;	// bit 14-22,max=511,typical value=30
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_angle=number_digit;	// bit 23-31,������С��,��λΪС��λ��,����������,��λ��Ч
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.width=width;	// bit 0-9,max=1023,typical value=char_size/10
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_x=start_x;	// bit 10-20,max=2047
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_y=start_y;	// bit 21-31,max=2047

	
	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=CP.ext_student_interactive_header_data_t.data_cmd_id;
	*(uint16_t*)&CP_send_buf[9]=CP.ext_student_interactive_header_data_t.send_ID;
	*(uint16_t*)&CP_send_buf[11]=CP.ext_student_interactive_header_data_t.receiver_ID;

	CP_send_buf[13]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[0];
	CP_send_buf[14]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[1];
	CP_send_buf[15]=CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_name[2];
	
	*(uint32_t*)&CP_send_buf[16]=
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.operate_tpye|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.graphic_tpye<<3|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.layer<<6|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.color<<10|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_angle<<14|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_angle<<23;

	*(uint32_t*)&CP_send_buf[20]=
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.width|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_x<<10|
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.start_y<<21;

	if(graphic_tpye == Graphic_Int_number) *(int32_t*)&CP_send_buf[24] = number;
	else *(int32_t*)&CP_send_buf[24] = number*1000;

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 30, 0xffff);
}

void CP_DrawOrDelete_Char(uint32_t graphic_name,operate_tpyedef operate_tpye,uint8_t layer,Color_tpyedef color,
												  uint16_t char_size,uint16_t width,uint16_t start_x,uint16_t start_y,
												  uint8_t* data)
{
	uint16_t len=strlen((char*)data);
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=6+45;
	CP.cmd_id=0x0301;
	CP.ext_student_interactive_header_data_t.data_cmd_id=0x0110;
	CP.ext_student_interactive_header_data_t.send_ID=Send_ID;
	CP.ext_student_interactive_header_data_t.receiver_ID=Receiver_ID;
	CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[0]=graphic_name&0xff;
	CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[1]=(graphic_name>>8)&0xff;
	CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[2]=(graphic_name>>16)&0xff;
	CP.ext_client_custom_character_t.grapic_data_struct.operate_tpye=operate_tpye;
	CP.ext_client_custom_character_t.grapic_data_struct.graphic_tpye=Graphic_Characters;
	CP.ext_client_custom_character_t.grapic_data_struct.layer=layer;	// bit 6-9,max=9
	CP.ext_client_custom_character_t.grapic_data_struct.color=color;
	CP.ext_client_custom_character_t.grapic_data_struct.start_angle=char_size;	// bit 14-22,max=511,typical value=30
	CP.ext_client_custom_character_t.grapic_data_struct.end_angle=len;
	CP.ext_client_custom_character_t.grapic_data_struct.width=width;	// bit 0-9,max=1023,typical value=char_size/10
	CP.ext_client_custom_character_t.grapic_data_struct.start_x=start_x;	// bit 10-20,max=2047
	CP.ext_client_custom_character_t.grapic_data_struct.start_y=start_y;	// bit 21-31,max=2047
	
	memset(CP.ext_client_custom_character_t.data,0,sizeof(CP.ext_client_custom_character_t.data));
	memcpy((void *)&CP.ext_client_custom_character_t.data, (void *)data, len);
	
	
	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=CP.ext_student_interactive_header_data_t.data_cmd_id;
	*(uint16_t*)&CP_send_buf[9]=CP.ext_student_interactive_header_data_t.send_ID;
	*(uint16_t*)&CP_send_buf[11]=CP.ext_student_interactive_header_data_t.receiver_ID;

	CP_send_buf[13]=CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[0];
	CP_send_buf[14]=CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[1];
	CP_send_buf[15]=CP.ext_client_custom_character_t.grapic_data_struct.graphic_name[2];
	
	*(uint32_t*)&CP_send_buf[16]=
	CP.ext_client_custom_character_t.grapic_data_struct.operate_tpye|
	CP.ext_client_custom_character_t.grapic_data_struct.graphic_tpye<<3|
	CP.ext_client_custom_character_t.grapic_data_struct.layer<<6|
	CP.ext_client_custom_character_t.grapic_data_struct.color<<10|
	CP.ext_client_custom_character_t.grapic_data_struct.start_angle<<14|
	CP.ext_client_custom_character_t.grapic_data_struct.end_angle<<23;

	*(uint32_t*)&CP_send_buf[20]=
	CP.ext_client_custom_character_t.grapic_data_struct.width|
	CP.ext_client_custom_character_t.grapic_data_struct.start_x<<10|
	CP.ext_client_custom_character_t.grapic_data_struct.start_y<<21;

	*(uint32_t*)&CP_send_buf[24] = 0;
	
	memcpy((void *)&CP_send_buf[28], (void *)&CP.ext_client_custom_character_t.data, 30);
	
	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 60, 0xffff);

}

void CP_Robot_SendBytes(uint16_t data_cmd_id,uint16_t* data,uint8_t size)
{
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=6+size;
	CP.cmd_id=0x0301;
	CP.ext_student_interactive_header_data_t.data_cmd_id=data_cmd_id;  // ����ID������ 0x0200 ~ 0x02FF ��ѡȡ
	CP.ext_student_interactive_header_data_t.send_ID=Send_ID;
	CP.ext_student_interactive_header_data_t.receiver_ID=Receiver_ID;
	
	memcpy((void *)&CP.ext_client_custom_character_t.data, (void *)data, size);
	
	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=CP.ext_student_interactive_header_data_t.data_cmd_id;
	*(uint16_t*)&CP_send_buf[9]=CP.ext_student_interactive_header_data_t.send_ID;
	*(uint16_t*)&CP_send_buf[11]=CP.ext_student_interactive_header_data_t.receiver_ID;
	
	memcpy((void *)&CP_send_buf[13], (void *)&CP.ext_client_custom_character_t.data, size);
	
	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, CP.frame_header.data_length+9, 0xffff);
}

void CP_Robot_SendByte(uint16_t data_cmd_id,uint16_t data)
{
	CP_Robot_SendBytes(data_cmd_id,&data,1);
}

void CP_Map(Robot_ID_tpyedef target_ID,uint16_t target_x,uint16_t target_y,uint16_t target_dir)
{
	CP.frame_header.SOF=0xA5;
	CP.frame_header.data_length=14;
	CP.cmd_id=0x0303;
	
	CP_send_buf[0]=CP.frame_header.SOF;
	*(uint16_t*)&CP_send_buf[1]=CP.frame_header.data_length;
	CP_send_buf[3]=CP.frame_header.seq++;
	Append_CRC8_Check_Sum(CP_send_buf, 5);
	*(uint16_t*)&CP_send_buf[5]=CP.cmd_id;
	*(uint16_t*)&CP_send_buf[7]=target_ID;
	*(uint32_t*)&CP_send_buf[9]=target_x;
	*(uint32_t*)&CP_send_buf[13]=target_y;
	*(uint32_t*)&CP_send_buf[17]=target_dir;

	Append_CRC16_Check_Sum(CP_send_buf,5+2+CP.frame_header.data_length+2);
	
	HAL_UART_Transmit(&CP_SYSTEM_USART_HANDLE, CP_send_buf, 23, 0xffff);
}

static uint16_t CP_data_deal(void)
{
	uint16_t CP_rev_buf_tempindex=0;
	uint8_t* CP_rev_buf_point = CP_rev_buf[!CP_FIFO];
	do
	{
		if(CP_rx_len - CP_rev_buf_tempindex < 3)
		{
			return CP_rx_len-CP_rev_buf_tempindex;
		}
		if(CP_rev_buf_point[0+CP_rev_buf_tempindex] == 0xA5 && CP_rev_buf_point[4+CP_rev_buf_tempindex] == Get_CRC8_Check_Sum(&CP_rev_buf_point[0+CP_rev_buf_tempindex],4))
		{
			CP.frame_header.data_length = *(uint16_t *)(CP_rev_buf_point+1+CP_rev_buf_tempindex);
			if(CP_rx_len - CP_rev_buf_tempindex < 5+2+CP.frame_header.data_length+2) 
				return CP_rx_len - CP_rev_buf_tempindex;
			if(*(uint16_t *)(CP_rev_buf_point+5+2+CP.frame_header.data_length+CP_rev_buf_tempindex) == Get_CRC16_Check_Sum(&CP_rev_buf_point[0+CP_rev_buf_tempindex],5+2+CP.frame_header.data_length))
			{
				CP.cmd_id = *(uint16_t *)(CP_rev_buf_point + 5 + CP_rev_buf_tempindex);
				switch(CP.cmd_id)
				{
					case 0x0001:
						CP.ext_game_status_t.game_type=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]&0x0f;
						CP.ext_game_status_t.game_progress=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]>>4;
						CP.ext_game_status_t.stage_remain_time=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
					break;//����״̬���ݣ�1Hz���ڷ��ͣ����ͷ�Χ�����л�����
					case 0x0002:
						CP.ext_game_result_t.winner=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//����������ݣ������������ͣ����ͷ�Χ�����л�����
					case 0x0003:
						CP.ext_game_robot_HP_t.red_1_robot_HP=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_2_robot_HP=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_3_robot_HP=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_4_robot_HP=CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_5_robot_HP=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_7_robot_HP=CP_rev_buf_point[11+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[10+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_outpost_HP=CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.red_base_HP=CP_rev_buf_point[15+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_1_robot_HP=CP_rev_buf_point[17+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[16+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_2_robot_HP=CP_rev_buf_point[19+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[18+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_3_robot_HP=CP_rev_buf_point[21+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[20+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_4_robot_HP=CP_rev_buf_point[23+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[22+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_5_robot_HP=CP_rev_buf_point[25+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[24+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_7_robot_HP=CP_rev_buf_point[27+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[26+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_outpost_HP=CP_rev_buf_point[29+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[28+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_HP_t.blue_base_HP=CP_rev_buf_point[31+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[30+5+2+CP_rev_buf_tempindex];
					break;//���������˴�����ݣ�1Hz���ڷ��ͣ����ͷ�Χ�����л�����
					case 0x0004:
						CP.ext_dart_status_t.dart_belong=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_dart_status_t.stage_remaining_time=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
					break;//���ڷ���״̬�����ڷ�����ͣ����ͷ�Χ�����л�����
					case 0x0101:
						CP.ext_event_data_t.event_type=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<24|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<16|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//�����¼����ݣ�1Hz���ڷ��ͣ����ͷ�Χ������������
					case 0x0102:
						CP.ext_supply_projectile_action_t.supply_projectile_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_projectile_step=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_projectile_num=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
					break;//����վ������ʶ���ݣ������������ͣ����ͷ�Χ������������
					case 0x0103:
						CP.ext_supply_projectile_booking_t.supply_projectile_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_booking_t.supply_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_booking_t.supply_num=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
					break;//���󲹸�վ�������ݣ��ɲ����ӷ��ͣ����� 10Hz����RM�Կ�����δ���ţ�
					case 0x0104:
						CP.ext_referee_warning_t.level=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_referee_warning_t.foul_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
					break;//���о�����Ϣ�����淢�����ͣ����ͷ�Χ������������
					case 0x0105:
						CP.ext_dart_remaining_time_t.dart_remaining_time=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//���ڷ���ڵ���ʱ��1Hz���ڷ��ͣ����ͷ�Χ������������
					case 0x0201:
						CP.ext_game_robot_status_t.robot_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.robot_level=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.remain_HP=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.max_HP=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id1_17mm_cooling_rate=CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id1_17mm_cooling_limit=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id1_17mm_speed_limit=CP_rev_buf_point[11+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[10+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id2_17mm_cooling_rate=CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id2_17mm_cooling_limit=CP_rev_buf_point[15+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex]; 
						CP.ext_game_robot_status_t.shooter_id2_17mm_speed_limit=CP_rev_buf_point[17+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[16+5+2+CP_rev_buf_tempindex]; 
						CP.ext_game_robot_status_t.shooter_id1_42mm_cooling_rate=CP_rev_buf_point[19+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[18+5+2+CP_rev_buf_tempindex];
					  CP.ext_game_robot_status_t.shooter_id1_42mm_cooling_limit=CP_rev_buf_point[21+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[20+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.shooter_id1_42mm_speed_limit=CP_rev_buf_point[23+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[22+5+2+CP_rev_buf_tempindex];
						CP.ext_game_robot_status_t.chassis_power_limit=CP_rev_buf_point[25+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[24+5+2+CP_rev_buf_tempindex]; 
						CP.ext_game_robot_status_t.mains_power_gimbal_output=CP_rev_buf_point[26+5+2+CP_rev_buf_tempindex]&0x01;
						CP.ext_game_robot_status_t.mains_power_chassis_output=(CP_rev_buf_point[26+5+2+CP_rev_buf_tempindex]&0x02)>>1;
						CP.ext_game_robot_status_t.mains_power_shooter_output=(CP_rev_buf_point[26+5+2+CP_rev_buf_tempindex]&0x04)>>2;
						DUM_Connect_time = micros() + 1000000;
					break;//������״̬���ݣ�10Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0202:
						CP.ext_power_heat_data_t.chassis_volt=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.chassis_current=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						memcpy((void *)&CP.ext_power_heat_data_t.chassis_power, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_power_heat_data_t.chassis_power));
						CP.ext_power_heat_data_t.chassis_power_buffer=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id1_17mm_cooling_heat=CP_rev_buf_point[11+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[10+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id2_17mm_cooling_heat=CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id1_42mm_cooling_heat=CP_rev_buf_point[15+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex];
					break;//ʵʱ�����������ݣ�50Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0203:
						memcpy((void *)&CP.ext_game_robot_pos_t.x, (void *)&CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.x));
						memcpy((void *)&CP.ext_game_robot_pos_t.y, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.y));
						memcpy((void *)&CP.ext_game_robot_pos_t.z, (void *)&CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.z));
						memcpy((void *)&CP.ext_game_robot_pos_t.yaw, (void *)&CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.yaw));
					break;//������λ�����ݣ�10Hz���ͣ����ͷ�Χ����һ������
					case 0x0204:
						CP.ext_buff_t.power_rune_buff=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//�������������ݣ�1Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0205:
						CP.aerial_robot_energy_t.attack_time=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//���л���������״̬���ݣ�10Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0206:
						CP.ext_robot_hurt_t.armor_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]&0x0f;
						CP.ext_robot_hurt_t.hurt_type=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]>>4;
					break;//�˺�״̬���ݣ��˺��������ͣ����ͷ�Χ����һ������
					case 0x0207:
						CP.ext_shoot_data_t.bullet_type=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];//�ӵ�����
						CP.ext_shoot_data_t.shooter_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];//�������ID
						CP.ext_shoot_data_t.bullet_freq=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];//�ӵ���Ƶ
						memcpy((void *)&CP.ext_shoot_data_t.bullet_speed, (void *)&CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_shoot_data_t.bullet_speed));
					break;//ʵʱ������ݣ��ӵ�������ͣ����ͷ�Χ����һ������
					case 0x0208:
						CP.ext_bullet_remaining_t.bullet_remaining_num_17mm=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_bullet_remaining_t.bullet_remaining_num_42mm=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_bullet_remaining_t.coin_remaining_num=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
					break;//�ӵ�ʣ�෢������10Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0209:
						CP.ext_rfid_status_t.rfid_status=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<24|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<16|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//������RFID״̬��1Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x020A:
						CP.ext_dart_client_cmd_t.dart_launch_opening_status=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_dart_client_cmd_t.dart_attack_target=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_dart_client_cmd_t.target_change_time=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_dart_client_cmd_t.operate_launch_cmd_time=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
					
//						CP.ext_dart_client_cmd_t.first_dart_speed=CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
//						CP.ext_dart_client_cmd_t.second_dart_speed=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex];
//						CP.ext_dart_client_cmd_t.third_dart_speed=CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
//						CP.ext_dart_client_cmd_t.fourth_dart_speed=CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex];
//						CP.ext_dart_client_cmd_t.last_dart_launch_time=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
					break;//���ڻ����˿ͻ���ָ�����ݣ�10Hz���ڷ��ͣ����ͷ�Χ����һ������
					case 0x0301:
						CP.ext_student_interactive_header_data_t.data_cmd_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_student_interactive_header_data_t.send_ID=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_student_interactive_header_data_t.receiver_ID=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						uint16_t temp_cnt;
						for(temp_cnt=0;temp_cnt<CP.frame_header.data_length-6;temp_cnt++) CP.robot_interactive_data_t.data[CP.frame_header.data_length-6-1-temp_cnt] = CP_rev_buf_point[temp_cnt+6+5+2+CP_rev_buf_tempindex];
					break;//�����˼佻�����ݣ����ͷ��������ͣ�����10Hz
					case 0x0303:
						memcpy((void *)&CP.ext_robot_command_t.target_position_x, (void *)&CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_x));
						memcpy((void *)&CP.ext_robot_command_t.target_position_y, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_y));
						memcpy((void *)&CP.ext_robot_command_t.target_position_z, (void *)&CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_z));
						CP.ext_robot_command_t.commd_keyboard=CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_command_t.target_robot_ID=CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex];
					break;//С��ͼ�·���Ϣ������ʱ����
					case 0x0304:
						CP.ext_robot_vision_command_t.mouse_x=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.mouse_y=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.mouse_z=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.left_button_down=CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.right_button_down=CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.keyboard_value=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
					break;//ͼ��ң����Ϣ��30Hz���ڷ���
					case 0x0305:
						CP.ext_client_map_command_t.target_robot_ID=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_client_map_command_t.target_position_x=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_client_map_command_t.target_position_y=CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
					break;//С��ͼ������Ϣ��10Hz���ڽ���
					case 0x020B:
						memcpy((void *)&CP.ground_robot_position_t.hero_x, (void *)&CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.hero_x));
						memcpy((void *)&CP.ground_robot_position_t.hero_y, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.hero_y));
						memcpy((void *)&CP.ground_robot_position_t.engineer_x, (void *)&CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.engineer_x));
						memcpy((void *)&CP.ground_robot_position_t.engineer_y, (void *)&CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.engineer_y));
						memcpy((void *)&CP.ground_robot_position_t.standard_3_x, (void *)&CP_rev_buf_point[16+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_3_x));
						memcpy((void *)&CP.ground_robot_position_t.standard_3_y, (void *)&CP_rev_buf_point[20+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_3_y));
						memcpy((void *)&CP.ground_robot_position_t.standard_4_x, (void *)&CP_rev_buf_point[24+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_4_x));
						memcpy((void *)&CP.ground_robot_position_t.standard_4_y, (void *)&CP_rev_buf_point[28+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_4_y));
						memcpy((void *)&CP.ground_robot_position_t.standard_5_x, (void *)&CP_rev_buf_point[32+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_5_x));
						memcpy((void *)&CP.ground_robot_position_t.standard_5_y, (void *)&CP_rev_buf_point[36+5+2+CP_rev_buf_tempindex], sizeof(CP.ground_robot_position_t.standard_5_y));
					break;//����������λ������
					case 0x020C:
						CP.radar_mark_data_t.mark_hero_progress 			= CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_engineer_progress 	= CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_3_progress = CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_4_progress = CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_5_progress = CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_sentry_progress 		= CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex];
					break;//�Է������˱���ǽ���
					case 0x0302:
						CP.custom_client_data_t.key_value = CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.x_position = CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.mouse_left = CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.y_position = CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.mouse_right = CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex];					
						CP.custom_client_data_t.reserved = CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];//����λ
					break;//�Զ��������
					case 0x0306:
						
					break;//�Զ��������ģ��������ѡ�ֶˡ�					
					case 0x0307:
						CP.map_sentry_data_t.intention = CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.map_sentry_data_t.start_position_x = CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.map_sentry_data_t.start_position_y = CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
						memcpy((void *)&CP.map_sentry_data_t.delta_x, (void *)&CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex], sizeof(CP.map_sentry_data_t.delta_x));
						memcpy((void *)&CP.map_sentry_data_t.delta_y, (void *)&CP_rev_buf_point[54+5+2+CP_rev_buf_tempindex], sizeof(CP.map_sentry_data_t.delta_y));
					break;
					default :break;
				}
				CP_rev_buf_tempindex += 5 + 2 + CP.frame_header.data_length + 2;
			}
			else 
				CP_rev_buf_tempindex++;
		}
		else CP_rev_buf_tempindex++;
	}
	while(CP_rev_buf_tempindex < CP_rx_len);
	return 0;
}

// DJI-CRC8(算法C)已收纳进统一 crc 模块。保留原签名,仅转调 crc_dji8。
static unsigned char Get_CRC8_Check_Sum(unsigned char *pchMessage,unsigned int dwLength) 
{ 
	return crc_dji8((const uint8_t *)pchMessage, (uint32_t)dwLength); 
}
static void Append_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength)
{
	unsigned char ucCRC = 0;
	if ((pchMessage == 0) || (dwLength <= 2)) return;
	ucCRC = Get_CRC8_Check_Sum ( (unsigned char *)pchMessage, dwLength-1);
	pchMessage[dwLength-1] = ucCRC;
}
// DJI-CRC16(算法B)已收纳进统一 crc 模块。保留原签名与 NULL→0xFFFF 语义,仅转调 crc_dji16。
static uint16_t Get_CRC16_Check_Sum(uint8_t *pchMessage,uint32_t dwLength) 
{ 
	return crc_dji16(pchMessage, dwLength); 
}
static void Append_CRC16_Check_Sum(uint8_t * pchMessage,uint32_t dwLength)
{
	uint16_t wCRC = 0;
	if ((pchMessage == NULL) || (dwLength <= 2)) return;
	wCRC = Get_CRC16_Check_Sum ( (uint8_t *)pchMessage, dwLength-2);
	pchMessage[dwLength-2] = (uint8_t)(wCRC & 0x00ff);
	pchMessage[dwLength-1] = (uint8_t)((wCRC >> 8)& 0x00ff);
}