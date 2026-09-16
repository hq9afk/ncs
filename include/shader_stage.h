#ifndef SHADER_STAGE_H
#define SHADER_STAGE_H

#include "shader_files.h"
#include "shaders.h"

class ShaderStage {
public:
    unsigned int glProgram = 0;
    FragmentShader* fragmentShader = NULL;
    VertexShader* vertexShader = NULL;

    ShaderFiles *fragmentShaderFile = NULL, *vertexShaderFile = NULL;

    // True when this stage shipped its own .vert file (currently only
    // ncs-1.vert) instead of falling back to the default full-screen-quad
    // vertex shader. ShaderProgram::render() uses this to switch to the
    // point-sprite + additive-blend draw path (the ES 2.0 replacement for the
    // ES 3.2 atomic-image particle accumulation — see ncs-1.vert).
    bool isParticleStage = false;

    /// A Map to hold locations for uniforms in the current shader stage.
    std::map<std::string, int> uniformLocations = { { "audioR", -1 },
        { "audioL", -1 } };

    ShaderStage* next = NULL;

    ~ShaderStage()
    {
        if (fragmentShader != NULL)
            delete fragmentShader;
        if (vertexShader != NULL)
            delete vertexShader;
        next = NULL;
    }
};

#endif
