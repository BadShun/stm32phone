#include "stm32f4xx.h"                  // Device header
#include "ff.h"
#include "stdio.h"
#include "delay.h"
#include "diskio.h"

#define FLASH_APP_START  0x08080000
#define FLASH_APP_END    0x080FFFFF

#define READ_BUFFER_SIZE 1024
#define PATH_LEN_LIMITE  128

typedef void (*pFunction)(void);

static FATFS fs;
static FRESULT res;

static void GPIO_Reset_All(void) {
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOA, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOB, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOC, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOD, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOE, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOF, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOF, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOG, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOG, DISABLE);

    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOH, ENABLE);
    RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_GPIOH, DISABLE);
}

static void File_System_Init() {
	res = f_mount(&fs,"0:", 1);
	
	printf("%d\r\n", res);
	
	if (res == FR_NO_FILESYSTEM) {
		res = f_mkfs("0:", 0, 0);
		
		if (res == FR_OK) {
            res = f_mount(0,"0:",1);
            res = f_mount(&fs,"0:",1);
			printf("文件系统初始化完成\r\n");
        }
	} else if (res == FR_OK) {
		printf("文件系统初始化完成\r\n");
	}
}

void Launcher_Init() {
	File_System_Init();
	
	printf("开始擦除\r\n");
	
	__disable_irq();
	
	FLASH_Unlock();
	FLASH_EraseSector(FLASH_Sector_8,  VoltageRange_3);
	FLASH_EraseSector(FLASH_Sector_9,  VoltageRange_3);
	FLASH_EraseSector(FLASH_Sector_10, VoltageRange_3);
	FLASH_EraseSector(FLASH_Sector_11, VoltageRange_3);
	FLASH_Lock();

	__enable_irq();
	
	printf("擦除完成\r\n");
}

void Launcher_Download(const char *app_name) {
	char path[PATH_LEN_LIMITE];
	sprintf(path, "0:%s.bin", app_name);
	
	FIL fd;
	res = f_open(&fd, path, FA_OPEN_EXISTING | FA_READ);
	
	if (res == FR_OK) {
		printf("文件打开成功\r\n");
		
		FLASH_Unlock();
		
		UINT read_bytes;
		BYTE buffer[READ_BUFFER_SIZE]= {0};
		uint32_t flash_addr = FLASH_APP_START;
		
		while (1) {
			res = f_read(&fd, buffer, READ_BUFFER_SIZE, &read_bytes);
			
			if (res != FR_OK) {
				printf("文件读取失败 错误代码:%d\r\n", res);
				break;
			}
			__disable_irq();
			
			for (UINT i = 0; i < read_bytes; i += 4) {
				uint32_t word = 0xFFFFFFFF; 
				
				for (UINT j = 0; j < 4 && i + j < read_bytes; j++) {
					((uint8_t *)&word)[j] = buffer[i + j];
				}
				
				FLASH_ProgramWord(flash_addr, word);
				flash_addr += 4;
			}
			
			__enable_irq();
			
			if (read_bytes == 0) {
				break;
			}
		}
		
		FLASH_Lock();
		
		f_close(&fd);
		
		printf("%s.bin写入完成\r\n", app_name);
		
	} else {
		printf("文件打开失败 错误代码:%d\r\n", res);
	}

	f_mount(0, "0:", 1);
}

void Launch_App() {
	uint32_t app_stack;
    uint32_t app_reset_handler;
	app_stack = *(__IO uint32_t*)FLASH_APP_START;
	
	if ((app_stack & 0x2FFE0000) != 0x20000000) {
        printf("非法App\r\n");
        return;
    }
	
    __disable_irq();
	
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
	
	for (int i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;   // 禁用中断
		NVIC->ICPR[i] = 0xFFFFFFFF;   // 清 pending
	}
	
	RCC_DeInit();
	GPIO_Reset_All();

    SCB->VTOR = FLASH_APP_START;

    __set_MSP(app_stack);
	
	app_reset_handler = *(__IO uint32_t*)(FLASH_APP_START + 4);
    pFunction AppResetHandler = (pFunction)app_reset_handler;
    AppResetHandler();
}