LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := modmenu
LOCAL_SRC_FILES := main.cpp \
                   imgui/imgui.cpp \
                   imgui/imgui_draw.cpp \
                   imgui/imgui_tables.cpp \
                   imgui/imgui_widgets.cpp \
                   imgui/backends/imgui_impl_android.cpp \
                   imgui/backends/imgui_impl_opengl3.cpp

# Add main.cpp later, once we create it in the next step. For now, it's a placeholder.
# If main.cpp doesn't exist yet, the build would fail, but this subtask is about setting up the makefile.
# We will create main.cpp in the next plan step.

LOCAL_C_INCLUDES += $(LOCAL_PATH)/imgui

# Ensure these linker flags are present (they were added in the previous step but good to confirm)
LOCAL_LDLIBS    := -llog -lEGL -lGLESv2 -landroid

# For ImGui, if using NDK GLAD or similar, you might need specific preprocessor definitions
# or to link against GLESv3 if imgui_impl_opengl3.cpp is used as is.
# For simplicity with AIDE, sticking to GLESv2 might be easier if a GLES2 backend is available.
# If using imgui_impl_opengl3.cpp directly, it might require some tweaks for GLES on Android.
# For now, we assume it's compatible enough or will be adapted.

include $(BUILD_SHARED_LIBRARY)
