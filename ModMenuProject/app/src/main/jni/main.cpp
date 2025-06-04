#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h> // Using GLES2 for broader compatibility, ImGui backend might need adjustment

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
// Assuming GLES2 backend. If you use imgui_impl_opengl3.cpp, make sure it's configured for GLES
// or use an explicit GLES2 backend if available from ImGui.
// For this example, we'll assume imgui_impl_opengl3.h/cpp can work with GLES2 context
// or that you'd swap it for an imgui_impl_gles2.h/cpp if that's what you have.
#include "imgui/backends/imgui_impl_opengl3.h"


#define LOG_TAG "ModMenuApp"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

struct SavedState {
    // Add any state you want to save/restore here
};

struct Engine {
    struct android_app* app;
    ImGuiIO* io; // Store ImGuiIO pointer

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;

    struct SavedState state;
    bool active; // Is the app active and rendering?
};

// Initialize EGL display, surface, and context
static int engine_init_display(struct Engine* engine) {
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 16, // Depth buffer
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };
    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

    // ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format); // Deprecated in later NDKs

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);

    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, NULL, context_attribs);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGE("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    glViewport(0, 0, w, h);
    // Initialize GLES state here if needed
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background

    LOGI("EGL Initialized. Width: %d, Height: %d", w, h);
    return 0;
}

// Initialize ImGui
static void engine_init_imgui(struct Engine* engine) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    engine->io = &io; // Store IO pointer

    // Setup ImGui style (optional)
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplAndroid_Init(engine->app->window); // Pass ANativeWindow
    ImGui_ImplOpenGL3_Init("#version 100"); // Using GL ES 2.0 shader version (GLSL 100)

    LOGI("ImGui Initialized.");
}

// Render a frame
static void engine_draw_frame(struct Engine* engine) {
    if (engine->display == NULL || !engine->active) {
        return;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(); // Consider display size if it can change
    ImGui::NewFrame();

    // --- Your ImGui code starts here ---
    static float multiplier = 1.0f;
    static bool show_another_window = false;

    ImGui::Begin("Mod Menu"); // Create a window called "Mod Menu"

    ImGui::Text("Game Modifiers");
    ImGui::SliderFloat("Multiplier", &multiplier, 0.0f, 10000.0f, "%.1fx");

    ImGui::Separator();
    ImGui::Text("Placeholders for other features:");
    if (ImGui::Button("Toggle HP Mod")) { /* Do something */ }
    if (ImGui::Button("Toggle Damage Mod")) { /* Do something */ }
    // Add more placeholders as needed...
    // ImGui::Checkbox("Enable Crit Mod", &crit_mod_enabled);
    // ImGui::InputInt("Item ID", &item_id);

    ImGui::Separator();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
    // --- Your ImGui code ends here ---

    // Rendering
    ImGui::Render();
    glViewport(0, 0, engine->width, engine->height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetRenderData());

    eglSwapBuffers(engine->display, engine->surface);
}

// Terminate EGL display, surface, and context
static void engine_term_display(struct Engine* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->active = false;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
    LOGI("EGL Terminated.");
}

// Shutdown ImGui
static void engine_shutdown_imgui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    LOGI("ImGui Shutdown.");
}

// Process input event
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    // Forward input to ImGui
    return ImGui_ImplAndroid_HandleInputEvent(event);
}

// Process main command from Android system
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct Engine* engine = (struct Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state. Do so.
            engine->app->savedState = malloc(sizeof(struct SavedState));
            *((struct SavedState*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct SavedState);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine); // Initialize EGL
                if (!ImGui::GetCurrentContext()) { // Initialize ImGui only once
                    engine_init_imgui(engine);
                }
                engine->active = true;
                engine_draw_frame(engine); // Initial draw
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine->active = false;
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            engine->active = true;
            break;
        case APP_CMD_LOST_FOCUS:
            // Also stop animating if the app loses focus.
            engine->active = false;
            // engine_draw_frame(engine); // Optionally draw one last frame
            break;
        case APP_CMD_DESTROY:
            // Activity is being destroyed.
            engine_shutdown_imgui(); // Shutdown ImGui
            // engine_term_display(engine); // Ensure display is torn down if not already
            break;
        // You can handle other commands like APP_CMD_CONFIG_CHANGED if needed
    }
}

// Main entry point for a NativeActivity
void android_main(struct android_app* state) {
    struct Engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore it from here.
        engine.state = *(struct SavedState*)state->savedState;
    }

    // Event loop
    while (1) {
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, block until events are received.
        // If animating, loop until all events are processed, then draw the next frame.
        while ((ident = ALooper_pollAll(engine.active ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            // (Example for sensor data, not used in this ImGui setup)
            // if (ident == LOOPER_ID_USER) {
            //     if (engine.accelerometerSensor != NULL) {
            //         ASensorEvent event;
            //         while (ASensorEventQueue_getEvents(engine.sensorEventQueue, &event, 1) > 0) {
            //             LOGI("accelerometer: x=%f y=%f z=%f",
            //                  event.acceleration.x, event.acceleration.y,
            //                  event.acceleration.z);
            //         }
            //     }
            // }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                // engine_term_display(&engine); // Already handled by APP_CMD_DESTROY
                // engine_shutdown_imgui();    // Already handled by APP_CMD_DESTROY
                return;
            }
        }

        if (engine.active) {
            // Drawing is controlled by APP_CMD_INIT_WINDOW and the active state
            engine_draw_frame(&engine);
        }
    }
}
