LOCAL_PATH := $(call my-dir)

LIB_PATH := $(PREFIX)/lib
INCLUDE_PATH := $(PREFIX)/include
SUBSURFACE_PATH := $(LOCAL_PATH)/../crossbuild/subsurface
ANDROID_LIB_DIR := ../libs/$(TARGET_ARCH_ABI)

include $(CLEAR_VARS)
LOCAL_MODULE    := libsqlite
LOCAL_SRC_FILES := $(LIB_PATH)/libsqlite3.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libz
LOCAL_SRC_FILES := $(LIB_PATH)/libz.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libzip
LOCAL_SRC_FILES := $(LIB_PATH)/libzip.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libexslt
LOCAL_SRC_FILES := $(LIB_PATH)/libexslt.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libxslt
LOCAL_SRC_FILES := $(LIB_PATH)/libxslt.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libxml
LOCAL_SRC_FILES := $(LIB_PATH)/libxml2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libgit
LOCAL_SRC_FILES := $(LIB_PATH)/libgit2.a
include $(PREBUILT_STATIC_LIBRARY)

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
LOCAL_MODULE	:= libsubsurface
LOCAL_SRC_FILES	:= $(ANDROID_LIB_DIR)/libsubsurface_jni.a
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/libxml2
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/../lib/libzip/include
LOCAL_WHOLE_STATIC_LIBRARIES := libsqlite libz libzip libexslt libxslt libxml
LOCAL_WHOLE_STATIC_LIBRARIES += libgit libusb libftdi libdivecomputer
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := subsurface_jni
LOCAL_SRC_FILES := subsurface_jni.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_C_INCLUDES += $(SUBSURFACE_PATH)
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/libxml2
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/../lib/libzip/include
LOCAL_WHOLE_STATIC_LIBRARIES += libsubsurface
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

