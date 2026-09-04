#include "g_application_handler.h"

#include <algorithm>
#include <cstdlib>
#include <poll.h>
#include <time.h>

static double monotonicSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

GApplicationHandler::GApplicationHandler(ShaderWindowHandler *windowHandler) {
    this->windowHandler = windowHandler;
}

void GApplicationHandler::runApp() {
    windowHandler->shaderProgram.pipeWireSetting->createPipeWireThread();

    windowHandler->setup();

    wl_display *display = windowHandler->display;
    int fd = wl_display_get_fd(display);

    double fps = (double)windowHandler->shaderProgram.fps;
    double minFrameTime = fps > 0 ? 1.0 / fps : 0.0;
    double sleepTime = 0.0;
    double last = monotonicSeconds();

    // ponytail: poll-timeout render loop; move to wl_surface_frame callbacks if
    // a compositor complains about commits without a frame callback.
    while (windowHandler->running) {
        while (wl_display_prepare_read(display) != 0)
            wl_display_dispatch_pending(display);
        wl_display_flush(display);

        struct pollfd pfd = {fd, POLLIN, 0};
        int timeoutMs = minFrameTime > 0 ? (int)(minFrameTime * 1000.0) : -1;

        if (poll(&pfd, 1, timeoutMs) > 0 && (pfd.revents & POLLIN))
            wl_display_read_events(display);
        else
            wl_display_cancel_read(display);
        wl_display_dispatch_pending(display);

        if (!windowHandler->running)
            break;

        windowHandler->applyPendingResize();

        windowHandler->shaderProgram.render();
        windowHandler->swap();

        double now = monotonicSeconds();
        double deltaTime = now - last;
        last = now;

        if (minFrameTime > 0) {
            sleepTime = std::max(0.0, sleepTime + minFrameTime - deltaTime);
            if (sleepTime > 0) {
                struct timespec request = {0, (long)(sleepTime * 1e9)};
                nanosleep(&request, NULL);
            }
        }
    }
}

GApplicationHandler::~GApplicationHandler() { std::exit(0); }
