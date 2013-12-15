# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := $(PLATFORM_PREFIX)/lib/libfreetype.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := glyphy
LOCAL_SRC_FILES := $(PLATFORM_PREFIX)/lib/libglyphy.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := glut
LOCAL_SRC_FILES := $(PLATFORM_PREFIX)/lib/libfreeglut-gles2.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := glyphy-demo
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	../../matrix4x4.c \
	../../trackball.c \
	../../demo-atlas.cc \
	../../demo-buffer.cc \
	../../demo-font.cc \
	../../demo-glstate.cc \
	../../demo-shader.cc \
	../../demo-view.cc \
	../../glyphy-demo.cc \
	$(NULL)
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv2 -lz
LOCAL_WHOLE_STATIC_LIBRARIES := gnustl_static freetype glyphy glut
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
