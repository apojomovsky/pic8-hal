# PIC16F87XA HAL — MPLAB XC8 Makefile
#
# Builds the HAL on real silicon via MPLAB XC8.
# Tested with:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 Compiler v2.40
#   - PIC16F877A
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v2.40/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# To program the device with PICkit 2/3/4 or ICD, open MPLAB X
# and use the "Make and Program" button. The .hex output here
# is suitable for any external programmer (PK2CMD, ippe, MPLAB IPE).

# ─────────────────────────── device selection ─────────────────────
MCU     ?= 16F877A
FOSC_HZ ?= 20000000

ifeq ($(MCU),16F873A)
  CFLAGS_DEVICE := -DPIC16F873A
else ifeq ($(MCU),16F874A)
  CFLAGS_DEVICE := -DPIC16F874A
else ifeq ($(MCU),16F876A)
  CFLAGS_DEVICE := -DPIC16F876A
else ifeq ($(MCU),16F877A)
  CFLAGS_DEVICE := -DPIC16F877A
else
  $(error Unsupported MCU=$(MCU); choose 873A, 874A, 876A or 877A)
endif

# ─────────────────────────── paths ────────────────────────────────
# Adjust HAL_DIR if the firmware project is not a sibling of pic16f87xa-hal.
HAL_DIR ?= ../..
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/$(MCU)-firmware

CC     := xc8-cc
INC    := -I$(HAL_DIR)/include
CFLAGS := -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The HAL sources in src/ compile for both host (sim) and target.
# The sim/ subdirectory is NOT compiled on real targets — its
# PIC16F87XA_USE_SIMULATOR define is never set, so pic16f87xa_sim.c
# will resolve to the real-target direct-access path.

HAL_SOURCES := \
  $(HAL_DIR)/src/peripherals/pic16f87xa_gpio.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer0.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer1.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_timer2.c   \
  $(HAL_DIR)/src/peripherals/pic16f87xa_ccp.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_usart.c    \
  $(HAL_DIR)/src/peripherals/pic16f87xa_ssp.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_adc.c      \
  $(HAL_DIR)/src/peripherals/pic16f87xa_comp.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_vref.c     \
  $(HAL_DIR)/src/peripherals/pic16f87xa_eeprom.c   \
  $(HAL_DIR)/src/core/pic16f87xa_interrupt.c       \
  $(HAL_DIR)/src/core/pic16f87xa_wdt_sleep.c

# PSP is only on 40/44-pin parts.
ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  HAL_SOURCES += $(HAL_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# Add your application sources here. example_blink is the canonical
# "turn an LED on" smoke test. Replace it with your project's main.c.
APP_SOURCES := $(HAL_DIR)/tests/example_blink.c

SRCS := $(CONFIG_SRC) $(HAL_SOURCES) $(APP_SOURCES)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# The PIC16F87XA Configuration Word is a flash-locked register that
# defines oscillator, WDT, PWRT, BOR, code protection, and LVP at
# programming time.  This file emits the directives — adjust as
# needed per your hardware.  See DS39582B §14.1 (Register 14-1).
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c

# ─────────────────────────── targets ──────────────────────────────
.PHONY: all clean
all: $(TARGET).hex

$(BUILD_DIR):
	mkdir -p $@

$(CONFIG_SRC): | $(BUILD_DIR)
	@printf '/* Auto-generated. Do not edit. */\n' > $@
	@printf '#include <xc.h>\n' >> $@
	@printf '#pragma config FOSC = HS\n' >> $@
	@printf '#pragma config WDTE = ON\n' >> $@
	@printf '#pragma config PWRTE = ON\n' >> $@
	@printf '#pragma config BOREN = ON\n' >> $@
	@printf '#pragma config LVP = OFF\n' >> $@
	@printf '#pragma config WRT = OFF\n' >> $@

$(BUILD_DIR)/%.o: %.c $(CONFIG_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(TARGET).hex: $(TARGET).elf
	$(CC) $(CFLAGS) $< -ohex:$(TARGET).hex
	@echo ""
	@echo "Built $(TARGET).hex — program with MPLAB X or your favorite programmer."

clean:
	rm -rf $(BUILD_DIR)