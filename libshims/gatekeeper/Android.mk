LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    gatekeeper_shim.cpp
LOCAL_SHARED_LIBRARIES := libgatekeeper liblog
LOCAL_MODULE := libshim_gatekeeper
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_VENDOR_MODULE := true
include $(BUILD_SHARED_LIBRARY)
