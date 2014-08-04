LOCAL_PATH := $(call my-dir)

LIB_PATH := $(PREFIX)/lib
INCLUDE_PATH := $(PREFIX)/include

include $(CLEAR_VARS)
LOCAL_MODULE    := subsurface_jni
LOCAL_SRC_FILES := subsurface_jni.c
LOCAL_C_INCLUDES := $(INCLUDE_PATH)
LOCAL_WHOLE_STATIC_LIBRARIES := libsubsurface
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

$(call import-module,subsurface)
