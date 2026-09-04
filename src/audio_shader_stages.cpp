#include "shader_stage.h"

#include "audio_shader_stages.h"

#include <cstdint>
#include <vector>

// Desktop stored these as normalized GL_R16 (FFT path, clamps to [0, 1]) and
// GL_R16_SNORM (raw path, clamps to [-1, 1]) and let the driver convert from
// float on upload. GLES exposes both via GL_EXT_texture_norm16 but only accepts
// integer pixel data, so convert here. Keeping the exact formats preserves the
// per-write clamping the rest of the pipeline was tuned against.
static void audioTexImage2D(GLsizei width, bool applyFFT, const float* data)
{
    if (data == NULL) {
        glTexImage2D(GL_TEXTURE_2D, 0, applyFFT ? GL_R16 : GL_R16_SNORM, width, 1, 0,
            GL_RED, applyFFT ? GL_UNSIGNED_SHORT : GL_SHORT, NULL);
        return;
    }

    if (applyFFT) {
        std::vector<uint16_t> buf((size_t)width);
        for (GLsizei i = 0; i < width; i++) {
            float v = data[i] < 0.0f ? 0.0f : (data[i] > 1.0f ? 1.0f : data[i]);
            buf[i] = (uint16_t)(v * 65535.0f + 0.5f);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, width, 1, 0, GL_RED, GL_UNSIGNED_SHORT, buf.data());
    } else {
        std::vector<int16_t> buf((size_t)width);
        for (GLsizei i = 0; i < width; i++) {
            float v = data[i] < -1.0f ? -1.0f : (data[i] > 1.0f ? 1.0f : data[i]);
            buf[i] = (int16_t)(v * 32767.0f + (v < 0.0f ? -0.5f : 0.5f));
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16_SNORM, width, 1, 0, GL_RED, GL_SHORT, buf.data());
    }
}

void AudioShaderStage::compile(ShaderFiles* fragmentShaderFiles)
{
    VertexShaderCompilationArgs* vertexArgs = new VertexShaderCompilationArgs(0, 0, &glProgram, &uniformLocations);
    vertexShader = new VertexShader("", vertexArgs);

    FragmentShaderCompilationArgs* args = new FragmentShaderCompilationArgs(0, 0, &glProgram, &uniformLocations);

    fragmentShader = new FragmentShader(fragmentShaderFiles->fileContent, args);

    fragmentShaderFile = fragmentShaderFiles;
}

AudioShaderStage::AudioShaderStage(bool applyFFT)
{
    this->applyFFT = applyFFT;
}

void AudioShaderStage::bindAudio(int offset, int size, char* errorContext)
{
    unsigned int* outputTexture = 0;
    outputTexture = offset == 1 ? &fragmentShader->outputTexture : &outputLTexture;

    if (outputTexture == NULL || *outputTexture == 0) {
        glGenTextures(1, outputTexture);

        if (fragmentShader->frameBufferObject == 0)
            glGenFramebuffers(1, &fragmentShader->frameBufferObject);

        glBindTexture(GL_TEXTURE_2D, *outputTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // ES has no 1D textures; emulate with an Nx1 2D texture (same R16/SNORM format).
        audioTexImage2D(size, applyFFT, NULL);

        glBindFramebuffer(GL_FRAMEBUFFER, fragmentShader->frameBufferObject);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, *outputTexture, 0);

        switch (glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            Errors::throwError("an error occured while binding audio framebuffer", errorContext, "In");
        }
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, fragmentShader->frameBufferObject);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, *outputTexture, 0);
    }
}

AudioShaderStages::AudioShaderStages(int numGravityFBOs, bool applyFFT)
{
    this->numGravityFBOs = numGravityFBOs;
    this->applyFFT = applyFFT;
    this->gravityFBOStages = new AudioShaderStage*[numGravityFBOs];
    smoothStage = new AudioShaderStage(applyFFT);
    averageStage = new AudioShaderStage(applyFFT);
    passStage = new AudioShaderStage(applyFFT);
    gravityStage = new AudioShaderStage(applyFFT);

    for (int i = 0; i < numGravityFBOs; i++) {
        gravityFBOStages[i] = new AudioShaderStage(applyFFT);
        gravityFBOStages[i]->fragmentShader = new FragmentShader();
    }
}

GLuint AudioShaderStages::create1DTexture()
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return tex;
}

void AudioShaderStages::createAudioTextures()
{
    audioLTexture = create1DTexture();
    audioRTexture = create1DTexture();
}

// Portions adapted from GLava's Gravity and Averaging Shaders by jarcode-foss
// Licensed under GPL-3.0

