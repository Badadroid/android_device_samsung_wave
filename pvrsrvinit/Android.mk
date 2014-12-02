LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := pvrsrvinit.c
LOCAL_LDFLAGS := -L vendor/samsung/wave/proprietary
LOCAL_LDLIBS := -lsrv_init -lsrv_um
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/
LOCAL_MODULE := pvrsrvinit
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES:=libc libdl libcutils

include $(BUILD_EXECUTABLE)