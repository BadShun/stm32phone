#include "stm32f4xx.h"
#include "spi.h"

// GPIOB
#define SCK    		  GPIO_Pin_3
#define SCK_PinSource GPIO_PinSource3
#define SDO    		  GPIO_Pin_4
#define SDO_PinSource GPIO_PinSource4
#define SDI    		  GPIO_Pin_5
#define SDI_PinSource GPIO_PinSource5

void LCD_SPI_Init() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = SCK | SDO | SDI;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_PinAFConfig(GPIOB, SCK_PinSource, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, SDO_PinSource, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, SDI_PinSource, GPIO_AF_SPI1);
	
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);
	
	SPI_InitTypeDef  SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	//SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;		//定义波特率预分频的值:波特率预分频值为256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
 
	SPI_Cmd(SPI1, ENABLE);
}


void SPI_Write(uint8_t data) {
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE));
	SPI1->DR = data;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE));
	(void)SPI1->DR;
}