void AudioShaderStages::applyGravityPassShader(unsigned int audioTextureSize, int offset,
    std::string locationName, char* errorContext)
{

    if (applyFFT) {
        passStage->bindAudio(offset, audioTextureSize, errorContext);

        glUseProgram(passStage->glProgram);

        glActiveTexture(GL_TEXTURE0 + offset);
        glBindTexture(GL_TEXTURE_2D, offset == 1 ? audioRTexture : audioLTexture);
        glUniform1i(passStage->uniformLocations[locationName],
            offset);

        glEnable(GL_BLEND);
        glBlendEquation(GL_MAX);
        glViewport(0, 0, audioTextureSize, 1);
        passStage
            ->vertexShader->draw();

        glBlendEquation(GL_FUNC_ADD);
        glDisable(GL_BLEND);

        // ES has no texture barrier; the following stage samples the texture we
        // just rendered, so a full flush is the portable RAW-hazard guard.
        // ponytail: glFinish on an Nx1 draw is cheap; revisit only if the audio
        // path shows up in a profile.
        glFinish();

        glUseProgram(gravityStage->glProgram);

        glActiveTexture(GL_TEXTURE0 + offset);
        glBindTexture(GL_TEXTURE_2D,
            offset == 1 ? passStage->fragmentShader
                              ->outputTexture
                        : passStage->outputLTexture);
        glUniform1i(gravityStage
                        ->uniformLocations[locationName],
            offset);

        glViewport(0, 0, audioTextureSize, 1);
        gravityStage->vertexShader->draw();
    } else {
        passStage->fragmentShader->outputTexture = audioRTexture;
        passStage->outputLTexture = audioLTexture;
    }

    gravityFBOStages[offset == 1
            ? out_idx
            : out_idx_l]
        ->bindAudio(offset, audioTextureSize, errorContext);

    glUseProgram(passStage->glProgram);

    glActiveTexture(GL_TEXTURE0 + offset);
    glBindTexture(GL_TEXTURE_2D,
        offset == 1 ? passStage->fragmentShader
                          ->outputTexture
                    : passStage
                          ->outputLTexture);
    glUniform1i(passStage->uniformLocations[locationName],
        offset);

    glViewport(0, 0, audioTextureSize, 1);

    passStage->vertexShader->draw();
    averageStage->bindAudio(offset, audioTextureSize, errorContext);

    glUseProgram(averageStage->glProgram);

    for (int t = 0; t < numGravityFBOs; ++t) {
        GLuint c_off = offset + 1 + t;

        std::string a = "audioR";
        a += std::to_string(t);

        int fr = (offset == 1 ? out_idx
                              : out_idx_l)
            - t;
        if (fr < 0)
            fr = numGravityFBOs + fr;

        glActiveTexture(GL_TEXTURE0 + c_off);
        glBindTexture(
            GL_TEXTURE_2D,
            offset == 1 ? gravityFBOStages[fr]->fragmentShader->outputTexture
                        : gravityFBOStages[fr]->outputLTexture);
        GLuint audioLoc = averageStage->uniformLocations[(char*)&a[0]];
        glUniform1i(audioLoc, c_off);
    }

    glViewport(0, 0, audioTextureSize, 1);

    averageStage->vertexShader->draw();
    offset == 1 ? ++out_idx : ++out_idx_l;
    if (out_idx >= numGravityFBOs)
        out_idx = 0;
    if (out_idx_l >= numGravityFBOs)
        out_idx_l = 0;

    glEnable(GL_BLEND);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Portions adapted from GLava's Audio Smoothing Shader by jarcode-foss
// Licensed under GPL-3.0

void AudioShaderStages::applySmoothPassShader(unsigned int audioTextureSize, unsigned int adjacentSampleNums, int offset,
    std::string locationName, char* errorContext)
{

    smoothStage->bindAudio(offset, audioTextureSize, errorContext);

    glUseProgram(smoothStage->glProgram);
    int uniformLoc = smoothStage->uniformLocations["audioRSize"];
    glUniform1i(uniformLoc, audioTextureSize);

    uniformLoc = smoothStage->uniformLocations["adjacentSampleNums"];
    glUniform1i(uniformLoc, adjacentSampleNums);

    glActiveTexture(GL_TEXTURE0 + offset);
    glBindTexture(GL_TEXTURE_2D,
        offset == 1
            ? averageStage
                  ->fragmentShader->outputTexture
            : averageStage
                  ->outputLTexture);
    glUniform1i(smoothStage
                    ->uniformLocations[locationName],
        offset);

    glDisable(GL_BLEND);
    glViewport(0, 0, audioTextureSize, 1);

    smoothStage->vertexShader->draw();
    glEnable(GL_BLEND);

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AudioShaderStages::updateAudioTextures(int lOffset, size_t lSize, float* lData, int rOffset, size_t rSize, float* rData)
{

    glBindTexture(GL_TEXTURE_2D, audioLTexture);
    audioTexImage2D((GLsizei)lSize, applyFFT, lData + lOffset);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, audioRTexture);
    audioTexImage2D((GLsizei)rSize, applyFFT, rData + rOffset);
    glBindTexture(GL_TEXTURE_2D, 0);
}
