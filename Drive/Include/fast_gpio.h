 #ifndef __FAST_GPIO_H
 #define __FAST_GPIO_H
 
#include "stm32f4xx.h"

#define BITBAND(addr, bitnum)  ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2)) 
#define MEM_ADDR(addr)  	   *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum)) 

#define GPIOA_ODR_Addr    (GPIOA_BASE + 20) 
#define GPIOB_ODR_Addr    (GPIOB_BASE + 20)  
#define GPIOC_ODR_Addr    (GPIOC_BASE + 20)  
#define GPIOD_ODR_Addr    (GPIOD_BASE + 20)  
#define GPIOE_ODR_Addr    (GPIOE_BASE + 20)  
#define GPIOF_ODR_Addr    (GPIOF_BASE + 20)     
#define GPIOG_ODR_Addr    (GPIOG_BASE + 20)    
#define GPIOH_ODR_Addr    (GPIOH_BASE + 20)     
#define GPIOI_ODR_Addr    (GPIOI_BASE + 20)      

#define GPIOA_IDR_Addr    (GPIOA_BASE + 16) 
#define GPIOB_IDR_Addr    (GPIOB_BASE + 16) 
#define GPIOC_IDR_Addr    (GPIOC_BASE + 16) 
#define GPIOD_IDR_Addr    (GPIOD_BASE + 16) 
#define GPIOE_IDR_Addr    (GPIOE_BASE + 16) 
#define GPIOF_IDR_Addr    (GPIOF_BASE + 16) 
#define GPIOG_IDR_Addr    (GPIOG_BASE + 16) 
#define GPIOH_IDR_Addr    (GPIOH_BASE + 16) 
#define GPIOI_IDR_Addr    (GPIOI_BASE + 16)

#define PAOut(n)   BIT_ADDR(GPIOA_ODR_Addr,n)
#define PAIn(n)    BIT_ADDR(GPIOA_IDR_Addr,n) 

#define PBOut(n)   BIT_ADDR(GPIOB_ODR_Addr,n) 
#define PBIn(n)    BIT_ADDR(GPIOB_IDR_Addr,n) 

#define PCOut(n)   BIT_ADDR(GPIOC_ODR_Addr,n) 
#define PCIn(n)    BIT_ADDR(GPIOC_IDR_Addr,n) 

#define PDOut(n)   BIT_ADDR(GPIOD_ODR_Addr,n) 
#define PDIn(n)    BIT_ADDR(GPIOD_IDR_Addr,n) 

#define PEOut(n)   BIT_ADDR(GPIOE_ODR_Addr,n) 
#define PEIn(n)    BIT_ADDR(GPIOE_IDR_Addr,n)

#define PFOut(n)   BIT_ADDR(GPIOF_ODR_Addr,n) 
#define PFIn(n)    BIT_ADDR(GPIOF_IDR_Addr,n)

#define PGOut(n)   BIT_ADDR(GPIOG_ODR_Addr,n) 
#define PGIn(n)    BIT_ADDR(GPIOG_IDR_Addr,n)

#define PHOut(n)   BIT_ADDR(GPIOH_ODR_Addr,n) 
#define PHIn(n)    BIT_ADDR(GPIOH_IDR_Addr,n)

#define PIOut(n)   BIT_ADDR(GPIOI_ODR_Addr,n) 
#define PIIn(n)    BIT_ADDR(GPIOI_IDR_Addr,n)

#endif