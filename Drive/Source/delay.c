/***
	***************************************************************************
	*	@file  	delay.c
	*	@brief   delay接口相关函数
   ***************************************************************************
   *  @description
	*
	*  SysTick定时器配置为1ms中断，实现毫秒延时
	* 	
	***************************************************************************
***/

#include "delay.h"

static uint32_t g_fac_us = 168;       /* us延时倍乘数 */
static __IO uint32_t TimingDelay;
//	函数：延时初始化
//	说明：配置 SysTick 为1ms中断，并启动定时器
//
void Delay_Init(void)
{
	SysTick_Config(SystemCoreClock / 1000);  //配置SysTick时钟为1ms中断
}

/**
 * @brief       延时nus
 * @param       nus: 要延时的us数.
 * @note        nus取值范围 : 0~190887435(最大值即 2^32 / fac_us @fac_us = 21)
 * @retval      无
 */
void Delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;        /* LOAD的值 */
    ticks = nus * g_fac_us;                 /* 需要的节拍数 */
    told = SysTick->VAL;                    /* 刚进入时的计数器值 */
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;        /* 这里注意一下SYSTICK是一个递减的计数器就可以了 */
            }
            else 
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;                      /* 时间超过/等于要延迟的时间,则退出 */
            }
        }
    }
}

/**
 * @brief       延时nms
 * @param       nms: 要延时的ms数 (0< nms <= 65535)
 * @retval      无
 */
void Delay_ms(uint16_t nms)
{
    uint32_t repeat = nms / 30;     /*  这里用30,是考虑到可能有超频应用 */
    uint32_t remain = nms % 30;

    while (repeat)
    {
        Delay_us(30 * 1000);        /* 利用delay_us 实现 1000ms 延时 */
        repeat--;
    }

    if (remain)
    {
        Delay_us(remain * 1000);    /* 利用delay_us, 把尾数延时(remain ms)给做了 */
    }
}

void TimingDelay_Decrement(void) {
	if (TimingDelay != 0) { 
		TimingDelay--;
	}
}



