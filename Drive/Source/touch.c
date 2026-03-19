#include "stm32f4xx.h"
#include "touch.h"
#include "fast_gpio.h"
#include "delay.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#define READ_TIMES 5 	// 读取次数
#define LOST_VAL   1  	// 丢弃值
#define ERR_RANGE  50   // 误差范围 

#define CMD_RDX 0XD0
#define CMD_RDY 0X90

// GPIOB
#define CLK GPIO_Pin_0
#define IRQ GPIO_Pin_1
#define DO  GPIO_Pin_2
// GPIOC
#define CS GPIO_Pin_5
// GPIOF
#define DIN GPIO_Pin_11

#define TPEN  		PBIn(1)  	// T_PEN
#define TDOUT 		PBIn(2)   	// T_MISO
#define TDIN 		PFOut(11)  	// T_MOSI
#define TCLK 		PBOut(0)  	// T_SCK
#define TCS  		PCOut(5)  	// T_CS

#define T_PRES_DOWN 0x80  //触屏被按下	  
#define T_CATH_PRES 0x40  //有按键按下了

typedef struct {
	uint16_t x0;  //原始坐标(第一次按下时的坐标)
	uint16_t y0;
	uint16_t x;   //当前坐标(此次扫描时,触屏的坐标)
	uint16_t y;						   	    
	uint8_t  sta; //笔的状态        			  							
	float xfac;					
	float yfac;
	short xoff;
	short yoff;	   
	u8 touchtype;
} Touch_Dev;

Touch_Dev touch_dev = {0};

uint16_t left_top[2]     = {410, 3731};  // 3
uint16_t left_bottom[2]  = {410, 300};   // 1
uint16_t right_top[2]    = {3620, 3748}; // 2
uint16_t right_bottom[2] = {3620, 300};	 // 0

static void Touch_Write_Byte(uint8_t cmd) {
	for (uint8_t i = 0; i < 8; i++) {
		TDIN = (cmd & 0x80) >> 7;
		cmd <<= 1;
		TCLK = 0;
		Delay_us(1);
		TCLK = 1;
	}
}

static uint16_t Touch_Read_AD(uint8_t cmd) {
	TCLK = 0;	// 先拉低时钟 	 
	TDIN = 0; 	// 拉低数据线
	TCS  = 0; 	// 选中触摸屏IC
	Touch_Write_Byte(cmd);//发送命令字
	Delay_us(6);// ADS7846的转换时间最长为6us
	TCLK = 0; 	     	    
	Delay_us(1);    	   
	TCLK = 1;		// 给1个时钟，清除BUSY	    	    
	Delay_us(1);    
	TCLK = 0;
	
	uint16_t ad_data = 0;
	for (uint8_t i = 0; i < 16; i++) {
		ad_data <<= 1;
		TCLK = 0;	// 下降沿有效  	    	   
		Delay_us(1);    
		TCLK = 1;
		ad_data += TDOUT; 
	}
	
	ad_data >>= 4;
	TCS = 1;
	
	return ad_data;
}

static uint16_t Touch_Read_XOY(uint8_t cmd) {
	uint16_t buffer[READ_TIMES];
	
	for (uint8_t i = 0; i < READ_TIMES; i++) {
		buffer[i] = Touch_Read_AD(cmd);
	}
	
	uint16_t tmp;
	
	for (uint8_t i = 0; i < READ_TIMES - 1; i++) {
		for (uint8_t j = 0; j < READ_TIMES; j++) {
			if (buffer[i] > buffer[j]) {
				tmp = buffer[i];
				buffer[i] = buffer[j];
				buffer[j] = tmp;
			}
		}
	}
	
	uint16_t sum = 0;
	
	for (uint8_t i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++) {
		sum += buffer[i];
	}
	
	tmp = sum / (READ_TIMES - 2 * LOST_VAL);
	return tmp;
}

static void Touch_Read_XY(uint16_t *x, uint16_t *y) {
	*x = Touch_Read_XOY(CMD_RDX);
	*y = Touch_Read_XOY(CMD_RDY);
}

uint8_t Touch_Read_XY2(uint16_t *x, uint16_t *y) {
	uint16_t x1, y1;
	uint16_t x2, y2;
	
	Touch_Read_XY(&x1, &y1);
	Touch_Read_XY(&x2, &y2);
	
	if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE)) // 前后两次采样在+-50内
    &&  ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE))) {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
		
        return 1;
    } else {
		return 0;
	}
}

uint8_t Touch_Scan(uint8_t flag) {
	if(TPEN == 0) {
		if(flag) {
			Touch_Read_XY2(&touch_dev.x, &touch_dev.y); //读取物理坐标
		} else {
			Touch_Read_XY2(&touch_dev.x, &touch_dev.y); //读取屏幕坐标
			touch_dev.x = touch_dev.xfac * touch_dev.x + touch_dev.xoff; //将结果转换为屏幕坐标
			touch_dev.y = (320 - (touch_dev.yfac * touch_dev.y + touch_dev.yoff)) ? (320 - (touch_dev.yfac * touch_dev.y + touch_dev.yoff)) : 0;  
		}
		
		if((touch_dev.sta & T_PRES_DOWN)==0) { // 之前没有被按下	 
			touch_dev.sta = T_PRES_DOWN | T_CATH_PRES; // 按键按下  
			touch_dev.x0=touch_dev.x;// 记录第一次按下时的坐标
			touch_dev.y0=touch_dev.y;  	   			 
		}else {
			if(touch_dev.sta & T_PRES_DOWN) { // 之前是被按下的
				touch_dev.sta &= ~(1 << 7);// 标记按键松开	
			} else { // 之前就没有被按下
				touch_dev.x0 = 0;
				touch_dev.y0 = 0;
				touch_dev.x = 0xffff;
				touch_dev.y = 0xffff;
			}	    
		}
	}
	
	return touch_dev.sta & T_PRES_DOWN;//返回当前的触屏状态
}

uint8_t Touch_Is_Pressed() {
	if (touch_dev.sta & T_PRES_DOWN) {
		return 1;
	}
	
	return 0;
}

uint16_t Touch_Get_X() {
	if (touch_dev.x > 0 && touch_dev.x < 240) {
		return touch_dev.x;
	}
	
	return touch_dev.x > 120 ? 240 : 0;
}

uint16_t Touch_Get_Y() {
	if (touch_dev.y > 0 && touch_dev.y < 320) {
		return touch_dev.y;
	}
	
	return touch_dev.y > 160 ? 320 : 0;
}

static void Touch_HW_Init() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOF, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = IRQ | DO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

}

void Touch_Init() {
	Touch_HW_Init();
	
	Touch_Read_XY(&touch_dev.x,&touch_dev.y);//第一次读取初始化	 
	
	touch_dev.xfac = (float)(240) / (right_bottom[0] - left_bottom[0]);
	touch_dev.xoff = (240 - touch_dev.xfac * (right_bottom[0] + left_bottom[0])) / 2;
	
	touch_dev.yfac = (float)(320) / (right_top[1] - right_bottom[1]);
	touch_dev.yoff = (320 - touch_dev.yfac * (right_top[1] + right_bottom[1])) / 2;
}