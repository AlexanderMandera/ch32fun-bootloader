#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "hsusb_v30x.h"
#include "hsusb_v30x.c"
#include "flash.h"

uint32_t count;

uint8_t programBuffer[390];
uint32_t codeLength = 0;
uint32_t programAddress = 0x08005000;
uint32_t verifyAddress = 0x08005000;

int lrx = 0;
uint8_t input[256];

bool response = false;
uint8_t output[32];
uint8_t outputSize = 0;

int HandleHidUserSetReportSetup( struct _USBState * ctx, tusb_control_request_t * req )
{
	int id = req->wValue & 0xff;
    printf("HandleHidUserSetReportSetup: id = %d, wLength = %d\n", id, req->wLength);
	if( id == 0xaa && req->wLength <= sizeof(input) )
	{
        printf("HandleHidUserSetReportSetup: Receiving data...\n");
		ctx->pCtrlPayloadPtr = input;
		lrx = req->wLength;
		return req->wLength;
	}
	return 0;
}

int HandleHidUserGetReportSetup( struct _USBState * ctx, tusb_control_request_t * req )
{
	int id = req->wValue & 0xff;
	if( id == 0xaa )
	{
        if(!response) return 0;

        ctx->pCtrlPayloadPtr = output;
        response = false;
        return outputSize;
	}
	return 0;
}

#define CMD_ERASE 0x01
#define CMD_WRITE 0x02
#define CMD_WRITE_LAST 0x06
#define CMD_VERIFY 0x03
#define CMD_END 0x04
#define CMD_JUMP 0x05
#define CMD_INIT 0x07

void jumpToApp() {
    // Disable USBHS
    USBHSD->CONTROL &= ~USBHS_UC_DEV_PU_EN;
    USBHSD->CONTROL |= USBHS_UC_CLR_ALL | USBHS_UC_RESET_SIE;
    NVIC_DisableIRQ(USBHS_IRQn);
    USBHSD->HOST_CTRL=0x00;
    USBHSD->CONTROL=0x00;
    USBHSD->INT_EN=0x00;
    USBHSD->ENDP_CONFIG=0xFFffffff;
    USBHSD->CONTROL&=~USBHS_UC_DEV_PU_EN;
    USBHSD->CONTROL|=USBHS_UC_CLR_ALL|USBHS_UC_RESET_SIE;
    USBHSD->CONTROL=0x00;
    Delay_Ms(50);

    RCC->APB2PCENR &= ~RCC_APB2Periph_GPIOA;
    RCC->AHBPCENR &= ~(RCC_AHBPeriph_USBHS | RCC_AHBPeriph_DMA1);

    // Reset the system
    NVIC_EnableIRQ(Software_IRQn);
    NVIC_SetPendingIRQ(Software_IRQn);
}

void handleData() {
    if(lrx < 2) return;
    lrx = 0;

    // Output the first three bytes of the input buffer
    printf("handleData: input[0] = %d, input[1] = %d, input[2] = %d\n", input[0], input[1], input[2]);

    uint8_t length = input[1];
    uint8_t command = input[2];

    uint8_t *data = &input[3];

    uint8_t status = 255;

    printf("handleData: command = %d, length = %d\n", command, length);

    switch (command)
    {
        case CMD_INIT:
            printf("CMD_INIT: Initializing...\n");
            programAddress = 0x08005000;
            codeLength = 0;
            verifyAddress = 0x08005000;
            status = 0x37; // Success
            break;
        case CMD_ERASE:
            printf("CMD_ERASE: Unlocking flash...\n");
            FLASH_Unlock_Fast();
            printf("CMD_ERASE: Flash unlocked.\n");
            status = 0x00; // Success
            break;
        case CMD_WRITE:
        case CMD_WRITE_LAST:
            printf("CMD_WRITE: Writing %d bytes to buffer...\n", length);
            
            for(int i = 0; i < length; i++) {
                programBuffer[codeLength + i] = data[i];
            }
            codeLength += length;
            printf("CMD_WRITE: Current buffer length: %ld\n", codeLength);
            if(codeLength >= 256 || command == CMD_WRITE_LAST) {
                printf("CMD_WRITE: Buffer full. Flashing to address 0x%08lX...\n", programAddress);
                FLASH_Unlock_Fast();
                FLASH_ErasePage_Fast(programAddress);
                printf("CMD_WRITE: Page erased at address 0x%08lX.\n", programAddress);
                FLASH_ProgramPage_Fast(programAddress, (uint32_t*)programBuffer);
                printf("CMD_WRITE: Page programmed at address 0x%08lX.\n", programAddress);
                if(codeLength >= 256) {
                    codeLength -= 256;
                    for(int i = 0; i < codeLength; i++) {
                        programBuffer[i] = programBuffer[i + 256];
                    }
                    programAddress += 0x100;
                    printf("CMD_WRITE: Remaining buffer length: %ld. Next address: 0x%08lX.\n", codeLength, programAddress);
                }
            }
            status = 0x00; // Success
            break;
        case CMD_VERIFY:
            printf("CMD_VERIFY: Verifying %d bytes at address 0x%08lX...\n", length, verifyAddress);
            status = 0x00; // Success
            for(int i = 0; i < length; i++) {
                if(data[i] != *(uint8_t *)(verifyAddress + i)) {
                    printf("CMD_VERIFY: Mismatch at offset %d. Expected 0x%02X, got 0x%02X.\n", i, data[i], *(uint8_t *)(verifyAddress + i));
                    status = 0x01; // Error
                    break;
                }
            }
            if(status == 0x00) {
                printf("CMD_VERIFY: Verification successful.\n");
            }
            verifyAddress += length;
            break;
        case CMD_END:
            printf("CMD_END: Ending operation and locking flash.\n");
            status = 0xBE;
            FLASH->CTLR |= ((uint32_t)0x00008000);
            FLASH->CTLR |= ((uint32_t)0x00000080);
            break;
        case CMD_JUMP:
            printf("CMD_JUMP: Jumping to application...\n");
            Delay_Ms(10);
            jumpToApp();
            break;
    }

    output[0] = status;
    outputSize = 1;
    response = true;
}

int main()
{
	SystemInit();

	funGpioInitA();
	funPinMode( PA3, GPIO_CFGLR_OUT_10Mhz_PP );

	HSUSBSetup();

	funDigitalWrite( PA3, FUN_LOW );

    printf("Using frequency %d MHz\n", FUNCONF_SYSTEM_CORE_CLOCK / 1000000);
    printf("Started USB\n");

	while(1)
	{
        printf("lrx = %d\n", lrx);
        Delay_Ms(500);
        if(lrx > 0) {
            // Handle the received data
            handleData();
        }
	}
}

/* Software exception handler */
void SW_Handler(void) __attribute((interrupt));
void SW_Handler(void) {
    //__asm("li  a6, 0x5000");
    //__asm("jr  a6");
    //while(1) {}
}

/* Unused USB methods */
void HandleHidUserReportDataOut( struct _USBState * ctx, uint8_t * data, int len )
{
}

int HandleHidUserReportDataIn( struct _USBState * ctx, uint8_t * data, int len )
{
	return len;
}

void HandleHidUserReportOutComplete( struct _USBState * ctx )
{
}

void HandleGotEPComplete( struct _USBState * ctx, int ep )
{
}