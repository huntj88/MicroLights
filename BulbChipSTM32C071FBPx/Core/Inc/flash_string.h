#ifndef INC_FLASH_STRING_H_
#define INC_FLASH_STRING_H_

#include <stddef.h>
#include <stdint.h>

#define FLASH_INIT 0x08000000  // This is the page zero of our flash
#define PAGE_SECTOR 2048       // Page size

void writeStringToFlash(uint32_t page, const char str[], size_t length);
void readStringFromFlash(uint32_t page, char buffer[], size_t length);

#endif /* INC_FLASH_STRING_H_ */
