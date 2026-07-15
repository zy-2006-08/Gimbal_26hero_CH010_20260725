#ifndef MY_MATH_H
#define MY_MATH_H

#ifdef __cplusplus
 extern "C" {
#endif
	 
#include "stdint.h"
#include "math.h"
	 
//#define LIMIT(x,min,max) ((x)<(min)?(min):((x)>(max)?(max):(x)))
#define RP_MAX(x, y) 					((x)>(y)?(x):(y))
#define RP_MIN(x, y) 					((x)>(y)?(y):(x))
#define MAF_MaxSize  100
#define GIMBAL_YAW_CIRCULAR_STEP   360.0	 
	 
	 typedef struct 
{
    float LastP;//上次估算协方差 初始化值为0.02
    float Now_P;//当前估算协方差 初始化值为0
    float out;//卡尔曼滤波器输出 初始化值为0
    float Kg;//卡尔曼增益 初始化值为0
    float Q;//过程噪声协方差 初始化值为0.005
    float R;//观测噪声协方差 初始化值为0.543
}KFP;//Kalman Filter parameter


typedef struct {
    float X_last; //上一时刻的最优结果  X(k-|k-1)
    float X_mid;  //当前时刻的预测结果  X(k|k-1)
    float X_now;  //当前时刻的最优结果  X(k|k)
    float P_mid;  //当前时刻预测结果的协方差  P(k|k-1)
    float P_now;  //当前时刻最优结果的协方差  P(k|k)
    float P_last; //上一时刻最优结果的协方差  P(k-1|k-1)
    float kg;     //kalman增益
    float A;      //系统参数
    float B;
    float Q;
    float R;
    float H;
	
}extKalman_t;

/***************/
/*数据结构*/

typedef struct
{
    uint16_t nowLength;
    uint16_t queueLength;
    float queueTotal;
    //长度
    float queue[100];
    //指针
    float aver_num;//平均值

    float Diff;//差分值

    uint8_t full_flag;
} QueueObj;


class Kf{

private:
	double x_last;
	double p_last;
public:
	double KalmanFilter(const double ResrcData,double ProcessNiose_Q,double MeasureNoise_R,uint8_t kind);

};

typedef struct {
    QueueObj speed_queue;
    QueueObj accel_queue;
    QueueObj dis_queue;
	  float predict_angle;
	  float predict_angle_limt;
    float feedforwaurd_angle;
		float feedforwaurd_anti_top_angle;
		float predict_anti_top_angle;
		float yaw_speed_kp;
		float feed_pre_angle_yaw;
	  float speed_get;
	  float speed_get_last; 
	  float accel_get;
		float angle_now;
  	float angle_past;
	  float angle_get;
	  float distend_get;
	  uint8_t eeror;
	  float predict_anti_top_angle_limit;
	  uint32_t 		rx_time_prev;		// 接收数据的前一时刻
    uint32_t 		rx_time_now;		// 接收数据的当前时刻
    uint16_t 		rx_time_fps;		// 帧率
} Vision_process_t;

typedef struct Anti_top_Data
{
	float binary_low;
	float binary_high;
	float top_speed;
	float top_mid;
	float top_circle;
	int8_t flag;
	int8_t flag1;
	uint8_t Anti_top_flag;
	uint32_t cnt;
	uint32_t cnt_max = 300;
	float binary_err;
}Anti_top_Data;

typedef struct moving_Average_Filter
{
	float num[100];
	uint8_t lenth;
	uint8_t pot;//当前位置
	float total;
	float aver_num;
}moving_Average_Filter;	//最大设置MAF_MaxSize个

typedef struct LOW_Pass_Filter
{
	float last;
	float now;
	float threshold;
	float output;
	uint8_t High_flag;//加了一个跳变时指示的flag
}LOW_Pass_Filter;

extern Vision_process_t Vision_process;
extern LOW_Pass_Filter vision_speedY_LPF;
extern moving_Average_Filter vision_angleY_MF,vision_speedY_MF,vision_distance_MF;
extern extKalman_t kalman_accel,kalman_speedYaw,vision_absolute_Yaw_Kal,vision_speedY_KF,vision_angleY_KF;
extern Anti_top_Data TOP_Data;

void judge_stop();
void Predict_anti_top_get_circle(void);
float get_auto_speed(moving_Average_Filter *AUTO_MF, float angle);
uint32_t Vision_get_interval(uint8_t lost_flag);
int8_t Predict_Anti_Top_Binary_judge(float yaw_angle);
void Predict_anti_top_get_binary(float binary_l, float binary_h);
void Predict_anti_top_get_speed(void);
void Predict_Anti_Top_binary_update(float now_yaw);
void Predict_Anti_Top_Data_Clear(void);
void Predict_Anti_Top_Data_Heart_beat(void);
float Predict_Anti_Top_Judge_Yaw(float auto_yaw);
float Vision_Anti_top(float last_angle,float now_angle);
void Predict_Anti_Top_Cal_all(float binary_first, float binary_second);
float Predict_Anti_Top_Judge_Yaw(float auto_yaw);

float RampFloat(float final, float now, float ramp);
void kalmanCreate(KFP *kfp,float T_Q,float T_R);
float kalmanFilter(KFP *kfp,float input);

void  Filter_IIRLPF(float in, float *out, float LpfAttFactor);	 
float Slope(float M ,float *queue ,uint16_t len);	 
void  KalmanCreate(extKalman_t *p,float T_Q,float T_R);
float KalmanFilter(extKalman_t *p,float dat);
void KalmanClear(extKalman_t *p);
float Get_Diff(uint8_t queue_len, QueueObj *Data,float add_data);
void  Vision_Normal(float angle);	 
float DeathZoom(float input, float center, float death);	 
	 
void	MeanFilter_Init();
float MeanFilter(float num);	 
	 
void average_init(moving_Average_Filter *Aver, uint8_t lenth);
void average_clear(moving_Average_Filter *Aver);
void average_add(moving_Average_Filter *Aver, float add_data); 	 
	 
void LPF_Init(LOW_Pass_Filter *LPF, float threshold);
float LPF_add(LOW_Pass_Filter *LPF, float input_data);
void LPF_Clear(LOW_Pass_Filter *LPF);
	 
	 #ifdef __cplusplus
}
#endif

#endif


