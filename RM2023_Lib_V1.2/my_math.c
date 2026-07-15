#include "my_math.h"
#include "RM_Lib.h"
#include "communication.h"
#include "stm32f4xx_hal.h"

void Filter_IIRLPF(float in, float *out, float LpfAttFactor)
{
	float res ;
	*out = *out + LpfAttFactor*(in - *out);	
}

extern Kf  kalman_speedYaw1,kalman_accel1,kalman_distend1;

float RampFloat(float final, float now, float ramp)//逐渐调整一个值（now）接近目标值（final），并控制调整的速率
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

//滑动滤波
float Slope(float M ,float *queue ,uint16_t len)
{
	float sum=0;
	float res=0;
  
		//队列已满，FIFO。
		for(uint16_t i=0;i<len-1;i++)
		{
			queue[i] = queue[i+1];
			//更新队列
		}
		queue[len-1] = M;
    
	//更新完队列
	for(uint16_t j=0;j<len;j++)
	{
		sum+=queue[j];
	}
	res = sum/(len);
	
	return res;
}

uint32_t Vision_get_interval(uint8_t lost_flag)//获取两帧之间的时间间隔（单位：毫秒）
{
	uint32_t time_ms;
	uint32_t now_ms, last_ms;
	now_ms =	HAL_GetTick();//获取与上一帧的间隔时间
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
	  kfp-> LastP=0;//上次估算协方差 初始化值为0.02
    kfp-> Now_P=0;//当前估算协方差 初始化值为0
    kfp-> out=0;//卡尔曼滤波器输出 初始化值为0
    kfp-> Kg=0;//卡尔曼增益 初始化值为0
    kfp-> Q=T_Q;//过程噪声协方差 初始化值为0.005
    kfp-> R=T_R;//观测噪声协方差 初始化值为0.543
}

float kalmanFilter(KFP *kfp,float input)
{
     //预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
     kfp->Now_P = kfp->LastP + kfp->Q;
     //卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
     kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
     //更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
     kfp->out = kfp->out + kfp->Kg * (input -kfp->out);//因为这一次的预测值就是上一次的输出值
     //更新协方差方程: 本次的系统协方差付给 kfp->LastP 威下一次运算准备。
     kfp->LastP = (1-kfp->Kg) * kfp->Now_P;
     return kfp->out;
}

// 卡尔曼
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
    p->X_mid =p->A*p->X_last;                     //百度对应公式(1)    x(k|k-1) = A*X(k-1|k-1)+B*U(k)+W(K)     状态方程
    p->P_mid = p->A*p->P_last+p->Q;               //百度对应公式(2)    p(k|k-1) = A*p(k-1|k-1)*A'+Q            观测方程
		p->kg = p->P_mid/(p->P_mid+p->R);             //百度对应公式(4)    kg(k) = p(k|k-1)*H'/(H*p(k|k-1)*H'+R)   更新卡尔曼增益
		p->X_now = p->X_mid + p->kg*(dat-p->X_mid);   //百度对应公式(3)    x(k|k) = X(k|k-1)+kg(k)*(Z(k)-H*X(k|k-1))  修正估计值
    p->P_now = (1-p->kg)*p->P_mid;                //百度对应公式(5)    p(k|k) = (I-kg(k)*H)*P(k|k-1)           更新后验估计协方差
    p->P_last = p->P_now;                         //状态更新
    p->X_last = p->X_now;
		
    return p->X_now;							  //输出预测结果x(k|k)
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

/*******************************************预测*******************************/
/**
  * @brief 卡尔曼滤波函数
  * @other 	Q：过程噪声，Q增大，动态响应变快，收敛稳定性变坏
	R：测量噪声，R增大，动态响应变慢，收敛稳定性变好
  */
double Kf::KalmanFilter(const double ResrcData,double ProcessNoise_Q,double MeasureNoise_R,uint8_t kind)
{
   double R = MeasureNoise_R;
   double Q = ProcessNoise_Q;
   double x_mid;
   double x_now;
   double p_mid ;
   double p_now;
   double kg;
	
	 if(Vision_process.eeror==1)
	 {
		 p_last = 0;
     x_last = 0;
	 }
	 
   x_mid=x_last;
   p_mid=p_last+Q;
   kg=p_mid/(p_mid+R);
   x_now=x_mid+kg*(ResrcData-x_mid);

   p_now=(1-kg)*p_mid;
   p_last = p_now;
   x_last = x_now;
   return x_now;
}

