LOCAL_PATH := $(call my-dir)

LIB_PATH := $(PREFIX)/lib
INCLUDE_PATH := $(PREFIX)/include
SUBSURFACE_PATH := $(LOCAL_PATH)/../crossbuild/subsurface

include $(CLEAR_VARS)
LOCAL_MODULE    := subsurface_jni
LOCAL_SRC_FILES := subsurface_jni.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_C_INCLUDES += $(SUBSURFACE_PATH)
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/libxml2
LOCAL_C_INCLUDES += $(INCLUDE_PATH)/../lib/libzip/include
LOCAL_WHOLE_STATIC_LIBRARIES := libsubsurface
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

$(call import-module,subsurface)
