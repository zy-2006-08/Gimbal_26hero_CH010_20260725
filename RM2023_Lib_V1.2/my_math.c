#include "my_math.h"
#include "communication.h"
#include "stm32f4xx_hal.h"

void Filter_IIRLPF(float in, float *out, float LpfAttFactor)
{
	float res ;
	*out = *out + LpfAttFactor*(in - *out);	
}

extern Kf  kalman_speedYaw1,kalman_accel1,kalman_distend1;

float RampFloat(float final, float now, float ramp)//�𽥵���һ��ֵ��now���ӽ�Ŀ��ֵ��final���������Ƶ���������
{
	float buffer = 0;
	
	buffer = final - now;
	if(abs(now-final)>abs(ramp))
	{
		if(now< final)
		{
			buffer = now+ramp;
		}
		else
		{
			buffer = now-ramp;
		}
	}
	else
	{
		buffer = final;
	}

	return buffer;	
}

//�����˲�
float Slope(float M ,float *queue ,uint16_t len)
{
	float sum=0;
	float res=0;
  
		//����������FIFO��
		for(uint16_t i=0;i<len-1;i++)
		{
			queue[i] = queue[i+1];
			//���¶���
		}
		queue[len-1] = M;
    
	//���������
	for(uint16_t j=0;j<len;j++)
	{
		sum+=queue[j];
	}
	res = sum/(len);
	
	return res;
}

uint32_t Vision_get_interval(uint8_t lost_flag)//��ȡ��֮֡���ʱ��������λ�����룩
{
	uint32_t time_ms;
	uint32_t now_ms, last_ms;
	now_ms =	HAL_GetTick();//��ȡ����һ֡�ļ��ʱ��
	if(lost_flag >= 1)
	{
		last_ms = now_ms;
		time_ms = 7;
	}
	else
	{
		time_ms = now_ms-last_ms;
		last_ms = now_ms;
	}
	
	return time_ms;
}

float get_auto_speed(moving_Average_Filter *AUTO_MF, float angle)
{
	float speed;

	if((angle-(AUTO_MF->aver_num))>GIMBAL_YAW_CIRCULAR_STEP/2 )
	{
			angle-=GIMBAL_YAW_CIRCULAR_STEP;
	}
	else if((angle-(AUTO_MF->aver_num))<-(GIMBAL_YAW_CIRCULAR_STEP/2))
	{
			angle+=GIMBAL_YAW_CIRCULAR_STEP;
	}

	//last_angle = Vision_y_angle.aver_num;
	//average_add(AUTO_MF, angle);

	speed = (-(AUTO_MF->aver_num) + angle);


    return speed;
}

void kalmanCreate(KFP *kfp,float T_Q,float T_R)
{
	  kfp-> LastP=0;//�ϴι���Э���� ��ʼ��ֵΪ0.02
    kfp-> Now_P=0;//��ǰ����Э���� ��ʼ��ֵΪ0
    kfp-> out=0;//�������˲������ ��ʼ��ֵΪ0
    kfp-> Kg=0;//���������� ��ʼ��ֵΪ0
    kfp-> Q=T_Q;//��������Э���� ��ʼ��ֵΪ0.005
    kfp-> R=T_R;//�۲�����Э���� ��ʼ��ֵΪ0.543
}

float kalmanFilter(KFP *kfp,float input)
{
     //Ԥ��Э����̣�kʱ��ϵͳ����Э���� = k-1ʱ�̵�ϵͳЭ���� + ��������Э����
     kfp->Now_P = kfp->LastP + kfp->Q;
     //���������淽�̣����������� = kʱ��ϵͳ����Э���� / ��kʱ��ϵͳ����Э���� + �۲�����Э���
     kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
     //��������ֵ���̣�kʱ��״̬����������ֵ = ״̬������Ԥ��ֵ + ���������� * ������ֵ - ״̬������Ԥ��ֵ��
     kfp->out = kfp->out + kfp->Kg * (input -kfp->out);//��Ϊ��һ�ε�Ԥ��ֵ������һ�ε����ֵ
     //����Э�����: ���ε�ϵͳЭ����� kfp->LastP ����һ������׼����
     kfp->LastP = (1-kfp->Kg) * kfp->Now_P;
     return kfp->out;
}

