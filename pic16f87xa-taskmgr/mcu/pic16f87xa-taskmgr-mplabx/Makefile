# pic16f87xa-taskmgr — MPLAB XC8 Makefile
#
# Builds the task manager + the multi-blink example on real silicon via
# MPLAB XC8. Tested with:
#   - MPLAB X IDE v6.x
#   - MPLAB XC8 C Compiler v3.10 (the `xc8-cc` driver)
#   - PIC16F877A  (and 873A / 874A / 876A — the example uses PORTB only)
#
# This Makefile mirrors the HAL's own mcu/pic16f87xa-mplabx/Makefile: the
# host/target split is done at build time, not with #ifdef — include/target
# is put ahead of include on the include path so pic16f87xa_platform.h
# resolves to the real-target (volatile-dereference) version, and the
# target-side harness / wdt_sleep / interrupt-vector implementations are
# linked. The host-only sim backend (pic16f87xa_sim.c) is simply absent from
# the source list.
#
# From the command line (no IDE):
#
#   export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
#   make MCU=16F877A            # default; also 873A / 874A / 876A
#   make clean
#
# To program the device with PICkit 2/3/4 or ICD, open MPLAB X and use the
# "Make and Program" button. The .hex here suits any external programmer.

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
# HAL_DIR points at the pic16f87xa-hal tree; APP_DIR at this task-manager
# tree. Adjust if the layout differs from the default sibling layout.
HAL_DIR ?= ../../../pic16f87xa-hal
APP_DIR ?= ../..
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/$(MCU)-multi-blink

# XC8 v3.x device family pack. Empty → the flag is omitted (use when
# the toolchain resolves devices without an explicit DFP, e.g. older XC8).
DFP_DIR ?= /opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC16Fxxx_DFP/xc8
ifneq ($(strip $(DFP_DIR)),)
  DFP_FLAG := -mdfp=$(DFP_DIR)
else
  DFP_FLAG :=
endif

CC     := xc8-cc
# include/target ahead of include so pic16f87xa_platform.h resolves to the
# real-target (volatile-dereference) version, not the host memory-backed one.
# The task-manager headers live under APP_DIR/include.
INC    := -I$(HAL_DIR)/include/target -I$(HAL_DIR)/include -I$(APP_DIR)/include
CFLAGS := $(DFP_FLAG) \
          -mcpu=$(shell echo $(MCU) | tr A-Z a-z) \
          -O2 -std=c99 -Wall -Wextra \
          $(CFLAGS_DEVICE) \
          $(INC) \
          -DFOSC_HZ=$(FOSC_HZ)

# ─────────────────────────── source layout ─────────────────────────
# The HAL peripheral/core sources compile for the target exactly as they do
# for the host; the sim/ backend and the *_sim.c harness are NOT compiled
# (XC8 never links pic16f87xa_sim.c). The task-manager source is added to
# the HAL set.
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
  $(HAL_DIR)/src/core/pic16f87xa_wdt_sleep.c       \
  $(HAL_DIR)/src/core/pic16f87xa_wdt_sleep_target.c \
  $(HAL_DIR)/src/core/pic16f87xa_isr_vector.c       \
  $(HAL_DIR)/src/core/pic16f87xa_irq_dispatch.c    \
  $(HAL_DIR)/src/core/pic16f87xa_harness_target.c

# PSP is only on 40/44-pin parts.
ifeq ($(filter $(MCU),16F874A 16F877A),$(MCU))
  HAL_SOURCES += $(HAL_DIR)/src/peripherals/pic16f87xa_psp.c
endif

# The task manager itself.
TASKMGR_SOURCES := $(APP_DIR)/src/task_manager.c

# The application: the multi-blink example.
APP_SOURCES := $(APP_DIR)/examples/example_multi_blink.c

# All real .c sources (the config word is generated separately, below).
SRCS := $(HAL_SOURCES) $(TASKMGR_SOURCES) $(APP_SOURCES)
# xc8-cc -c emits <name>.p1; link from the .p1 set.
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.p1,$(notdir $(SRCS)))

# Tell make where to find sources referenced by basename.
VPATH := $(sort $(dir $(SRCS)))

# ─────────────────────────── config word ───────────────────────────
# The PIC16F87XA Configuration Word is flash-locked; this file emits the
# directives — adjust per your hardware. See DS39582B §14.1 (Register 14-1).
CONFIG_SRC := $(BUILD_DIR)/config_$(MCU).c
CONFIG_OBJ := $(BUILD_DIR)/config_$(MCU).p1

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

# Compile a source to p-code. VPATH resolves the basename to the real path.
$(BUILD_DIR)/%.p1: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# The generated config word is its own translation unit.
$(CONFIG_OBJ): $(CONFIG_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Single link step over all .p1 intermediates → Intel HEX.
$(TARGET).hex: $(OBJS) $(CONFIG_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(CONFIG_OBJ) -o $@ -ginhx32
	@echo ""
	@echo "Built $@ — program with MPLAB X or your favorite programmer."

clean:
	rm -rf $(BUILD_DIR)