#include "files.h"

void Files::setupShaderFiles()
{
    vertexShaderFiles.loadShaders(shaderName, configFileName, ShaderTypes::VERTEX, { });
    fragmentShaderFiles.loadShaders(shaderName, configFileName, ShaderTypes::FRAGMENT, { });

    // Single post-processing effect, single pass: glow.
    std::map<std::string, int> vars = { { "postProcessingNumber", 1 } };
    postProcessingVertexShaderFiles.loadShaders("glow", configFileName, ShaderTypes::VERTEX, vars);
    postProcessingFragmentShaderFiles.loadShaders("glow", configFileName, ShaderTypes::FRAGMENT, vars);
}

void Files::setupAudioShaderFiles(AudioOverride* audioOverrides)
{
    std::map<std::string, int> vars = { };

    vars["adjacentSampleNums"] = audioOverrides->smoothSettings->adjacentSampleNums;
    smoothShaderFile.loadShaders("audio-smooth", "", ShaderTypes::FRAGMENT, vars);

    vars.clear();

    gravityShaderFile.loadShaders("audio-gravity", "", ShaderTypes::FRAGMENT, { });

    vars["avgFrames"] = audioOverrides->gravitySettings->averageFrames;
    averageShaderFile.loadShaders("audio-average", "", ShaderTypes::FRAGMENT, vars);

    passShaderFile.loadShaders("audio-pass", "", ShaderTypes::FRAGMENT, { });
}

void Files::LoadFiles(AudioOverride* audioOverrides)
{
    setupShaderFiles();
    setupAudioShaderFiles(audioOverrides);
}

Files::Files(std::string shaderName, std::string configFileName)
{
    this->shaderName = shaderName;
    this->configFileName = configFileName;
}