// ������
 void KalmanCreate(extKalman_t *p,float T_Q,float T_R)
{
    p->X_last = (float)0;
    p->P_last = 0;
    p->Q = T_Q;
    p->R = T_R;
    p->A = 1;
    p->B = 0;
    p->H = 1;
    p->X_mid = p->X_last;
}
 float KalmanFilter(extKalman_t *p,float dat)
{
    p->X_mid =p->A*p->X_last;                     //�ٶȶ�Ӧ��ʽ(1)    x(k|k-1) = A*X(k-1|k-1)+B*U(k)+W(K)     ״̬����
    p->P_mid = p->A*p->P_last+p->Q;               //�ٶȶ�Ӧ��ʽ(2)    p(k|k-1) = A*p(k-1|k-1)*A'+Q            �۲ⷽ��
		p->kg = p->P_mid/(p->P_mid+p->R);             //�ٶȶ�Ӧ��ʽ(4)    kg(k) = p(k|k-1)*H'/(H*p(k|k-1)*H'+R)   ���¿���������
		p->X_now = p->X_mid + p->kg*(dat-p->X_mid);   //�ٶȶ�Ӧ��ʽ(3)    x(k|k) = X(k|k-1)+kg(k)*(Z(k)-H*X(k|k-1))  ��������ֵ
    p->P_now = (1-p->kg)*p->P_mid;                //�ٶȶ�Ӧ��ʽ(5)    p(k|k) = (I-kg(k)*H)*P(k|k-1)           ���º������Э����
    p->P_last = p->P_now;                         //״̬����
    p->X_last = p->X_now;
		
    return p->X_now;							  //���Ԥ����x(k|k)
}

void KalmanClear(extKalman_t *p)
{
    p->X_last = (float)0;
    p->P_last = 0;
    p->A = 1;
		p->B = 0;
    p->H = 1;
    p->X_mid = p->X_last;
}

/*******************************************Ԥ��*******************************/
/**
  * @brief �������˲�����
  * @other 	Q������������Q���󣬶�̬��Ӧ��죬�����ȶ��Ա仵
	R������������R���󣬶�̬��Ӧ�����������ȶ��Ա��
  */
// Kf::KalmanFilter(class Kf 成员)实现已移至 RM_Lib.cpp(去 C++ 化:my_math.c 回归纯 C)。

float Get_Diff(uint8_t queue_len, QueueObj *Data,float add_data)//���������������ƽ��ֵ֮��Ĳ��죬�����䷵����Ϊ���
{
    Data->queueTotal-=Data->queue[Data->nowLength];
    Data->queueTotal+=add_data;

		
    Data->queue[Data->nowLength]=add_data;
	
    Data->nowLength++;
	
    if(Data->full_flag==0)//��ʼ����δ��
    {
        Data->aver_num=Data->queueTotal/Data->nowLength;
    }
		else if(Data->full_flag == 1)
	  {
	    Data->aver_num=(Data->queueTotal)/queue_len;			
	  }
    if(Data->nowLength>=queue_len)
    {
        Data->nowLength=0;
        Data->full_flag=1;
    }

    Data->Diff=add_data - Data->aver_num;			
    return Data->Diff;
}

/**
* @brief ��ն���
* @param void
* @return void
*	�Զ��е��߼�
*/
void Clear_Queue(QueueObj* queue)
{
    for(uint16_t i=0; i<60; i++)
    {
      queue->queue[i]=0;
    }
    queue->nowLength = 0;
    queue->queueTotal = 0;
    queue->aver_num=0;
    queue->Diff=0;
    queue->full_flag=0;
}

