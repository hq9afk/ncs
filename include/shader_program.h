#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

// clang-format off
#include <epoxy/gl.h>

#include <map>
#include <string>

#include "config_types.h"
#include "shader_stage.h"
#include "audio_shader_stages.h"
#include "shader_files.h"
#include "files.h"
// clang-format on

// Static per-pixel "which canvas pixels are particles" grid for the ES 2.0
// point-sprite accumulation pass (shaders/ncs-1.vert), built once per canvas
// size instead of every frame. Replicates, in C++, the exact per-pixel hash
// dropout shaders/ncs.glsl's fragment shader used to `discard` on
// (`particleThin`), for the rectangular one-particle-per-pixel grid that
// config actually produces -- not a generic re-implementation of arbitrary
// baseForm/z-layer configs (those would need this logic duplicated per
// config, which nothing on the C++ side currently supports).
class ParticleGrid {
  public:
    unsigned int vbo = 0;
    int pointCount = 0;

    void build(int canvasWidth, int canvasHeight);
    void destroy();

    ~ParticleGrid();
};

class ShaderProgram {
  private:
    void initializeShaders();

  public:
    bool audioLoaded = false;

    int ticks = 0, fps = 60;
    unsigned int prevStageTexture;

    ParticleGrid particleGrid;

    // Manual textured-quad composite of the final canvas onto the default
    // framebuffer, replacing glBlitFramebuffer (ES 3.0+ only; ES 2.0 has a
    // single GL_FRAMEBUFFER target and no blit call at all).
    VertexShader *blitVertexShader = NULL;
    FragmentShader *blitFragmentShader = NULL;
    unsigned int blitProgram = 0;
    std::map<std::string, int> blitUniformLocations;

    ShaderProps *shaderProps = NULL;

    Files *files = NULL;

    ShaderStage *startStage = NULL;

    PipeWireHandler *pipeWireSetting;

    AudioShaderStages *audioShaderStages;

    void applyAudioTransformations();
    void loadAudioShaders();

    void render();
};

#endif