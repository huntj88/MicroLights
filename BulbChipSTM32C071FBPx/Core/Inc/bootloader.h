/*
 * bootloader.h
 *
 *  Created on: Jul 16, 2025
 *      Author: jameshunt
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

/*
 *  Handles entering the bootloader so that DFU mode can be used for firmware updates
 *  Based off of this: https://stm32world.com/wiki/STM32_Jump_to_System_Memory_Bootloader#Evaluation
 */

void setBootloaderFlagAndReset();
void handleBootFlag();

#endif /* INC_BOOTLOADER_H_ */
