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

#ifdef __cplusplus
extern "C" {
#endif

void My_Setup(void);
void My_Loop(void);

#ifdef __cplusplus
}
#endif

#endif /* MY_MAIN_H */
