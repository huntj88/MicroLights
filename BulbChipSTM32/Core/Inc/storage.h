#include "stm32c0xx.h"

#define FLASH_INIT  0x08000000   //This is the page zero of our flash
#define PAGE_SECTOR 2048         //Page size
#define BULB_PAGE_0_OFFSET 56    // 16K flash reserved for bulb modes starting at page 56

void writeBulbMode(uint8_t mode, uint8_t buf[], uint32_t bufCount);
void readBulbMode(uint8_t mode, char *buffer, uint32_t length);
//void readTextFromFlash(uint32_t page, char *buffer, uint32_t length);

