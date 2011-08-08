LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= LibCameraWrapper.cpp

LOCAL_SHARED_LIBRARIES:= libdl libutils libcutils libcamera_client

LOCAL_MODULE := libcamera
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

ifdef BOARD_SECOND_CAMERA_DEVICE
    LOCAL_CFLAGS += -DFFC_PRESENT
endif

include $(BUILD_SHARED_LIBRARY)

