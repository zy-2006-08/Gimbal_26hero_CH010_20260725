/**
  ******************************************************************************
  * File Name			  : CP_System.c
  * Description			: RM裁判系统库
	* Version         : 1.2
  * Creation Date		: 2021.7.22
  ******************************************************************************
  */

#include "CP_System.h"

volatile static uint16_t CP_rx_len=0;//接收到的数据长度
volatile static uint8_t CP_FIFO=0;

uint8_t CP_rev_buf[2][CP_RX_BUF_SIZE]; //数据双缓存数组
uint8_t CP_send_buf[256];
volatile uint32_t sysTickUptime;
volatile uint32_t usTicks = 0;
uint32_t DUM_Connect_time = 0;//上下主控联系

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

void CP_Delete_Graphic(uint8_t operate_tpye,uint8_t layer) //operate_tpye参数：0为空操作，1为删除某个图层，2为删除所有
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
	CP.ext_client_custom_graphic_single_t.grapic_data_struct.end_angle=number_digit;	// bit 23-31,若绘制小数,该位为小数位数,若绘制整数,该位无效
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
	CP.ext_student_interactive_header_data_t.data_cmd_id=data_cmd_id;  // 内容ID，可在 0x0200 ~ 0x02FF 间选取
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
					break;//比赛状态数据，1Hz周期发送，发送范围：所有机器人
					case 0x0002:
						CP.ext_game_result_t.winner=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//比赛结果数据，比赛结束后发送，发送范围：所有机器人
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
					break;//比赛机器人存活数据，1Hz周期发送，发送范围：所有机器人
					case 0x0004:
						CP.ext_dart_status_t.dart_belong=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_dart_status_t.stage_remaining_time=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
					break;//飞镖发射状态，飞镖发射后发送，发送范围：所有机器人
					case 0x0101:
						CP.ext_event_data_t.event_type=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<24|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<16|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//场地事件数据，1Hz周期发送，发送范围：己方机器人
					case 0x0102:
						CP.ext_supply_projectile_action_t.supply_projectile_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_projectile_step=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_action_t.supply_projectile_num=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
					break;//补给站动作标识数据，动作触发后发送，发送范围：己方机器人
					case 0x0103:
						CP.ext_supply_projectile_booking_t.supply_projectile_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_booking_t.supply_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.ext_supply_projectile_booking_t.supply_num=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
					break;//请求补给站补弹数据，由参赛队发送，上限 10Hz。（RM对抗赛尚未开放）
					case 0x0104:
						CP.ext_referee_warning_t.level=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_referee_warning_t.foul_robot_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
					break;//裁判警告信息，警告发生后发送，发送范围：己方机器人
					case 0x0105:
						CP.ext_dart_remaining_time_t.dart_remaining_time=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//飞镖发射口倒计时，1Hz周期发送，发送范围：己方机器人
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
					break;//机器人状态数据，10Hz周期发送，发送范围：单一机器人
					case 0x0202:
						CP.ext_power_heat_data_t.chassis_volt=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.chassis_current=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						memcpy((void *)&CP.ext_power_heat_data_t.chassis_power, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_power_heat_data_t.chassis_power));
						CP.ext_power_heat_data_t.chassis_power_buffer=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id1_17mm_cooling_heat=CP_rev_buf_point[11+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[10+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id2_17mm_cooling_heat=CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_power_heat_data_t.shooter_id1_42mm_cooling_heat=CP_rev_buf_point[15+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex];
					break;//实时功率热量数据，50Hz周期发送，发送范围：单一机器人
					case 0x0203:
						memcpy((void *)&CP.ext_game_robot_pos_t.x, (void *)&CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.x));
						memcpy((void *)&CP.ext_game_robot_pos_t.y, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.y));
						memcpy((void *)&CP.ext_game_robot_pos_t.z, (void *)&CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.z));
						memcpy((void *)&CP.ext_game_robot_pos_t.yaw, (void *)&CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_game_robot_pos_t.yaw));
					break;//机器人位置数据，10Hz发送，发送范围：单一机器人
					case 0x0204:
						CP.ext_buff_t.power_rune_buff=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//机器人增益数据，1Hz周期发送，发送范围：单一机器人
					case 0x0205:
						CP.aerial_robot_energy_t.attack_time=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//空中机器人能量状态数据，10Hz周期发送，发送范围：单一机器人
					case 0x0206:
						CP.ext_robot_hurt_t.armor_id=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]&0x0f;
						CP.ext_robot_hurt_t.hurt_type=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex]>>4;
					break;//伤害状态数据，伤害发生后发送，发送范围：单一机器人
					case 0x0207:
						CP.ext_shoot_data_t.bullet_type=CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];//子弹类型
						CP.ext_shoot_data_t.shooter_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];//发射机构ID
						CP.ext_shoot_data_t.bullet_freq=CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];//子弹射频
						memcpy((void *)&CP.ext_shoot_data_t.bullet_speed, (void *)&CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_shoot_data_t.bullet_speed));
					break;//实时射击数据，子弹发射后发送，发送范围：单一机器人
					case 0x0208:
						CP.ext_bullet_remaining_t.bullet_remaining_num_17mm=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_bullet_remaining_t.bullet_remaining_num_42mm=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_bullet_remaining_t.coin_remaining_num=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
					break;//子弹剩余发射数，10Hz周期发送，发送范围：单一机器人
					case 0x0209:
						CP.ext_rfid_status_t.rfid_status=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<24|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex]<<16|CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
					break;//机器人RFID状态，1Hz周期发送，发送范围：单一机器人
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
					break;//飞镖机器人客户端指令数据，10Hz周期发送，发送范围：单一机器人
					case 0x0301:
						CP.ext_student_interactive_header_data_t.data_cmd_id=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_student_interactive_header_data_t.send_ID=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_student_interactive_header_data_t.receiver_ID=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						uint16_t temp_cnt;
						for(temp_cnt=0;temp_cnt<CP.frame_header.data_length-6;temp_cnt++) CP.robot_interactive_data_t.data[CP.frame_header.data_length-6-1-temp_cnt] = CP_rev_buf_point[temp_cnt+6+5+2+CP_rev_buf_tempindex];
					break;//机器人间交互数据，发送方触发发送，上限10Hz
					case 0x0303:
						memcpy((void *)&CP.ext_robot_command_t.target_position_x, (void *)&CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_x));
						memcpy((void *)&CP.ext_robot_command_t.target_position_y, (void *)&CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_y));
						memcpy((void *)&CP.ext_robot_command_t.target_position_z, (void *)&CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex], sizeof(CP.ext_robot_command_t.target_position_z));
						CP.ext_robot_command_t.commd_keyboard=CP_rev_buf_point[12+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_command_t.target_robot_ID=CP_rev_buf_point[14+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[13+5+2+CP_rev_buf_tempindex];
					break;//小地图下发信息，触发时发送
					case 0x0304:
						CP.ext_robot_vision_command_t.mouse_x=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.mouse_y=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.mouse_z=CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.left_button_down=CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.right_button_down=CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex];
						CP.ext_robot_vision_command_t.keyboard_value=CP_rev_buf_point[9+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[8+5+2+CP_rev_buf_tempindex];
					break;//图传遥控信息，30Hz周期发送
					case 0x0305:
						CP.ext_client_map_command_t.target_robot_ID=CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.ext_client_map_command_t.target_position_x=CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.ext_client_map_command_t.target_position_y=CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];
					break;//小地图接收信息，10Hz周期接收
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
					break;//己方机器人位置坐标
					case 0x020C:
						CP.radar_mark_data_t.mark_hero_progress 			= CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_engineer_progress 	= CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_3_progress = CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_4_progress = CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_standard_5_progress = CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.radar_mark_data_t.mark_sentry_progress 		= CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex];
					break;//对方机器人被标记进度
					case 0x0302:
						CP.custom_client_data_t.key_value = CP_rev_buf_point[1+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[0+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.x_position = CP_rev_buf_point[2+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.mouse_left = CP_rev_buf_point[3+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.y_position = CP_rev_buf_point[4+5+2+CP_rev_buf_tempindex];
						CP.custom_client_data_t.mouse_right = CP_rev_buf_point[5+5+2+CP_rev_buf_tempindex];					
						CP.custom_client_data_t.reserved = CP_rev_buf_point[7+5+2+CP_rev_buf_tempindex]<<8|CP_rev_buf_point[6+5+2+CP_rev_buf_tempindex];//保留位
					break;//自定义控制器
					case 0x0306:
						
					break;//自定义控制器模拟键鼠操作选手端。					
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

static unsigned char Get_CRC8_Check_Sum(unsigned char *pchMessage,unsigned int dwLength) 
{ 
	unsigned char ucCRC8 = 0xff;
	unsigned char ucIndex; 
	static const unsigned char CRC8_TAB[256] = { 0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83, 0xc2,
	0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41, 0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
	0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc, 0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe,
	0xa0, 0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62, 0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81,
	0x63, 0x3d, 0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff, 0x46, 0x18, 0xfa, 0xa4, 0x27,
	0x79, 0x9b, 0xc5, 0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07, 0xdb, 0x85, 0x67, 0x39,
	0xba, 0xe4, 0x06, 0x58, 0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a, 0x65, 0x3b, 0xd9,
	0x87, 0x04, 0x5a, 0xb8, 0xe6, 0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24, 0xf8, 0xa6,
	0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b, 0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9, 0x8c,
	0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
	0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92, 0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e,
	0x50, 0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52,
	0xb0, 0xee, 0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 0xf0, 0xae, 0x4c, 0x12, 0x91,
	0xcf, 0x2d, 0x73, 0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49, 0x08, 0x56, 0xb4, 0xea,
	0x69, 0x37, 0xd5, 0x8b, 0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4, 0x95, 0xcb, 0x29,
	0x77, 0xf4, 0xaa, 0x48, 0x16, 0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 0x2b, 0x75,
	0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8, 0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7, 0xb6,
	0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35}; 
	while (dwLength--) 
	{ 
		ucIndex = ucCRC8^(*pchMessage++); 
		ucCRC8 = CRC8_TAB[ucIndex]; 
	} 
	return(ucCRC8); 
}
static void Append_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength)
{
	unsigned char ucCRC = 0;
	if ((pchMessage == 0) || (dwLength <= 2)) return;
	ucCRC = Get_CRC8_Check_Sum ( (unsigned char *)pchMessage, dwLength-1);
	pchMessage[dwLength-1] = ucCRC;
}
static uint16_t Get_CRC16_Check_Sum(uint8_t *pchMessage,uint32_t dwLength) 
{ 
	uint8_t chData; 
	uint16_t wCRC = 0xffff;
	static const uint16_t wCRC_Table[256] = { 0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
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
	0x0f78 };
	if (pchMessage == NULL) 
	{ 
		return 0xFFFF; 
	} 
	while(dwLength--) 
	{ 
		chData = *pchMessage++; 
		(wCRC) = ((uint16_t)(wCRC) >> 8) ^ wCRC_Table[((uint16_t)(wCRC) ^ (uint16_t)(chData)) & 0x00ff]; 
	} 
	return wCRC; 
}
static void Append_CRC16_Check_Sum(uint8_t * pchMessage,uint32_t dwLength)
{
	uint16_t wCRC = 0;
	if ((pchMessage == NULL) || (dwLength <= 2)) return;
	wCRC = Get_CRC16_Check_Sum ( (uint8_t *)pchMessage, dwLength-2);
	pchMessage[dwLength-2] = (uint8_t)(wCRC & 0x00ff);
	pchMessage[dwLength-1] = (uint8_t)((wCRC >> 8)& 0x00ff);
}