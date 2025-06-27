#include "storage.h"

uint32_t getHexAddressPage(int dataPage){
	uint32_t bits       = PAGE_SECTOR * dataPage;
	uint32_t hexAddress = FLASH_INIT + bits;
	return hexAddress;
}

void memoryPageErase(uint32_t memoryPage){
	HAL_StatusTypeDef eraseHandler = HAL_FLASH_Unlock();

	eraseHandler = FLASH_WaitForLastOperation(500);
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR);
	FLASH_PageErase(memoryPage);

	eraseHandler = FLASH_WaitForLastOperation(500);
	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
	HAL_FLASH_Lock();
}

void writeBytes(uint32_t hexPage, uint8_t buf[], uint32_t bufCount)
{
	memoryPageErase(DATA_PAGE);

	HAL_FLASH_Unlock();

	uint8_t emptyPaddingLength = DATA_SPACE - (bufCount % DATA_SPACE);
	uint8_t dataSpaceBuf[DATA_SPACE];

	// write DATA_SPACE bytes at a time, pad with zero's at end if bytes count is not a multiple of DATA_SPACE
	for (int32_t i = 0; i < bufCount + emptyPaddingLength; i++)
	{
		if (i < bufCount) {
			dataSpaceBuf[i % DATA_SPACE] = buf[i];
		} else {
			dataSpaceBuf[i % DATA_SPACE] = '0';
		}

		if (i % DATA_SPACE == DATA_SPACE - 1) {
			HAL_FLASH_Program(TYPEPROGRAM_DOUBLEWORD, hexPage + (DATA_SPACE * (i / DATA_SPACE)), dataSpaceBuf);
		}
	}

	HAL_FLASH_Lock();
}

uint32_t * retrieveDataFromAddress(uint32_t hexAddress) {
	return (uint32_t*)hexAddress;
}
