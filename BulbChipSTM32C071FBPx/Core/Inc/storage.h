#include <stdint.h>

#define FLASH_INIT 0x08000000  // This is the page zero of our flash
#define PAGE_SECTOR 2048       // Page size

void writeStringToFlash(uint32_t page, const char str[], uint32_t length);
void readStringFromFlash(uint32_t page, char buffer[], uint32_t length);