void  Vision_Normal(float angle)
{
	static float acc_use = 1.f;  
  static float predic_use =2.f;//2
  float dir_factor=1.5f;

	Vision_process.rx_time_now=HAL_GetTick();
	Vision_process.rx_time_fps=Vision_process.rx_time_now-Vision_process.rx_time_prev;
	Vision_process.rx_time_prev=Vision_process.rx_time_now;
	
	Vision_process.speed_get_last = Get_Diff(3,&Vision_process.speed_queue,angle);//yaw���ٶ�
	Vision_process.speed_get_last = 40.0*(Vision_process.speed_get_last/Vision_process.rx_time_fps); //ÿ����
	Vision_process.speed_get = kalman_speedYaw1.KalmanFilter(Vision_process.speed_get_last,0.001,11,0);
  Vision_process.speed_get = LIMIT(Vision_process.speed_get , -2.0 , 2.0);
		
	Vision_process.accel_get = Get_Diff(3,&Vision_process.accel_queue,Vision_process.speed_get);//yaw�Ǽ��ٶ�
  Vision_process.accel_get = 50.0*(Vision_process.accel_get/Vision_process.rx_time_fps);//ÿ����
  Vision_process.accel_get = kalman_accel1.KalmanFilter(Vision_process.accel_get,0.001,11,0);
	Vision_process.accel_get = LIMIT(Vision_process.accel_get , -1.0 , 1.0);
	
	if(isnan(Vision_process.speed_get)||isnan(Vision_process.accel_get)) //����
	{
			LPF_Clear(&vision_speedY_LPF);
			average_clear(&vision_angleY_MF);
			average_clear(&vision_speedY_MF);
			KalmanClear(&vision_absolute_Yaw_Kal);
			KalmanClear(&vision_angleY_KF);
			KalmanClear(&vision_speedY_KF);	
		
		Clear_Queue(&Vision_process.speed_queue);
    Clear_Queue(&Vision_process.accel_queue);
		Vision_process.eeror=1;
		Vision_process.feedforwaurd_angle = 0;
    Vision_process.predict_angle = 0;//��0Ԥ���
		Vision_process.speed_get_last=0;
		Vision_process.accel_get=0;
		Vision_process.speed_get=0;
	}	
	else if((abs(Vision_process.speed_get)==2.0)&&(abs(Vision_process.accel_get)==1.0)&&(abs(Vision_process.predict_angle)==Vision_process.predict_angle_limt))//����Χ
	{				
			LPF_Clear(&vision_speedY_LPF);
			average_clear(&vision_angleY_MF);
			average_clear(&vision_speedY_MF);
			KalmanClear(&vision_absolute_Yaw_Kal);
			KalmanClear(&vision_angleY_KF);
			KalmanClear(&vision_speedY_KF);	
		
		Clear_Queue(&Vision_process.speed_queue);
    Clear_Queue(&Vision_process.accel_queue);
		Vision_process.eeror=1;
		Vision_process.feedforwaurd_angle = 0;
    Vision_process.predict_angle = 0;//��0Ԥ���
		Vision_process.speed_get_last=0;
		Vision_process.accel_get=0;
		Vision_process.speed_get=0;
	}
	else
	{
		if(request.shooter_speed_limit==30)
		{
			Vision_process.predict_angle_limt=3.8;
		}
		else
		{
			Vision_process.predict_angle_limt=5.0;//5.0
		}			
	
		judge_stop();
		//�б�������ע�͵���
//		Vision_process.feedforwaurd_angle = acc_use * Vision_process.accel_get;
//		Vision_process.predict_angle = RampFloat(Vision_process.predict_angle, predic_use * (1.2f*Vision_process.speed_get*response.distance+1.2f*dir_factor*Vision_process.feedforwaurd_angle*response.distance), 0.3);
//		Vision_process.predict_angle = predic_use * (1.2f*Vision_process.speed_get*response.distance+1.2f*dir_factor*Vision_process.feedforwaurd_angle*response.distance) ;//�ٶ�1.1�����ٶ�3
//		Vision_process.predict_angle = LIMIT(Vision_process.predict_angle , -Vision_process.predict_angle_limt, Vision_process.predict_angle_limt);
	}
}

void judge_stop()
{
	uint16_t frist_time,second_time;
	if((Vision_process.speed_get * Vision_process.accel_get)<0.0f)//��ֹ
	{
		frist_time = HAL_GetTick();
		if(frist_time-second_time<20)
		{
			return;
		}
		Vision_process.feedforwaurd_angle = 0;
    Vision_process.predict_angle = 0;
		Vision_process.speed_get_last=0;
		Vision_process.accel_get=0;
		Vision_process.speed_get=0;
		second_time=frist_time;
	}
}
//******************************************************************************************************************************************************************//
void Predict_anti_top_get_circle(void)
{
	static uint32_t Anti_top_tick;
	uint32_t tick_ms, temp;

	temp = HAL_GetTick();
	
	tick_ms = (int)(temp - Anti_top_tick);
	
	if(tick_ms<30)//ms��С���ݴ��忴�������ڣ�ȥ��
	{
		return;
	}
		
	TOP_Data.top_circle = tick_ms;
	Anti_top_tick = temp;
}

int8_t Predict_Anti_Top_Binary_judge(float yaw_angle)
{
	if(yaw_angle>TOP_Data.binary_high)
	{
		return 1;
	}
	else if(yaw_angle>=TOP_Data.binary_low && yaw_angle<=TOP_Data.binary_high)
	{
		return 0;
	}
	else if(yaw_angle<TOP_Data.binary_low)
	{
		return -1;
	}
	else 
	{
		return 0;
	}
	
	return 0;
}

