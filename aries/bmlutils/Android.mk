LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bmlwrite
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := releasetools/kernel-tools
LOCAL_SRC_FILES := bmlwrite.c
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libstdc++ libc
include $(BUILD_EXECUTABLE)

