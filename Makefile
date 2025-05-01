all : flash

TARGET:=ch32fun-bootloader

TARGET_MCU?=CH32V003
CH32FUN:=ch32fun/ch32fun
MINICHLINK:=ch32fun/minichlink
include ch32fun/ch32fun/ch32fun.mk

flash : cv_flash
clean : cv_clean
