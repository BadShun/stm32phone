#include "stm32f4xx.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "touch.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "stm32phone.h"
#include "sdcard.h"
#include "stdio.h"
#include "app_launcher.h"

static uint8_t count_5 = 0;

void TIM2_Init() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	TIM_TimeBaseInitTypeDef TIM_BaseStruct;
    TIM_BaseStruct.TIM_Period = 1000 - 1;
    TIM_BaseStruct.TIM_Prescaler = 84 - 1;
    TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_BaseStruct);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    TIM_Cmd(TIM2, ENABLE);
}

int main(void) {	
	Delay_Init();
	LCD_USART_Init();
	LCD_Init();
	Touch_Init();
	lv_init();
	lv_port_disp_init(); 
	lv_port_indev_init(); 
	TIM2_Init();
	phone_desktop();
	Launcher_Init();
	
	while(1) {
		if (count_5 == 5) {
			lv_timer_handler();
		}
	}
	
	while(1) {}
}

void TIM2_IRQHandler() {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        lv_tick_inc(1); 
		
		if (count_5 < 5) {
			count_5++;
		} else {
			count_5 = 1;
		}
    }
}



