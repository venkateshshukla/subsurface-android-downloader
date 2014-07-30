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

include $(CLEAR_VARS)
LOCAL_MODULE    := libsubsurface
LOCAL_SRC_FILES := dive.c device.c divelist.c planner.c sha1.c libdivecomputer.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/libxml2
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/../lib/libzip/include
LOCAL_CFLAGS := -std=gnu99
LOCAL_WHOLE_STATIC_LIBRARIES := libusb libftdi libdivecomputer
include $(BUILD_SHARED_LIBRARY)

