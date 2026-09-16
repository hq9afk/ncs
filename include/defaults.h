#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "config_types.h"

namespace Defaults {

constexpr const char *audioName = "audio1";

constexpr unsigned int sampleRate = 11000;
constexpr int channels = 2;
constexpr int sampleSize = 1024;
constexpr int fragmentSize = 4096;
constexpr bool captureMic = false;
constexpr const char *audioFormat = "F32_LE";
constexpr bool applyFFT = true;
constexpr char *targetObject = nullptr;
constexpr float fftScale = 10.2f;
constexpr float fftCutOff = 0.3f;

constexpr int gravityAverageFrames = 5;
constexpr float gravityStep = 4.2f;

constexpr float sampleHybridWeight = .065f;
constexpr int sampleMode = 0;
constexpr int adjacentSampleNums = 1;
constexpr float sampleRange = .9f;
constexpr float sampleScale = 8.0f;
constexpr float smoothFactor = .025f;

constexpr const char *shaderName = "ncs";
constexpr const char *className = "ncs";
constexpr const char *configFileName = "ncs.glsl";
constexpr unsigned int fps = 60;

// Fixed square canvas the shader pipeline always renders at. The sphere geometry
// is defined in pixels relative to this (see shaders/ncs.glsl:
// sphere.radius = .7236 * resolution.x), so it must stay constant regardless of
// the Wayland surface size, and square, or sphereCoords() yields an ellipsoid.
// 1000 matches the gtk-layer-shell branch.
constexpr unsigned int sphereCanvas = 1000;

// Initial xdg_toplevel surface size. The compositor may hand out anything; the
// canvas above is blitted centered into whatever we get.
constexpr unsigned int windowWidth = 1000;
constexpr unsigned int windowHeight = 1000;

} // namespace Defaults

#endif
