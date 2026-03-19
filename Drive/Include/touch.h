#ifndef __TOUCH_H
#define __TOUCH_H

#include "stm32f4xx.h"

void Touch_Init();
void Touch_Adjust();
uint8_t Touch_Is_Pressed();
uint16_t Touch_Get_X();
uint16_t Touch_Get_Y();
uint8_t Touch_Scan(uint8_t flag);

#endif