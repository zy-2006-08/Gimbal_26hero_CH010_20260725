#ifndef MY_MAIN_H
#define MY_MAIN_H

/*
 * my_main - 应用层业务入口(薄壳化)
 *
 * main.c 回归 CubeMX 生成的纯 C,只保留外设初始化,并在 USER CODE 区调用:
 *   - My_Setup(): 原 main() 的 USER CODE BEGIN 2 内容(业务初始化)
 *   - My_Loop() : 原 main() 的 while(1) 循环体(业务主循环)
 * 二者实体在 my_main.cpp(C++),通过 extern "C" 暴露给纯 C 的 main.c 调用。
 */

#include "main.h"   /* DMA_HandleTypeDef 等 HAL 类型 */

#ifdef __cplusplus
extern "C" {
#endif

void My_Setup(void);
void My_Loop(void);

/* ===== 从 stm32f4xx_it.c 抽离的中断业务逻辑(实体在 my_main.cpp,供纯 C 的中断入口调用) ===== */
void VD_2rx(DMA_HandleTypeDef *hdma);   /* USART3 图传接收(原 stm32f4xx_it.c PM 区) */
void TX_VD_Deal(void);                  /* 图传转发处理 */
void IT_UART5_YK_Handle(void);          /* UART5: YK.DT16_RxCplt_IRQHandler 段 */
void IT_USART6_YK_Handle(void);         /* USART6: YK.VT13_* 段 */

#ifdef __cplusplus
}
#endif

#endif /* MY_MAIN_H */
