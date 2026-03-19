#include "stm32f4xx.h"
#include "lcd.h"
#include "spi.h"
#include "delay.h"

// GPIOB
#define RST GPIO_Pin_12
#define LED GPIO_Pin_13
#define RS  GPIO_Pin_14
#define CS  GPIO_Pin_15

#define LCD_W 240
#define LCD_H 320

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t id;
	uint8_t  direction;
	uint16_t wram_cmd;
	uint16_t set_x_cmd;
	uint16_t set_y_cmd;
} LCD_Settings;

static LCD_Settings settings = {0};

static void LCD_GPIO_Init() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB ,ENABLE);
	
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin =  RST | LED | RS | CS;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void LCD_Reset() {
	SET_RST(0);
	Delay_ms(100);
	SET_RST(1);
	Delay_ms(50);
}

static void LCD_Select_Reg(uint8_t reg) {
	SET_CS(0);
	SET_RS(0);
	SPI_Write(reg);
	SET_CS(1);
}

static void LCD_Write_Data(uint8_t *data, uint8_t len) {
	SET_CS(0);
	SET_RS(1);
	
	for (int i = 0; i < len; i++) {
		SPI_Write(data[i]);
	}
	
	SET_CS(1);
}

void LCD_Set_Window(uint16_t x_begin, uint16_t y_begin, uint16_t x_end, uint16_t y_end) {
	LCD_Select_Reg(settings.set_x_cmd);
	LCD_Write_Data((uint8_t[]){x_begin >> 8, 0x00FF & x_begin,
							   x_end   >> 8, 0x00FF & x_end}, 4);
	LCD_Select_Reg(settings.set_y_cmd);
	LCD_Write_Data((uint8_t[]){y_begin >> 8, 0x00FF & y_begin,
							   y_end   >> 8, 0x00FF & y_end}, 4);
	LCD_Select_Reg(settings.wram_cmd);
}

void LCD_Init() {
	LCD_SPI_Init();
	LCD_GPIO_Init();
	LCD_Reset();
	
	LCD_Select_Reg(0xCF);
	LCD_Write_Data((uint8_t[]){0x00, 0xC9, 0x30}, 3);

	LCD_Select_Reg(0xED);
	LCD_Write_Data((uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);

	LCD_Select_Reg(0xE8);
	LCD_Write_Data((uint8_t[]){0x85, 0x10, 0x7A}, 3);

	LCD_Select_Reg(0xCB);
	LCD_Write_Data((uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);

	LCD_Select_Reg(0xF7);
	LCD_Write_Data((uint8_t[]){0x20}, 1);

	LCD_Select_Reg(0xEA);
	LCD_Write_Data((uint8_t[]){0x00, 0x00}, 2);

	LCD_Select_Reg(0xC0);
	LCD_Write_Data((uint8_t[]){0x1B}, 1);
	
	LCD_Select_Reg(0xC1);
	LCD_Write_Data((uint8_t[]){0x00}, 1);

	LCD_Select_Reg(0xC5);
	LCD_Write_Data((uint8_t[]){0x30, 0x30}, 2);

	LCD_Select_Reg(0xC7);
	LCD_Write_Data((uint8_t[]){0xB7}, 1);

	LCD_Select_Reg(0x36);
	LCD_Write_Data((uint8_t[]){0x08}, 1);

	LCD_Select_Reg(0x3A);
	LCD_Write_Data((uint8_t[]){0x55}, 1);

	LCD_Select_Reg(0xB1);
	LCD_Write_Data((uint8_t[]){0x00, 0x1A}, 2);

	LCD_Select_Reg(0xB6);
	LCD_Write_Data((uint8_t[]){0x0A, 0xA2}, 2);

	LCD_Select_Reg(0xF2);
	LCD_Write_Data((uint8_t[]){0x00}, 1);

	LCD_Select_Reg(0x26);
	LCD_Write_Data((uint8_t[]){0x01}, 1);

	LCD_Select_Reg(0xE0);
	LCD_Write_Data((uint8_t[]){
    0x0F,0x2A,0x28,0x08,0x0E,0x08,0x54,0xA9,
    0x43,0x0A,0x0F,0x00,0x00,0x00,0x00}, 15);

	LCD_Select_Reg(0xE1);
	LCD_Write_Data((uint8_t[]){
    0x00,0x15,0x17,0x07,0x11,0x06,0x2B,0x56,
    0x3C,0x05,0x10,0x0F,0x3F,0x3F,0x0F}, 15);

	LCD_Select_Reg(0x2B);
	LCD_Write_Data((uint8_t[]){0x00,0x00,0x01,0x3F}, 4);

	LCD_Select_Reg(0x2A);
	LCD_Write_Data((uint8_t[]){0x00,0x00,0x00,0xEF}, 4);

	LCD_Select_Reg(0x11);
	Delay_ms(120);
	LCD_Select_Reg(0x29);
	
	LCD_Set_Rotation(Rot_180);
	LCD_Clear(0xFFFF);
}

void LCD_Set_Rotation(LCD_Rotation rot) {
	settings.set_x_cmd = 0x2A;
	settings.set_y_cmd = 0x2B;
	settings.wram_cmd  = 0x2C;
	
	switch (rot) {
		case Rot_0:
			settings.width 	= LCD_W;
			settings.height = LCD_H;
			LCD_Select_Reg(0x36);
			LCD_Write_Data((uint8_t[]){(1 << 3) | (0 << 6) | (0 << 7)}, 1);
			break;
		case Rot_90:
			settings.width 	= LCD_H;
			settings.height = LCD_W;
			LCD_Select_Reg(0x36);
			LCD_Write_Data((uint8_t[]){(1 << 3) | (0 << 7) | (1 << 6) | (1 << 5)}, 1);
			break;
		case Rot_180:
			settings.width 	= LCD_W;
			settings.height = LCD_H;
			LCD_Select_Reg(0x36);
			LCD_Write_Data((uint8_t[]){(1 << 3) | (1 << 6) | (1 << 7)}, 1);
			break;
		case Rot_270:
			settings.width 	= LCD_H;
			settings.height = LCD_W;
			LCD_Select_Reg(0x36);
			LCD_Write_Data((uint8_t[]){(1 << 3) | (1 << 7) | (1 << 5)}, 1);
			break;
	}
}

void LCD_Clear(uint16_t color) {
	LCD_Set_Window(0, 0, settings.width - 1, settings.height - 1);
	
	SET_CS(0);
	SET_RS(1);
	
	for (uint16_t i = 0; i < settings.height; i++) {
		for (uint16_t j = 0; j < settings.width; j++) {
			SPI_Write(color >> 8);
			SPI_Write(color & 0x00FF);
		}
	}
	
	SET_CS(1);
}

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
	LCD_Set_Window(x, y, x, y);
	SET_CS(0);
	SET_RS(1);
	SPI_Write(color >> 8);
	SPI_Write(color & 0x00FF);
	SET_CS(1);
}
