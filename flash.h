/**
 * Parts of this code are from the ch32v30x_flash.c source by WCH, licensed under Apache 2.0
 */

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

    FLASH->CTLR |= CR_PAGE_PG;
    while(FLASH->STATR & SR_BSY)
        ;
    while(FLASH->STATR & SR_WR_BSY)
        ;

    while(size)
    {
        *(uint32_t *)pageAddress = *(uint32_t *)data;
        pageAddress += 4;
        data += 1;
        size -= 1;
        printf("To write %d bytes to %08lx\n", size, pageAddress);
        while(FLASH->STATR & SR_WR_BSY)
            ;
    }

    FLASH->CTLR |= CR_PG_STRT;
    while(FLASH->STATR & SR_BSY)
        ;
    FLASH->CTLR &= ~CR_PAGE_PG;
}