LOCAL_PATH := $(call my-dir)
LIB_PATH := ../crossbuild/ndk-$(ARCH)/sysroot/usr/lib
INCLUDE_PATH := $(LOCAL_PATH)/../crossbuild/ndk-$(ARCH)/sysroot/usr/include

include $(CLEAR_VARS)
LOCAL_MODULE    := libusb
LOCAL_SRC_FILES := $(LIB_PATH)/libusb-1.0.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libftdi
LOCAL_SRC_FILES := $(LIB_PATH)/libftdi1.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libdivecomputer
LOCAL_SRC_FILES := $(LIB_PATH)/libdivecomputer.a
include $(PREBUILT_STATIC_LIBRARY)