float Get_Diff(uint8_t queue_len, QueueObj *Data,float add_data)//计算新数据与队列平均值之间的差异，并将其返回作为结果
{
    Data->queueTotal-=Data->queue[Data->nowLength];
    Data->queueTotal+=add_data;

		
    Data->queue[Data->nowLength]=add_data;
	
    Data->nowLength++;
	
    if(Data->full_flag==0)//初始队列未满
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
* @brief 清空队列
* @param void
* @return void
*	以队列的逻辑
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
	
	Vision_process.speed_get_last = Get_Diff(3,&Vision_process.speed_queue,angle);//yaw角速度
	Vision_process.speed_get_last = 40.0*(Vision_process.speed_get_last/Vision_process.rx_time_fps); //每毫秒
	Vision_process.speed_get = kalman_speedYaw1.KalmanFilter(Vision_process.speed_get_last,0.001,11,0);
  Vision_process.speed_get = LIMIT(Vision_process.speed_get , -2.0 , 2.0);
		
	Vision_process.accel_get = Get_Diff(3,&Vision_process.accel_queue,Vision_process.speed_get);//yaw角加速度
  Vision_process.accel_get = 50.0*(Vision_process.accel_get/Vision_process.rx_time_fps);//每毫秒
  Vision_process.accel_get = kalman_accel1.KalmanFilter(Vision_process.accel_get,0.001,11,0);
	Vision_process.accel_get = LIMIT(Vision_process.accel_get , -1.0 , 1.0);
	
	if(isnan(Vision_process.speed_get)||isnan(Vision_process.accel_get)) //出错
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
    Vision_process.predict_angle = 0;//清0预测角
		Vision_process.speed_get_last=0;
		Vision_process.accel_get=0;
		Vision_process.speed_get=0;
	}	
	else if((abs(Vision_process.speed_get)==2.0)&&(abs(Vision_process.accel_get)==1.0)&&(abs(Vision_process.predict_angle)==Vision_process.predict_angle_limt))//超范围
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
    Vision_process.predict_angle = 0;//清0预测角
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
		//有报错所以注释掉了
//		Vision_process.feedforwaurd_angle = acc_use * Vision_process.accel_get;
//		Vision_process.predict_angle = RampFloat(Vision_process.predict_angle, predic_use * (1.2f*Vision_process.speed_get*response.distance+1.2f*dir_factor*Vision_process.feedforwaurd_angle*response.distance), 0.3);
//		Vision_process.predict_angle = predic_use * (1.2f*Vision_process.speed_get*response.distance+1.2f*dir_factor*Vision_process.feedforwaurd_angle*response.distance) ;//速度1.1，加速度3
//		Vision_process.predict_angle = LIMIT(Vision_process.predict_angle , -Vision_process.predict_angle_limt, Vision_process.predict_angle_limt);
	}
}

void judge_stop()
{
	uint16_t frist_time,second_time;
	if((Vision_process.speed_get * Vision_process.accel_get)<0.0f)//静止
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
	
	if(tick_ms<30)//ms的小陀螺大体看作不存在，去掉
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

void Predict_anti_top_get_binary(float binary_l, float binary_h)//通过传入的参数设置TOP_Data结构体中与二进制范围相关的变量值，包括下限、上限和中间值。这些值用于后续的预测和处理过程
{
	TOP_Data.binary_low = binary_l;
	TOP_Data.binary_high = binary_h;
	TOP_Data.top_mid = (binary_l+binary_h)/2;
}

void Predict_anti_top_get_speed(void)
{
	TOP_Data.top_speed = (TOP_Data.binary_high+TOP_Data.binary_low)/TOP_Data.top_circle;//反陀螺速度计算
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
	
	if(fabs(binary_first-binary_second)<0.01f)//如果边界相差太近，当作震荡处理
	{
		return;
	}
	
	if(binary_first-binary_second > 0.0f) TOP_Data.flag1=1;
	else if(binary_first-binary_second < 0.0f) TOP_Data.flag1=-1;
	
	//获取陀螺周期
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
float sum; 						/*<! 宽度和数字和 */
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
   sum -= buffer_num[where_num];			  /*<! sum减去旧值 */
   sum += num;													/*<! sum加上新值 */
   buffer_num[where_num++] = num;
   flag > 0? flag-- : 0;								/*<!flag=Length然后递减保证宽度内都是有效波值 */
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
  * @note    滑动滤波器初始化，设置长度
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
  * @note    滑动滤波器清空
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
  * @note    滑动平均滤波器进入队列，先进先出
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
  * @param  	LPF 低通滤波器的指针
							threshold	低通滤波器的阈值
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
  * @param  	LPF 低通滤波器的指针
							input_data 进入低通滤波的数据
  * @retval 	none
  * @author  RobotPilots
  */
	
float LPF_add(LOW_Pass_Filter *LPF, float input_data)
{
	float err_temp;
	//判断不正常的情况
	if((abs(LPF->last))>LPF->threshold || (abs(LPF->now))>LPF->threshold)
	{
		if(abs(input_data)<LPF->threshold)//如果输入小于阈值，通过
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
	
	//判断正常的情况
	err_temp = abs(input_data-LPF->now);
	if(err_temp<LPF->threshold)//小于阈值
	{
		LPF->last = LPF->now;
		LPF->now = input_data;
		LPF->High_flag = 0;
	}
	else	//大于阈值
	{
		//不做处理
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
