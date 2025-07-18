/*
 * bootloader.c
 *
 *  Created on: Jul 16, 2025
 *      Author: jameshunt
 */

#include "stm32c0xx.h"
#include "bootloader.h"

/*
 *  Handles entering the bootloader so that DFU mode can be used for firmware updates
 *  Based off of this: https://stm32world.com/wiki/STM32_Jump_to_System_Memory_Bootloader#Evaluation
 */

#define BOOTLOADER_FLAG 0xABCDEF00

extern int _bflag; // see linker script

void setBootloaderFlagAndReset() {
	__disable_irq();
	uint32_t *dfu_boot_flag = (uint32_t*) (&_bflag);
	*dfu_boot_flag = BOOTLOADER_FLAG;
	NVIC_SystemReset();
}

static void jumpToBootloader() {
	void (*SysMemBootJump)(void);
	SysMemBootJump = (void (*)(void)) (*((uint32_t *) 0x1FFF0004));
	SysMemBootJump();
}

void handleBootFlag() {
	uint32_t *dfu_boot_flag = (uint32_t*) (&_bflag);

	if (*dfu_boot_flag == BOOTLOADER_FLAG) {
		*dfu_boot_flag = 0;
		jumpToBootloader();
	}
}
