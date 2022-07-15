TARGETS := firmware_d firmware # debug, release and cycleleds targets (the latest is a release build with some settings turned on to help with dice manufacturing)
OUTPUT_DIRECTORY := _build
PUBLISH_DIRECTORY := binaries
PROJ_DIR := .
SDK_VER := 17

# Percent character is escaped in some versions of make when using the shell commands below
PERCENT := %
# Generate build timestamp
BUILD_TIMESTAMP := $(shell python -c 'import time; print(round(time.time()))')
# Format timestamp like this: 2022-07-14T0923+0200
# No semicolon between hours and minutes because the date/time string is used in filenames.
# This format is ISO 8601 conformant.
BUILD_DATE_TIME := $(shell python -c 'from datetime import datetime; \
    tzinfo = datetime.utcnow().astimezone().tzinfo; \
	dt = datetime.fromtimestamp($(BUILD_TIMESTAMP), tz=tzinfo); \
	print(dt.strftime("$(PERCENT)Y-$(PERCENT)m-$(PERCENT)dT$(PERCENT)H$(PERCENT)M$(PERCENT)z")) \
	')

# Different accelerometer hw for compiling old boards, uncomment to compile its source
# Can override the default in cmd line by calling "make ACCEL_HW='*name*' *target*"
# ACCEL_HW = lis2de12		#	D20v9.3, D20v9.1, D20v8, D20v5, D6, D20v1, DevD20
# ACCEL_HW = mxc4005xc		#	D20v10, D20v9.4
ACCEL_HW := kxtj3-1057

# To prevent downgrading the firmware, set NRF_DFU_APP_DOWNGRADE_PREVENTION to 1 in bootloader config.h
# The bootloader can only be upgraded (applying an update with a higher version number).
FW_VER := 0x100
BL_VER := 0x100

# SDK 17 path
SDK_ROOT := C:/nRF5_SDK

# SoftDevice image filename and path
# Download latest SoftDevice here: https://www.nordicsemi.com/Products/Development-software/s112/download
# but do not update the SDK header files!
#SOFTDEVICE_HEX_FILE := s112_nrf52_7.0.1_softdevice.hex
#SOFTDEVICE_HEX_FILE := s112_nrf52_7.2.0_softdevice.hex
SOFTDEVICE_HEX_FILE := s112_nrf52_7.3.0_softdevice.hex
SOFTDEVICE_HEX_PATHNAME := $(SDK_ROOT)/components/softdevice/s112/hex/$(SOFTDEVICE_HEX_FILE)
SD_REQ_ID := 0xCD,0x103,0x126
# Any version: 0xFFFE

# Bootloader image filename and path
BOOTLOADER_HEX_FILE := nrf52810_xxaa_s112.hex
BOOTLOADER_HEX_PATHNAME := $(PROJ_DIR)/../DiceBootloader/_build/$(BOOTLOADER_HEX_FILE)

# Filename for the zip file used for DFU over Bluetooth
ZIP_FILE := firmware_$(BUILD_DATE_TIME)_sdk$(SDK_VER).zip
ZIPBL_FILE := bootloader_$(BUILD_DATE_TIME)_sdk$(SDK_VER).zip
VALIDATION_ZIP := firmware_validation_$(BUILD_DATE_TIME)_sdk$(SDK_VER).zip

LINKER_SCRIPT := Firmware.ld

# Default target = first one defined
.PHONY: default
default: firmware_debug
 
