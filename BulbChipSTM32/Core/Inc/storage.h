#include "stm32c0xx.h"

#define FLASH_INIT  0x08000000   //This is the page zero of our flash
#define DATA_SPACE  8            //Samples between each saved element
#define PAGE_SECTOR 2048         //Page size
#define DATA_PAGE   31           //Page that will store our data

uint32_t getHexAddressPage(int dataPage);                               //Here receive our page on int format and we get the address on hex format
uint32_t retrieveDataFromAddress(uint32_t hexAddress);                  //Retrieve our stored data
void writeBytes(uint32_t hexPage, uint8_t buf[], uint32_t bufCount);
void memoryPageErase(uint32_t memoryPage);                              //Erase a memory page from the flash retrieve
