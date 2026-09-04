#ifndef G_APPLICATION_HANDLER_H
#define G_APPLICATION_HANDLER_H

#include "window_handler.h"

class GApplicationHandler {
  private:
    ShaderWindowHandler *windowHandler = NULL;

  public:
    GApplicationHandler(ShaderWindowHandler *windowHandler);
    void runApp();
    ~GApplicationHandler();
};

#endif
