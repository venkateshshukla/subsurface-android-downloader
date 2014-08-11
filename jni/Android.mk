LOCAL_PATH := $(call my-dir)

LIB_PATH := $(PREFIX)/lib
INCLUDE_PATH := $(PREFIX)/include

include $(CLEAR_VARS)
LOCAL_MODULE := usb
LOCAL_SRC_FILES := $(LIB_PATH)/libusb-1.0.a
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ftdi
LOCAL_SRC_FILES := $(LIB_PATH)/libftdi1.a
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := divecomputer
LOCAL_SRC_FILES := $(LIB_PATH)/libdivecomputer.a
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := universal
LOCAL_SRC_FILES := utils.c common.c universal.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_CFLAGS += -std=gnu99
LOCAL_WHOLE_STATIC_LIBRARIES := usb ftdi divecomputer
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libdivecomputer_jni
LOCAL_SRC_FILES := libdivecomputer_jni.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_LDFLAGS := -llog
LOCAL_WHOLE_STATIC_LIBRARIES := universal divecomputer
include $(BUILD_SHARED_LIBRARY)

