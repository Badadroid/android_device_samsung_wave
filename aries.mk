ifeq ($(filter-out aries,$(TARGET_BOOTLOADER_BOARD_NAME)),)
    include device/samsung/common/aries/Android.mk
endif