void Predict_anti_top_get_binary(float binary_l, float binary_h)//ͨ������Ĳ�������TOP_Data�ṹ����������Ʒ�Χ��صı���ֵ���������ޡ����޺��м�ֵ����Щֵ���ں�����Ԥ��ʹ�������
{
	TOP_Data.binary_low = binary_l;
	TOP_Data.binary_high = binary_h;
	TOP_Data.top_mid = (binary_l+binary_h)/2;
}

void Predict_anti_top_get_speed(void)
{
	TOP_Data.top_speed = (TOP_Data.binary_high+TOP_Data.binary_low)/TOP_Data.top_circle;//�������ٶȼ���
}

void Predict_Anti_Top_binary_update(float now_yaw)
{
	TOP_Data.flag = Predict_Anti_Top_Binary_judge(now_yaw);

	if(TOP_Data.flag1==1)
	{
		TOP_Data.binary_err = now_yaw-TOP_Data.binary_high;
		TOP_Data.binary_high =  TOP_Data.binary_high+TOP_Data.binary_err;
		TOP_Data.binary_low = TOP_Data.binary_low+TOP_Data.binary_err;
		TOP_Data.top_mid = (TOP_Data.binary_high+TOP_Data.binary_low)/2;
	}
	else if(TOP_Data.flag1==-1)
	{
		TOP_Data.binary_err = now_yaw-TOP_Data.binary_low;
		TOP_Data.binary_high =  TOP_Data.binary_high+TOP_Data.binary_err;
		TOP_Data.binary_low = TOP_Data.binary_low+TOP_Data.binary_err;
		TOP_Data.top_mid = (TOP_Data.binary_high+TOP_Data.binary_low)/2;
	}
}

void Predict_Anti_Top_Cal_all(float binary_first, float binary_second)//shang zhe
{
	float binary_l, binary_h;
	
	if(fabs(binary_first-binary_second)<0.01f)//����߽����̫���������𵴴���
	{
		return;
	}
	
	if(binary_first-binary_second > 0.0f) TOP_Data.flag1=1;
	else if(binary_first-binary_second < 0.0f) TOP_Data.flag1=-1;
	
	//��ȡ��������
	Predict_anti_top_get_circle();
	
	binary_h = RP_MAX(binary_first, binary_second);
	binary_l = RP_MIN(binary_first, binary_second);
	

	Predict_anti_top_get_binary(binary_l, binary_h);
	Predict_anti_top_get_speed();
	
	TOP_Data.cnt = 0;
}

void Predict_Anti_Top_Data_Clear(void)
{
	TOP_Data.binary_err=0;
	TOP_Data.binary_high = 0;
	TOP_Data.binary_low = 0;
	TOP_Data.top_circle = 1500;
	TOP_Data.top_speed = 0;
	TOP_Data.top_mid = 0;
	TOP_Data.Anti_top_flag=0;
	TOP_Data.flag=0;
}

void Predict_Anti_Top_Data_Heart_beat(void)
{
	TOP_Data.cnt++;
	if(TOP_Data.cnt>TOP_Data.cnt_max)
	{
		Predict_Anti_Top_Data_Clear();
	}
	else
	{
		TOP_Data.Anti_top_flag=1;
	}
}

float Predict_Anti_Top_Judge_Yaw(float auto_yaw)
{
	float final_yaw;
	
	if(auto_yaw>TOP_Data.binary_high)
	{
		final_yaw = auto_yaw- TOP_Data.binary_low + TOP_Data.binary_high;
	}
	else if(auto_yaw>=TOP_Data.binary_low && auto_yaw<=TOP_Data.binary_high)
	{
		final_yaw = auto_yaw;
	}
	else if(auto_yaw<TOP_Data.binary_low)
	{
		final_yaw = auto_yaw  -TOP_Data.binary_high+TOP_Data.binary_low;
	}
	else 
	{
		final_yaw = auto_yaw;
	}
	
	return final_yaw;
}
	
	
/*********************************************************************************************************************/


#define Length   10
float buffer_num[Length];
float now_num;
float sum; 						/*<! ���Ⱥ����ֺ� */
int flag,where_num;