# Source files common to all targets
SRC_FILES += \
	$(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52810.S \
	$(SDK_ROOT)/modules/nrfx/mdk/system_nrf52810.c \
	$(SDK_ROOT)/components/libraries/util/app_error_weak.c \
	$(SDK_ROOT)/components/ble/ble_advertising/ble_advertising.c \
	$(SDK_ROOT)/components/ble/ble_services/ble_dfu/ble_dfu.c \
	$(SDK_ROOT)/components/ble/ble_services/ble_dfu/ble_dfu_unbonded.c \
	$(SDK_ROOT)/components/ble/common/ble_advdata.c \
	$(SDK_ROOT)/components/ble/common/ble_conn_params.c \
	$(SDK_ROOT)/components/ble/common/ble_conn_state.c \
	$(SDK_ROOT)/components/ble/common/ble_srv_common.c \
	$(SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
	$(SDK_ROOT)/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
	$(SDK_ROOT)/components/ble/peer_manager/gatt_cache_manager.c \
	$(SDK_ROOT)/components/ble/peer_manager/gatts_cache_manager.c \
	$(SDK_ROOT)/components/ble/peer_manager/id_manager.c \
	$(SDK_ROOT)/components/boards/boards.c \
	$(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
	$(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
	$(SDK_ROOT)/components/libraries/atomic_flags/nrf_atflags.c \
	$(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
	$(SDK_ROOT)/components/libraries/bootloader/dfu/nrf_dfu_svci.c \
	$(SDK_ROOT)/components/libraries/bsp/bsp.c \
	$(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
	$(SDK_ROOT)/components/libraries/fds/fds.c \
	$(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
	$(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
	$(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_uart.c \
	$(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
	$(SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
	$(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
	$(SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
	$(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
	$(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
	$(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
	$(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
	$(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
	$(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
	$(SDK_ROOT)/components/libraries/timer/app_timer.c \
	$(SDK_ROOT)/components/libraries/util/app_util_platform.c \
	$(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
	$(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
	$(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
	$(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
	$(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
	$(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
	$(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_twi.c \
	$(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_uart.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_power.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_pwm.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uarte.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_wdt.c \
	$(SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
	$(SDK_ROOT)/modules/nrfx/hal/nrf_nvmc.c \
	$(PROJ_DIR)/src/die_init.cpp \
	$(PROJ_DIR)/src/die_main.cpp \
	$(PROJ_DIR)/src/animations/animation.cpp \
	$(PROJ_DIR)/src/animations/animation_simple.cpp \
	$(PROJ_DIR)/src/animations/animation_gradient.cpp \
	$(PROJ_DIR)/src/animations/animation_keyframed.cpp \
	$(PROJ_DIR)/src/animations/animation_rainbow.cpp \
	$(PROJ_DIR)/src/animations/animation_gradientpattern.cpp \
	$(PROJ_DIR)/src/animations/animation_noise.cpp \
	$(PROJ_DIR)/src/animations/animation_cycle.cpp \
	$(PROJ_DIR)/src/animations/animation_name.cpp \
	$(PROJ_DIR)/src/animations/blink.cpp \
	$(PROJ_DIR)/src/animations/keyframes.cpp \
	$(PROJ_DIR)/src/behaviors/action.cpp \
	$(PROJ_DIR)/src/behaviors/condition.cpp \
	$(PROJ_DIR)/src/bluetooth/bluetooth_stack.cpp \
	$(PROJ_DIR)/src/bluetooth/bluetooth_messages.cpp \
	$(PROJ_DIR)/src/bluetooth/bluetooth_message_service.cpp \
	$(PROJ_DIR)/src/bluetooth/bulk_data_transfer.cpp \
	$(PROJ_DIR)/src/bluetooth/telemetry.cpp \
	$(PROJ_DIR)/src/config/board_config.cpp \
	$(PROJ_DIR)/src/config/settings.cpp \
	$(PROJ_DIR)/src/config/dice_variants.cpp \
	$(PROJ_DIR)/src/data_set/data_animation_bits.cpp \
	$(PROJ_DIR)/src/data_set/data_set.cpp \
	$(PROJ_DIR)/src/data_set/data_set_defaults.cpp \
	$(PROJ_DIR)/src/drivers_hw/apa102.cpp \
	$(PROJ_DIR)/src/drivers_hw/battery.cpp \
	$(PROJ_DIR)/src/drivers_hw/neopixel.cpp \
	$(PROJ_DIR)/src/drivers_nrf/a2d.cpp \
	$(PROJ_DIR)/src/drivers_nrf/dfu.cpp \
	$(PROJ_DIR)/src/drivers_nrf/flash.cpp \
	$(PROJ_DIR)/src/drivers_nrf/gpiote.cpp \
	$(PROJ_DIR)/src/drivers_nrf/i2c.cpp \
	$(PROJ_DIR)/src/drivers_nrf/log.cpp \
	$(PROJ_DIR)/src/drivers_nrf/power_manager.cpp \
	$(PROJ_DIR)/src/drivers_nrf/scheduler.cpp \
	$(PROJ_DIR)/src/drivers_nrf/timers.cpp \
	$(PROJ_DIR)/src/drivers_nrf/watchdog.cpp \
	$(PROJ_DIR)/src/modules/accelerometer.cpp \
	$(PROJ_DIR)/src/modules/anim_controller.cpp \
	$(PROJ_DIR)/src/modules/animation_preview.cpp \
	$(PROJ_DIR)/src/modules/behavior_controller.cpp \
	$(PROJ_DIR)/src/modules/led_color_tester.cpp \
	$(PROJ_DIR)/src/modules/leds.cpp \
	$(PROJ_DIR)/src/modules/battery_controller.cpp \
	$(PROJ_DIR)/src/modules/hardware_test.cpp \
	$(PROJ_DIR)/src/modules/rssi_controller.cpp \
	$(PROJ_DIR)/src/modules/validation_manager.cpp \
	$(PROJ_DIR)/src/utils/abi.cpp \
	$(PROJ_DIR)/src/utils/rainbow.cpp \
	$(PROJ_DIR)/src/utils/utils.cpp \
	$(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twi.c \
	$(PROJ_DIR)/src/drivers_hw/$(ACCEL_HW).cpp 
	

# Include folders common to all targets
INC_FOLDERS += \
	$(PROJ_DIR) \
	$(PROJ_DIR)/src \
	$(PROJ_DIR)/src/config \
	$(SDK_ROOT) \
	$(SDK_ROOT)/modules/nrfx \
	$(SDK_ROOT)/modules/nrfx/hal \
	$(SDK_ROOT)/modules/nrfx/mdk \
	$(SDK_ROOT)/modules/nrfx/soc \
	$(SDK_ROOT)/modules/nrfx/drivers/include \
	$(SDK_ROOT)/components/ble/common \
	$(SDK_ROOT)/components/ble/ble_advertising \
	$(SDK_ROOT)/components/ble/ble_services \
	$(SDK_ROOT)/components/ble/ble_services/ble_lbs \
	$(SDK_ROOT)/components/ble/ble_services/ble_dfu \
	$(SDK_ROOT)/components/ble/nrf_ble_gatt \
	$(SDK_ROOT)/components/ble/nrf_ble_qwr \
	$(SDK_ROOT)/components/ble/peer_manager \
	$(SDK_ROOT)/components/boards \
	$(SDK_ROOT)/components/softdevice/common \
	$(SDK_ROOT)/components/softdevice/mbr/headers \
	$(SDK_ROOT)/components/softdevice/s112/headers \
	$(SDK_ROOT)/components/libraries \
	$(SDK_ROOT)/components/libraries/bootloader \
	$(SDK_ROOT)/components/libraries/bootloader/dfu \
	$(SDK_ROOT)/components/libraries/bootloader/ble_dfu \
	$(SDK_ROOT)/components/libraries/atomic_flags \
	$(SDK_ROOT)/components/libraries/util \
	$(SDK_ROOT)/components/libraries/bsp \
	$(SDK_ROOT)/components/libraries/button \
	$(SDK_ROOT)/components/libraries/fds \
	$(SDK_ROOT)/components/libraries/fstorage \
	$(SDK_ROOT)/components/libraries/memobj \
	$(SDK_ROOT)/components/libraries/ringbuf \
	$(SDK_ROOT)/components/libraries/balloc \
	$(SDK_ROOT)/components/libraries/strerror \
	$(SDK_ROOT)/components/libraries/log \
	$(SDK_ROOT)/components/libraries/log/src \
	$(SDK_ROOT)/components/libraries/atomic \
	$(SDK_ROOT)/components/libraries/atomic_fifo \
	$(SDK_ROOT)/components/libraries/mutex \
	$(SDK_ROOT)/components/libraries/timer \
	$(SDK_ROOT)/components/libraries/delay \
	$(SDK_ROOT)/components/libraries/pwr_mgmt \
	$(SDK_ROOT)/components/libraries/experimental_section_vars \
	$(SDK_ROOT)/components/libraries/scheduler \
	$(SDK_ROOT)/components/libraries/sensorsim \
	$(SDK_ROOT)/components/libraries/svc \
	$(SDK_ROOT)/components/toolchain/cmsis/include \
	$(SDK_ROOT)/integration/nrfx \
	$(SDK_ROOT)/integration/nrfx/legacy \
	$(SDK_ROOT)/external/fprintf \
	$(SDK_ROOT)/external/segger_rtt \
	# $(RTOS_DIR)/Source/include \
	# $(RTOS_DIR)/config \
	# $(RTOS_DIR)/portable/GCC/nrf52 \
	# $(RTOS_DIR)/portable/CMSIS/nrf52 \
	# $(ARDUINO_CORE) \
	# $(ARDUINO_CORE)/cmsis/include \
	# $(ARDUINO_CORE)/sysview/SEGGER \
	# $(ARDUINO_CORE)/sysview/Config \
	# $(ARDUINO_CORE)/usb \
	# $(ARDUINO_CORE)/usb/tinyusb/src \
	# $(ARDUINO_ROOT)/libraries/Wire \
	# $(ARDUINO_ROOT)/libraries/SPI \
	# $(ARDUINO_ROOT)/

# Libraries common to all targets
LIB_FILES += \

# Optimization flags
OPT = -Os -g3
#OPT = -O0 -g3
# Uncomment the line below to enable link time optimization
OPT += -flto

COMMON_FLAGS = -DBL_SETTINGS_ACCESS_ONLY
COMMON_FLAGS += -DNRF52_SERIES
COMMON_FLAGS += -DBOARD_CUSTOM
COMMON_FLAGS += -DCONFIG_GPIO_AS_PINRESET
COMMON_FLAGS += -DFLOAT_ABI_SOFT
COMMON_FLAGS += -DNRF52810_XXAA
COMMON_FLAGS += -DS112
COMMON_FLAGS += -DSOFTDEVICE_PRESENT
COMMON_FLAGS += -DSWI_DISABLE0
COMMON_FLAGS += -mcpu=cortex-m4
COMMON_FLAGS += -mthumb -mabi=aapcs
COMMON_FLAGS += -mfloat-abi=soft
COMMON_FLAGS += -DNRF52_PAN_74
COMMON_FLAGS += -DNRF_DFU_SVCI_ENABLED
COMMON_FLAGS += -DNRF_DFU_TRANSPORT_BLE=1
COMMON_FLAGS += -DSDK_VER=$(SDK_VER)
COMMON_FLAGS += -DBUILD_TIMESTAMP=$(BUILD_TIMESTAMP)

# COMMON_FLAGS += -DDEVELOP_IN_NRF52832

# Storage address: first available page in flash memory for storing app data at runtime.
# It's the FLASH start address + the app size rounded to the next 4k bytes (size of a page = 0x1000).
# The start address depends on the SoftDevice, 0x19000 in our case.
# https://devzone.nordicsemi.com/guides/short-range-guides/b/getting-started/posts/adjustment-of-ram-and-flash-memory
FSTORAGE_ADDR = 0x26000 # 0x19000 + 0xD000 (max app size = 53248 bytes = 52 kB)

# Debug builds are bigger, but the bootloader is not present so we can use higher addresses
firmware_debug: FSTORAGE_ADDR = 0x2E000 

COMMON_FLAGS += -DFSTORAGE_START=$(FSTORAGE_ADDR)

# Debug flags
DEBUG_FLAGS = -DNRF_LOG_ENABLED=0
DEBUG_FLAGS += -DDICE_SELFTEST=0

firmware_debug: DEBUG_FLAGS = -DDEBUG
firmware_debug: DEBUG_FLAGS += -DDEBUG_NRF
firmware_debug: DEBUG_FLAGS += -DNRF_LOG_ENABLED=1

COMMON_FLAGS += $(DEBUG_FLAGS)

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += $(COMMON_FLAGS)
CFLAGS += -Wall
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums

# C++ flags common to all targets
CXXFLAGS += $(OPT)
CXXFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-threadsafe-statics

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += $(COMMON_FLAGS)
ASMFLAGS += -D_CONSOLE

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs
#LDFLAGS += -u _printf_float

STACK_SIZE := 2048
HEAP_SIZE := 5800
firmware_debug: HEAP_SIZE := 3000

CFLAGS += -D__HEAP_SIZE=$(HEAP_SIZE)
CFLAGS += -D__STACK_SIZE=$(STACK_SIZE)
ASMFLAGS += -D__HEAP_SIZE=$(HEAP_SIZE)
ASMFLAGS += -D__STACK_SIZE=$(STACK_SIZE)

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

# Generate build command for each target
TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc
include $(TEMPLATE_PATH)/Makefile.common
$(foreach target, $(TARGETS), $(call define_target, $(target)))

# Settings
SETTINGS_FLAGS := --family NRF52810 --application-version $(FW_VER) --bootloader-version $(BL_VER) --bl-settings-version 1

settings_d: firmware_d
	nrfutil settings generate $(SETTINGS_FLAGS) --application $(OUTPUT_DIRECTORY)/firmware_d.hex $(OUTPUT_DIRECTORY)/firmware_settings_d.hex

settings: firmware
	nrfutil settings generate $(SETTINGS_FLAGS) --application $(OUTPUT_DIRECTORY)/firmware.hex $(OUTPUT_DIRECTORY)/firmware_settings.hex

#
# Common commands
#

.PHONY: reset
reset:
	nrfjprog -f nrf52 --reset

.PHONY: hardreset
hardreset:
	nrfjprog -f nrf52 --pinreset

.PHONY: erase
erase:
	nrfjprog -f nrf52 --eraseall

.PHONY: flash_softdevice
flash_softdevice:
	@echo ==== Flashing: $(SOFTDEVICE_HEX_FILE) ====
	nrfjprog -f nrf52 --program $(SOFTDEVICE_HEX_PATHNAME) --sectorerase --verify --reset

.PHONY: flash_bootloader
flash_bootloader:
	@echo ==== Flashing: $(BOOTLOADER_HEX_FILE) ====
	nrfjprog -f nrf52 --program $(BOOTLOADER_HEX_PATHNAME) --sectorerase --verify --reset

#
# Debug commands
#

.PHONY: firmware_debug
firmware_debug: firmware_d

.PHONY: settings_debug
settings_debug: settings_d

# Flash the program
.PHONY: flash
flash: firmware_debug settings_debug
	@echo ==== Flashing: $(OUTPUT_DIRECTORY)/firmware_d.hex ====
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/firmware_d.hex --sectorerase --verify
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/firmware_settings_d.hex --sectorerase --verify --reset

.PHONY: reflash
reflash: erase flash_softdevice flash

#
# Release commands
#

.PHONY: firmware_release
firmware_release: firmware

.PHONY: settings_release
settings_release: settings

# Flash the program
.PHONY: flash_release
flash_release: firmware_release settings_release
	@echo ==== Flashing: $(OUTPUT_DIRECTORY)/firmware.hex ====
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/firmware.hex --sectorerase --verify
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/firmware_settings.hex --sectorerase --verify --reset

.PHONY: reflash_release
reflash_release: erase flash_softdevice flash_release

.PHONY: flash_board
flash_board: erase flash_softdevice flash_bootloader flash_release

# Flash over BLE, you must use DICE=D_XXXXXXX argument to make flash_ble
# e.g. make flash_ble DICE=D_71902510
.PHONY: flash_ble
flash_ble: zip
	@echo Flashing: $(ZIP_FILE) over BLE DFU
	nrfutil dfu ble -cd 0 -ic NRF52 -p COM5 -snr 680120179 -f -n $(DICE) -pkg $(OUTPUT_DIRECTORY)/$(ZIP_FILE)

#
# Validation commands
#

# UICR_bit.hex file uses the intel hex format:
# https://www.intel.com/content/www/us/en/support/programmable/articles/000076770.html
# Each byte following the ':' follows this format:
# Record length (1B), load addr (2B), record type (1B - types outlined in above documentation),
# 	data (number of bytes specified in record length), checksum (2's complement of sum of all bytes)

.PHONY: validation_bit
validation_bit: 
	@echo ===== Writing validation bit =====
	nrfjprog --memwr 0x10001080 --val 0xFFFFFFFE

.PHONY: exit_validation_bit
exit_validation_bit: 
	@echo ===== Writing exit validation bit =====
	nrfjprog --memwr 0x10001080 --val 0xFFFFFFFC

.PHONY: hex_validation
hex_validation: hex_release
	mergehex -m $(OUTPUT_DIRECTORY)/full_firmware.hex UICR_ValidationModeEnabled.hex -o $(OUTPUT_DIRECTORY)/full_firmware_validation.hex

.PHONY: flash_validation
flash_validation: erase hex_validation
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/full_firmware_validation.hex --chiperase --verify --reset

.PHONY: zip_validation
zip_validation: hex_validation
	nrfutil pkg generate --application $(OUTPUT_DIRECTORY)/full_firmware_validation.hex --application-version $(FW_VER) --hw-version 52 --key-file private.pem --sd-req $(SD_REQ_ID) $(OUTPUT_DIRECTORY)/$(VALIDATION_ZIP)

#
# Publishing commands
#

.PHONY: hex_release
hex_release: firmware_release settings_release
	mergehex -m $(OUTPUT_DIRECTORY)/firmware.hex $(OUTPUT_DIRECTORY)/firmware_settings.hex $(SOFTDEVICE_HEX_PATHNAME) $(BOOTLOADER_HEX_PATHNAME) -o $(OUTPUT_DIRECTORY)/full_firmware.hex

.PHONY: zip
zip: firmware_release
	nrfutil pkg generate --application $(OUTPUT_DIRECTORY)/firmware.hex --application-version $(FW_VER) --hw-version 52 --key-file private.pem --sd-req $(SD_REQ_ID) $(OUTPUT_DIRECTORY)/$(ZIP_FILE)

.PHONY: zipbl
zipbl:
	nrfutil pkg generate --bootloader $(BOOTLOADER_HEX_PATHNAME) --bootloader-version $(BL_VER) --hw-version 52 --key-file private.pem --sd-req 0x103 $(OUTPUT_DIRECTORY)/$(ZIPBL_FILE)

# Be sure to use a backslash in the pathname, otherwise the copy command will fail (in CMD environment) 
.PHONY: publish
publish: zip
	copy $(OUTPUT_DIRECTORY)\$(ZIP_FILE) $(PUBLISH_DIRECTORY)
