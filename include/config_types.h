#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include "pipewire_handler.h"
#include <cstdint>
#include <iostream>

class GravitySettings {
  public:
    int averageFrames;
    float gravityStep;
};

class SmoothSettings {
  public:
    int sampleMode, adjacentSampleNums;
    float sampleHybridWeight, sampleScale, sampleRange, smoothFactor;
};

class AudioOverride {
  public:
    SmoothSettings *smoothSettings = NULL;
    GravitySettings *gravitySettings = NULL;
};

class ShaderProps {
  public:
    char *shaderName = NULL, *configFileName = NULL, *className = NULL;

    // windowWidth/windowHeight is the fixed square render canvas (fed to shaders
    // as `resolution`). surfaceWidth/surfaceHeight is the actual Wayland surface
    // the canvas is composited into, centered.
    uint16_t windowWidth, windowHeight;
    uint16_t surfaceWidth, surfaceHeight;

    unsigned int fps;

    AudioOverride *audioOverrides = NULL;
};

#endif
