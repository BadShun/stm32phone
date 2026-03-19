#ifndef __IIC_H
#define __IIC_H

#include "stm32f4xx.h"

void IIC_Init();
void IIC_Start();
void IIC_Stop();
uint8_t IIC_Wait_Ack();
void IIC_Ack();
void IIC_NAck();
void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(uint8_t ack);

#endif