void	MeanFilter_Init()
{
		static_assert((Length>0)&&(Length<101),"MedianFilter Length [1,100]");
		for(int x = 0 ; x < Length; x++) buffer_num[x] = 0;
		flag = Length;
		where_num = 0;
		sum = 0;
} 	
	
float MeanFilter(float num)
{
   now_num = num;
   sum -= buffer_num[where_num];			  /*<! sum��ȥ��ֵ */
   sum += num;													/*<! sum������ֵ */
   buffer_num[where_num++] = num;
   flag > 0? flag-- : 0;								/*<!flag=LengthȻ��ݼ���֤�����ڶ�����Ч��ֵ */
   where_num %= Length; 
    
   if(flag>0)
     return now_num;
   else
     return (sum/Length);
}
		
		
float DeathZoom(float input, float center, float death)
{
	if(abs(input - center) < death)
		return center;
	return input;
}


/**
  * @brief    average_init
  * @note    �����˲�����ʼ�������ó���
  * @param  None
  * @retval None
  * @author  RobotPilots
  */
void average_init(moving_Average_Filter *Aver, uint8_t lenth)
{
	uint16_t i;
	
	for(i = 0; i<MAF_MaxSize; i++)
		Aver->num[i] = 0;
	
	if(lenth >MAF_MaxSize)
	{
		lenth = MAF_MaxSize;
	}
	
	Aver->lenth = lenth;
	Aver->pot = 0;
	Aver->aver_num = 0;
	Aver->total = 0;
	
}

/**
  * @brief    average_clear
  * @note    �����˲������
  * @param  None
  * @retval None
  * @author  RobotPilots
  */
void average_clear(moving_Average_Filter *Aver)
{
	uint16_t i;
	
	for(i = 0; i<MAF_MaxSize; i++)
		Aver->num[i] = 0;
	
	Aver->pot = 0;
	Aver->aver_num = 0;
	Aver->total = 0;
}

/**
  * @brief    average_add
  * @note    ����ƽ���˲���������У��Ƚ��ȳ�
  * @param  None
  * @retval None
  * @author  RobotPilots
  */
void average_add(moving_Average_Filter *Aver, float add_data)
{
	
	Aver->total -=  Aver->num[Aver->pot];
	Aver->total += add_data;
	
	Aver->num[Aver->pot] = add_data;
	
	Aver->aver_num = (Aver->total)/(Aver->lenth);
	Aver->pot++;
	
	if(Aver->pot == Aver->lenth)
	{
		Aver->pot = 0;
	}
	
}

/**
  * @brief    LPF_Init
  * @note    LOW_PASS_FILTER Initialize
  * @param  	LPF ��ͨ�˲�����ָ��
							threshold	��ͨ�˲�������ֵ
  * @retval 	none
  * @author  RobotPilots
  */
void LPF_Init(LOW_Pass_Filter *LPF, float threshold)
{
	LPF->last = 0;
	LPF->now = 0;
	LPF->threshold = threshold;
	LPF->output = 0;
	LPF->High_flag = 0;
}

/**
  * @brief    LPF_Init
  * @note    LOW_PASS_FILTER Initialize
  * @param  	LPF ��ͨ�˲�����ָ��
							input_data �����ͨ�˲�������
  * @retval 	none
  * @author  RobotPilots
  */
	
float LPF_add(LOW_Pass_Filter *LPF, float input_data)
{
	float err_temp;
	//�жϲ����������
	if((abs(LPF->last))>LPF->threshold || (abs(LPF->now))>LPF->threshold)
	{
		if(abs(input_data)<LPF->threshold)//�������С����ֵ��ͨ��
		{
			LPF->now = input_data;
			LPF->last = LPF->now;
			LPF->output = 0;
			LPF->High_flag = 0;
			return 0;
		}
		else
		{
			LPF->now = 0;
			LPF->last = LPF->now;
			LPF->output = 0;
			LPF->High_flag = 0;
			return 0;
		}
	}
	
	//�ж����������
	err_temp = abs(input_data-LPF->now);
	if(err_temp<LPF->threshold)//С����ֵ
	{
		LPF->last = LPF->now;
		LPF->now = input_data;
		LPF->High_flag = 0;
	}
	else	//������ֵ
	{
		//��������
		LPF->High_flag = 1;
	}
	
	LPF->output = LPF->now;
	return LPF->output;
}


void LPF_Clear(LOW_Pass_Filter *LPF)
{
	LPF->last = 0;
	LPF->now = 0;
	LPF->output = 0;
	LPF->High_flag = 0;
}
