#include "shader_files.h"
#include "config_types.h"
#include <string>

class Files
{
private:
    std::string shaderName, configFileName;

    void setupShaderFiles();
    void setupAudioShaderFiles(AudioOverride *audioOverrides);

public:
    void LoadFiles(AudioOverride *audioOverrides);
    Files(std::string shaderName, std::string configFileName);

    ShaderFiles vertexShaderFiles, fragmentShaderFiles, smoothShaderFile, postProcessingVertexShaderFiles, postProcessingFragmentShaderFiles,
        gravityShaderFile, averageShaderFile, passShaderFile;
};
