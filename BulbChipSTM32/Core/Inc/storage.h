#include "stm32c0xx.h"

#define FLASH_INIT  0x08000000   //This is the page zero of our flash
#define PAGE_SECTOR 2048         //Page size

//uint32_t * retrieveDataFromAddress(uint32_t page);                 //Retrieve our stored data
void readTextFromFlash(uint32_t page, char* buffer, uint32_t length);
void writeBytes(uint32_t page, uint8_t buf[], uint32_t bufCount);
void memoryPageErase(uint32_t memoryPage);                              //Erase a memory page from the flash retrieve
