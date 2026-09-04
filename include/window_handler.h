#ifndef CONFIG_H
#define CONFIG_H

// clang-format off
#include "shader_program.h"

#include <epoxy/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
// clang-format on

class ShaderWindowHandler {
  private:
    // Wayland globals
    wl_registry *registry = NULL;
    wl_compositor *compositor = NULL;
    xdg_wm_base *wmBase = NULL;
    zxdg_decoration_manager_v1 *decorationManager = NULL;

    // Surface stack
    wl_surface *surface = NULL;
    xdg_surface *xdgSurface = NULL;
    xdg_toplevel *xdgToplevel = NULL;
    zxdg_toplevel_decoration_v1 *toplevelDecoration = NULL;
    wl_egl_window *eglWindow = NULL;

    // EGL
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;

    int pendingWidth = 0, pendingHeight = 0;

    static void registryGlobal(void *data, wl_registry *reg, uint32_t name,
                               const char *interface, uint32_t version);
    static void registryGlobalRemove(void *data, wl_registry *reg,
                                     uint32_t name);
    static void wmBasePing(void *data, xdg_wm_base *base, uint32_t serial);
    static void xdgSurfaceConfigure(void *data, xdg_surface *surf,
                                    uint32_t serial);
    static void toplevelConfigure(void *data, xdg_toplevel *top, int32_t width,
                                  int32_t height, wl_array *states);
    static void toplevelClose(void *data, xdg_toplevel *top);

    void initEGL();
    void applyResize(int width, int height);

  public:
    ShaderProgram shaderProgram;

    wl_display *display = NULL;
    bool running = true;

    ShaderWindowHandler(ShaderProps *shaderProps,
                        PipeWireHandler *pipeWireHandler,
                        AudioShaderStages *audioShaderStages);
    ~ShaderWindowHandler();

    // Connect, create the toplevel + GLES context, draw the first frame.
    void setup();

    // Called once per loop iteration by GApplicationHandler.
    void applyPendingResize();
    void swap();
};

#endif
