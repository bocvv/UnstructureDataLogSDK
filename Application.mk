APP_OPTIM := release
ANDROID_TOOLCHAIN=clang 
APP_ABI ?= armeabi-v7a arm64-v8a
APP_CPPFLAGS := -frtti -fexceptions -std=c++14
APP_PLATFORM := android-21
APP_STL := c++_static
#APP_STL := c++_shared
