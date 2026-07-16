/*
 * my_main.cpp - 应用层业务实现(薄壳化)
 *
 * 承载原 main.c 里的全部 C++ 业务:全局对象、业务函数、中断回调,
 * 并对外提供 My_Setup()/My_Loop() 两个 extern "C" 入口给 main.c 调用。
 *
 * 分阶段搬运中:当前为骨架阶段,My_Setup/My_Loop 暂为空壳,
 * 业务仍在 main.c,先让链接跑通、行为零变化。后续逐批把业务搬入本文件。
 */

#include "my_main.h"

// 骨架阶段:空实现。业务仍在 main.c,行为不变。
// 后续搬运时,My_Setup() 承接原 USER CODE BEGIN 2,My_Loop() 承接原 while(1) 循环体。
extern "C" void My_Setup(void)
{
}

extern "C" void My_Loop(void)
{
}
