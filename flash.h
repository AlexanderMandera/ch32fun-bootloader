#include "ch32fun.h"

void FLASH_Unlock_Fast(void)
{
    // Unlock the flash memory for writing
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;

    // Fast program mode unlock
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;
}

int FLASH_ErasePage_Fast(uint32_t pageAddress)
{
    pageAddress &= 0xFFFFFF00;

    FLASH->CTLR |= CR_PAGE_ER;
    FLASH->ADDR = pageAddress;
    FLASH->CTLR |= CR_STRT_Set;
    while(FLASH->STATR & SR_BSY)
        ;
    FLASH->CTLR &= ~CR_PAGE_ER;
    
    return 0;
}

void FLASH_ProgramPage_Fast(uint32_t pageAddress, uint8_t *data)
{
    uint8_t size = 64;

    pageAddress &= 0xFFFFFF00;

    // Clear buffer and prep for flashing.
	FLASH->CTLR = CR_PAGE_PG;  // synonym of FTPG.
    FLASH->ADDR = (intptr_t)pageAddress;
    printf( "FLASH->ADDR = %08lx\n", FLASH->ADDR );

    while( FLASH->STATR & SR_BSY );  // No real need for this.
    while( FLASH->STATR & SR_WR_BSY );  // No real need for this.

    while(size)
    {
        *(uint32_t *)pageAddress = *(uint32_t *)data;
        pageAddress += 4;
        data += 1;
        size -= 1;
        printf("To write %d bytes to %08lx\n", size, pageAddress);
		while( FLASH->STATR & SR_WR_BSY );  // Only needed if running from RAM.
    }

    printf("Writing to flash\n");
    // Actually write the flash out. (Takes about 3ms)
	FLASH->CTLR = CR_PAGE_PG|CR_STRT_Set;

    while(FLASH->STATR & FLASH_STATR_BSY);

    FLASH->CTLR &= ~CR_PAGE_PG;
}