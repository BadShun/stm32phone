#include "stm32f4xx.h"
#include "iic.h"
#include "fast_gpio.h"
#include "delay.h"

#define SDA_IN() do {\
			GPIOB->MODER &= ~(3 << (9 * 2));\
			GPIOB->MODER |= 0 << 9 * 2;\
		} while (0)

#define SDA_OUT() do {\
			GPIOB->MODER &= ~(3 << (9 * 2));\
			GPIOB->MODER |= 1 << 9 * 2;\
		} while (0)

#define IIC_SCL    PBOut(8)
#define IIC_SDA    PBOut(9)
#define READ_SDA   PBIn(9)
		
// GPIOB
#define SCL  GPIO_Pin_8
#define SDA  GPIO_Pin_9

void IIC_Init() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SCL | SDA;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	IIC_SCL = 1;
	IIC_SDA = 1;
}

void IIC_Start() {
	SDA_OUT();
	IIC_SDA = 1;
	IIC_SCL = 1;
	Delay_us(4);
	IIC_SDA=0;
	Delay_us(4);
	IIC_SCL=0;
}

void IIC_Stop() {
	SDA_OUT();
	IIC_SCL=0;
	IIC_SDA=0;
 	Delay_us(4);
	IIC_SCL=1; 
	IIC_SDA=1;
	Delay_us(4);							   	
}

uint8_t IIC_Wait_Ack() {
	uint8_t ucErrTime=0;
	
	SDA_IN();
	IIC_SDA=1;
	Delay_us(1);	   
	IIC_SCL=1;
	Delay_us(1);	 
	
	while(READ_SDA) {
		ucErrTime++;
		
		if(ucErrTime > 250) {
			IIC_Stop();
			return 1;
		}
	}
	
	IIC_SCL=0;
	
	return 0;  
}

void IIC_Ack() {
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 0;
	Delay_us(2);
	IIC_SCL = 1;
	Delay_us(2);
	IIC_SCL = 0;
}

void IIC_NAck() {
	IIC_SCL = 0;
	SDA_OUT();
	IIC_SDA = 1;
	Delay_us(2);
	IIC_SCL = 1;
	Delay_us(2);
	IIC_SCL = 0;
}

void IIC_Send_Byte(uint8_t txd) {                          
	SDA_OUT(); 	    
    IIC_SCL=0;//拉低时钟开始数据传输
	
    for(uint8_t t=0; t < 8; t++) {              
        IIC_SDA = (txd & 0x80) >> 7;
        txd <<= 1;
		
		Delay_us(2);   //对TEA5767这三个延时都是必须的
		IIC_SCL = 1;
		Delay_us(2); 
		IIC_SCL = 0;	
		Delay_us(2);
    }	 
}

uint8_t IIC_Read_Byte(uint8_t ack) {
	uint8_t receive = 0;
	SDA_IN();//SDA设置为输入
	
    for(uint8_t i=0; i < 8; i++) {
        IIC_SCL=0; 
        Delay_us(2);
		IIC_SCL = 1;
        receive <<= 1;
		
        if (READ_SDA) {
			receive++;
		}			
		
		Delay_us(1); 
    }	
	
    if (!ack) {
        IIC_NAck();//发送nACK
	} else {
        IIC_Ack(); //发送ACK
	}
	
    return receive;
}