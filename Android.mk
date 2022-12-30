LOCAL_PATH := $(call my-dir)

OpenCV_BASE = $(LOCAL_PATH)/dep/OpenCV-android-sdk

COMMON_BASE = $(LOCAL_PATH)/com
ENGINE_BASE = $(LOCAL_PATH)/src

include $(CLEAR_VARS)

OPENCV_INSTALL_MODULES := on
OPENCV_LIB_TYPE := STATIC
#OPENCV_LIB_TYPE := SHARED
#include $(OpenCV_BASE)/sdk/native/jni/OpenCV.mk
OPENCV_INCLUDE_DIR = $(OpenCV_BASE)/sdk/native/jni/include
$(warning "opencv include dir $(OPENCV_INCLUDE_DIR)")

#LOCAL_C_INCLUDES += $(OPENCV_INCLUDE_DIR)
LOCAL_C_INCLUDES += $(COMMON_BASE)
LOCAL_C_INCLUDES += $(ENGINE_BASE)

LOCAL_SRC_FILES := $(COMMON_BASE)/cJSON.cpp \
                   $(ENGINE_BASE)/AladdinSDKLog.cpp \
                   $(ENGINE_BASE)/AladdinSDKLogPack.cpp \
                   $(ENGINE_BASE)/AladdinSDKLogComm.cpp $(ENGINE_BASE)/AladdinSDKLogDrop.cpp

LOCAL_LDLIBS := #-landroid -llog -ldl -lz
LOCAL_CFLAGS   := -D_FORTIFY_SOURCE=2 -O2 -fvisibility=hidden -fomit-frame-pointer -fstrict-aliasing -ffunction-sections -fdata-sections -ffast-math -ftree-vectorize -fPIC -Ofast -ffast-math -w -std=c++14
LOCAL_CPPFLAGS := -D_FORTIFY_SOURCE=2 -O2 -fvisibility=hidden -fvisibility-inlines-hidden -fomit-frame-pointer -fstrict-aliasing -ffunction-sections -fdata-sections -ffast-math -fPIC -Ofast -ffast-math -std=c++14
LOCAL_LDFLAGS  += -Wl,--gc-sections
#LOCAL_CFLAGS   += -fopenmp
#LOCAL_CPPFLAGS += -fopenmp
#LOCAL_LDFLAGS  += -fopenmp

LOCAL_ARM_NEON := true

#APP_STL := c++_static
#APP_STL := c++_shared

APP_ALLOW_MISSING_DEPS = false

#LOCAL_STATIC_LIBRARIES := opencv_imgproc opencv_imgcodecs opencv_core cpufeatures tegra_hal tbb ittnotify
LOCAL_SHARED_LIBRARIES :=
LOCAL_STATIC_LIBRARIES :=
#LOCAL_SHARED_LIBRARIES := c++_shared opencv_java4

LOCAL_MODULE     := AladdinSDKLog

include $(BUILD_SHARED_LIBRARY)
