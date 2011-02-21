ifeq ($(filter-out s5pv210,$(TARGET_BOARD_PLATFORM)),)
    include $(call my-dir)/aries/Android.mk
endif
