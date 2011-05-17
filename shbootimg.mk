INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): device/samsung/aries-common/mkshbootimg.py \
$(recovery_ramdisk) \
$(INSTALLED_RAMDISK_TARGET) \
$(TARGET_PREBUILT_KERNEL)
$(call pretty,"Boot image: $@")
$(hide) ./device/samsung/aries-common/mkshbootimg.py $@ $(TARGET_PREBUILT_KERNEL) $(INSTALLED_RAMDISK_TARGET) $(recovery_ramdisk)

INSTALLED_RECOVERYIMAGE_TARGET :=
