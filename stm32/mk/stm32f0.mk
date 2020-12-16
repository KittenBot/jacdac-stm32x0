SERIES = F0
CFLAGS += -mcpu=cortex-m0

#OPENOCD ?= openocd -f interface/stlink-v2-1.cfg -f target/stm32f0x.cfg
#OPENOCD ?= ./scripts/openocd -s ./scripts -f cmsis-dap.cfg -f stm32f0x.cfg
OPENOCD ?= openocd -f interface/cmsis-dap.cfg -f target/stm32f0x.cfg

HALPREF = stm32/stm32f0xx_hal_driver/Src
HALSRC =  \
$(HALPREF)/stm32f0xx_ll_adc.c \
$(HALPREF)/stm32f0xx_ll_comp.c \
$(HALPREF)/stm32f0xx_ll_crc.c \
$(HALPREF)/stm32f0xx_ll_crs.c \
$(HALPREF)/stm32f0xx_ll_dac.c \
$(HALPREF)/stm32f0xx_ll_dma.c \
$(HALPREF)/stm32f0xx_ll_exti.c \
$(HALPREF)/stm32f0xx_ll_gpio.c \
$(HALPREF)/stm32f0xx_ll_i2c.c \
$(HALPREF)/stm32f0xx_ll_pwr.c \
$(HALPREF)/stm32f0xx_ll_rcc.c \
$(HALPREF)/stm32f0xx_ll_rtc.c \
$(HALPREF)/stm32f0xx_ll_spi.c \
$(HALPREF)/stm32f0xx_ll_tim.c \
$(HALPREF)/stm32f0xx_ll_usart.c \
$(HALPREF)/stm32f0xx_ll_utils.c \

BMP ?= 1

PLATFORM = stm32

LD_FLASH_SIZE ?= $(FLASH_SIZE)

# TODO move some of this to common stm32.mk
AS_SRC = $(STARTUP_FILE)
CPPFLAGS += 	\
	-Istm32/stm32f0xx_hal_driver/Inc \
	-Istm32/cmsis_device_f0/Include \
	-Istm32/cmsis_core/Include
DEFINES += -DUSE_FULL_LL_DRIVER -DSTM32$(SERIES)
DEFINES += -D$(MCU) -DFLASH_SIZE="1024*$(FLASH_SIZE)" -DFLASH_PAGE_SIZE=$(PAGE_SIZE) -DBL_SIZE="1024*$(BL_SIZE)"

include stm32/mk/$(MCU).mk

CONFIG_DEPS += $(wildcard stm32/mk/*.mk)

LD_SCRIPT = $(BUILT)/linker.ld
$(BUILT)/linker.ld: $(wildcard stm32/mk/*.mk) Makefile
	mkdir -p $(BUILT)
	: > $@
	echo "MEMORY {" >> $@
	echo "RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = $(RAM_SIZE)K" >> $@
ifeq ($(BL),)
	echo "FLASH (rx)  : ORIGIN = 0x8000000, LENGTH = $(LD_FLASH_SIZE)K - $(BL_SIZE)K" >> $@
	echo "}" >> $@
	echo "INCLUDE ld/gcc_arm.ld" >> $@
else
	echo "FLASH (rx)  : ORIGIN = 0x8000000 + $(LD_FLASH_SIZE)K - $(BL_SIZE)K, LENGTH = $(BL_SIZE)K" >> $@
	echo "}" >> $@
	echo "INCLUDE ld/gcc_arm_bl_at_end.ld" >> $@
endif
