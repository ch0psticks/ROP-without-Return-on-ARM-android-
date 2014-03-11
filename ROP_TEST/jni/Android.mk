LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ROP_TEST
LOCAL_SRC_FILES := ROP_TEST.cpp

include $(BUILD_SHARED_LIBRARY)
