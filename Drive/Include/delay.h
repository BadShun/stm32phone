#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

void Delay_Init(void);				//延时函数初始化
void Delay_us(uint32_t nus); //微秒延时函数
void Delay_ms(uint16_t nms);	//毫秒延时函数
void TimingDelay_Decrement(void);

#endif //__DELAY_H

