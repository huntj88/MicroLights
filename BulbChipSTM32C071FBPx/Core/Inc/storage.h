#include <stdint.h>

#define FLASH_INIT 0x08000000  // This is the page zero of our flash
#define PAGE_SECTOR 2048       // Page size
#define SETTINGS_PAGE 56       // 2K flash reserved for settings starting at page 56
#define BULB_PAGE_0 57         // 14K flash reserved for bulb modes starting at page 57

void writeSettingsToFlash(uint8_t buf[], uint32_t bufCount);
void readSettingsFromFlash(char *buffer, uint32_t length);

void writeBulbModeToFlash(uint8_t mode, uint8_t buf[], uint32_t bufCount);
void readBulbModeFromFlash(uint8_t mode, char *buffer, uint32_t length);

// TODO: flash page is 2048 bytes, only reading half, have it hardcode as 2048.
//       Also should validate json size? Anything to do here?
//       Use less flash for modes and put two modes per page?
