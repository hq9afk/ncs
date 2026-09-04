#include "window_handler.h"
#include "errors.h"

#include <cstring>

ShaderWindowHandler::ShaderWindowHandler(ShaderProps *shaderProps,
                                         PipeWireHandler *pipeWireHandler,
                                         AudioShaderStages *audioShaderStages) {

    shaderProgram.shaderProps = shaderProps;

    if (shaderProps->audioOverrides == NULL)
        return;

    shaderProgram.pipeWireSetting = pipeWireHandler;

    shaderProgram.files =
        new Files(std::string(shaderProgram.shaderProps->shaderName),
                  std::string(shaderProps->configFileName));
    shaderProgram.files->LoadFiles(shaderProps->audioOverrides);

    shaderProgram.audioShaderStages = audioShaderStages;

    shaderProgram.fps = shaderProps->fps;
}

void ShaderWindowHandler::registryGlobal(void *data, wl_registry *reg,
                                         uint32_t name, const char *interface,
                                         uint32_t version) {
    ShaderWindowHandler *self = (ShaderWindowHandler *)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
        self->compositor = (wl_compositor *)wl_registry_bind(
            reg, name, &wl_compositor_interface, 4);
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
        self->wmBase = (xdg_wm_base *)wl_registry_bind(reg, name,
                                                       &xdg_wm_base_interface, 1);
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
        self->decorationManager = (zxdg_decoration_manager_v1 *)wl_registry_bind(
            reg, name, &zxdg_decoration_manager_v1_interface, 1);
}

void ShaderWindowHandler::registryGlobalRemove(void *data, wl_registry *reg,
                                               uint32_t name) {}

void ShaderWindowHandler::wmBasePing(void *data, xdg_wm_base *base,
                                     uint32_t serial) {
    xdg_wm_base_pong(base, serial);
}

void ShaderWindowHandler::xdgSurfaceConfigure(void *data, xdg_surface *surf,
                                              uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
}

void ShaderWindowHandler::toplevelConfigure(void *data, xdg_toplevel *top,
                                            int32_t width, int32_t height,
                                            wl_array *states) {
    ShaderWindowHandler *self = (ShaderWindowHandler *)data;

    // Zero means "you decide"; keep the configured default size.
    if (width > 0 && height > 0) {
        self->pendingWidth = width;
        self->pendingHeight = height;
    }
}

void ShaderWindowHandler::toplevelClose(void *data, xdg_toplevel *top) {
    ((ShaderWindowHandler *)data)->running = false;
}

void ShaderWindowHandler::initEGL() {
    ShaderProps *props = shaderProgram.shaderProps;

    eglDisplay = eglGetDisplay((EGLNativeDisplayType)display);
    if (eglDisplay == EGL_NO_DISPLAY)
        Errors::throwError("eglGetDisplay failed", "", "In");

    if (!eglInitialize(eglDisplay, NULL, NULL))
        Errors::throwError("eglInitialize failed", "", "In");

    eglBindAPI(EGL_OPENGL_ES_API);

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE,
    };
    EGLConfig config;
    EGLint numConfig = 0;
    if (!eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfig) ||
        numConfig == 0)
        Errors::throwError("no matching EGL config", "", "In");

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2, EGL_NONE,
    };
    eglContext =
        eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (eglContext == EGL_NO_CONTEXT)
        Errors::throwError("eglCreateContext failed", "", "In");

    eglWindow = wl_egl_window_create(surface, props->surfaceWidth,
                                    props->surfaceHeight);
    eglSurface = eglCreateWindowSurface(eglDisplay, config,
                                        (EGLNativeWindowType)eglWindow, NULL);
    if (eglSurface == EGL_NO_SURFACE)
        Errors::throwError("eglCreateWindowSurface failed", "", "In");

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    eglSwapInterval(eglDisplay, 1);
}

void ShaderWindowHandler::setup() {
    ShaderProps *props = shaderProgram.shaderProps;

    display = wl_display_connect(NULL);
    if (display == NULL)
        Errors::throwError("could not connect to a Wayland display", "", "In");

    static const wl_registry_listener registryListener = {
        .global = registryGlobal,
        .global_remove = registryGlobalRemove,
    };
    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, this);
    wl_display_roundtrip(display);

    if (compositor == NULL || wmBase == NULL)
        Errors::throwError(
            "compositor does not expose wl_compositor / xdg_wm_base", "", "In");

    static const xdg_wm_base_listener wmBaseListener = {.ping = wmBasePing};
    xdg_wm_base_add_listener(wmBase, &wmBaseListener, this);

    surface = wl_compositor_create_surface(compositor);

    xdgSurface = xdg_wm_base_get_xdg_surface(wmBase, surface);
    static const xdg_surface_listener xdgSurfaceListener = {
        .configure = xdgSurfaceConfigure};
    xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, this);

    xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
    static const xdg_toplevel_listener xdgToplevelListener = {
        .configure = toplevelConfigure,
        .close = toplevelClose,
    };
    xdg_toplevel_add_listener(xdgToplevel, &xdgToplevelListener, this);

    xdg_toplevel_set_title(xdgToplevel, props->className);
    xdg_toplevel_set_app_id(xdgToplevel, props->className);

    if (decorationManager != NULL) {
        toplevelDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
            decorationManager, xdgToplevel);
        zxdg_toplevel_decoration_v1_set_mode(
            toplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    }

    // Empty input region -> the surface is click-through.
    wl_region *empty = wl_compositor_create_region(compositor);
    wl_surface_set_input_region(surface, empty);
    wl_region_destroy(empty);

    wl_surface_commit(surface);
    wl_display_roundtrip(display); // wait for the first xdg_surface.configure

    initEGL();

    shaderProgram.render();
    swap();
}

void ShaderWindowHandler::applyResize(int width, int height) {
    wl_egl_window_resize(eglWindow, width, height, 0, 0);

    // Only the surface changed; the render canvas is fixed, so the shader
    // pipeline stays valid. render() re-centers the blit off these values.
    shaderProgram.shaderProps->surfaceWidth = width;
    shaderProgram.shaderProps->surfaceHeight = height;
}

void ShaderWindowHandler::applyPendingResize() {
    if (pendingWidth > 0 && pendingHeight > 0 &&
        (pendingWidth != shaderProgram.shaderProps->surfaceWidth ||
         pendingHeight != shaderProgram.shaderProps->surfaceHeight))
        applyResize(pendingWidth, pendingHeight);

    pendingWidth = pendingHeight = 0;
}

void ShaderWindowHandler::swap() { eglSwapBuffers(eglDisplay, eglSurface); }

ShaderWindowHandler::~ShaderWindowHandler() {
    if (eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        if (eglSurface != EGL_NO_SURFACE)
            eglDestroySurface(eglDisplay, eglSurface);
        if (eglContext != EGL_NO_CONTEXT)
            eglDestroyContext(eglDisplay, eglContext);
        eglTerminate(eglDisplay);
    }
    if (eglWindow)
        wl_egl_window_destroy(eglWindow);
    if (xdgToplevel)
        xdg_toplevel_destroy(xdgToplevel);
    if (xdgSurface)
        xdg_surface_destroy(xdgSurface);
    if (surface)
        wl_surface_destroy(surface);
    if (display)
        wl_display_disconnect(display);
}
