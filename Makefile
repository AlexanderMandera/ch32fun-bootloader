all : flash

TARGET:=ch32fun-bootloader
TARGET_MCU:=CH32V305
TARGET_MCU_PACKAGE:=CH32V305RBT6
ENABLE_FPU:=0
LINKER_SCRIPT=ch32fun-bootloader.ld

CH32FUN:=ch32fun/ch32fun
MINICHLINK:=ch32fun/minichlink

include ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
