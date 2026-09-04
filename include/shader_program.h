#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

// clang-format off
#include <epoxy/gl.h>

#include "config_types.h"
#include "shader_stage.h"
#include "audio_shader_stages.h"
#include "shader_files.h"
#include "files.h"
// clang-format on

class ShaderProgram {
  private:
    void initializeShaders();

  public:
    bool audioLoaded = false;

    int ticks = 0, fps = 60;
    unsigned int prevStageTexture;

    GLuint *atomicImageTexture = NULL;

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