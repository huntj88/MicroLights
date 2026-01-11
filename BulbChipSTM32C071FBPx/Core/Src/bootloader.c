/*
 * bootloader.c
 *
 *  Created on: Jul 16, 2025
 *      Author: jameshunt
 */

#include "bootloader.h"
#include "stm32c0xx.h"

/*
 *  Handles entering the bootloader so that DFU mode can be used for firmware updates
 *  Based off of this: https://stm32world.com/wiki/STM32_Jump_to_System_Memory_Bootloader#Evaluation
 */

#define BOOTLOADER_FLAG 0xABCDEF00

extern int _bflag;  // see linker script
// TODO: 8 bytes of memory assigned to _bflag, but we only use the first 4 bytes here (int). Could
// we use the other 4 bytes to count failures before enabling dfu automatically? The goal would be
// to prevent boot loops where we are not able to invoke dfu via serial, requiring debugger/flash
// tool to be used. Woul need to increment in errorHandler, and HardFault_Handler. Can HardFault
// handler reset MCU without power loss? _bflag gets reset when power is lost.

void setBootloaderFlagAndReset() {
    __disable_irq();
    uint32_t *dfu_boot_flag = (uint32_t *)(&_bflag);
    *dfu_boot_flag = BOOTLOADER_FLAG;
    NVIC_SystemReset();
}

static void jumpToBootloader() {
    void (*SysMemBootJump)(void);
    SysMemBootJump = (void (*)(void))(*((uint32_t *)0x1FFF0004));
    SysMemBootJump();
}

void handleBootFlag() {
    uint32_t *dfu_boot_flag = (uint32_t *)(&_bflag);

    if (*dfu_boot_flag == BOOTLOADER_FLAG) {
        *dfu_boot_flag = 0;
        jumpToBootloader();
    }
}
