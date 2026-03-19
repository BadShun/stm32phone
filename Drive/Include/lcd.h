#ifndef __LCD_H
#define __LCD_H

#include "stm32f4xx.h"
#include "fast_gpio.h"

#define SET_RST(x) PBOut(12)=(x)
#define SET_RS(x)  PBOut(14)=(x)
#define SET_CS(x)  PBOut(15)=(x)

typedef enum {
	Rot_0 = 0,
	Rot_90,
	Rot_180,
	Rot_270
} LCD_Rotation;

void LCD_Init();
void LCD_Set_Rotation(LCD_Rotation rot);
void LCD_Clear(uint16_t color);
void LCD_Set_Window(uint16_t x_begin, uint16_t y_begin, uint16_t x_end, uint16_t y_end);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);

#endif