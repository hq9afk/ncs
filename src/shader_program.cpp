#include <epoxy/gl.h>

#include "shader_program.h"

void ShaderProgram::initializeShaders() {
    startStage = new ShaderStage;

    ShaderStage *currentStage = startStage;

    ShaderFiles *fragmentShaderFilesIterator = &files->fragmentShaderFiles;
    ShaderFiles *vertexShaderFilesIterator = &files->vertexShaderFiles;
    ShaderFiles *postProcessingFragmentShaderFilesIterator =
        &files->postProcessingFragmentShaderFiles;
    ShaderFiles *postProcessingVertexShaderFilesIterator =
        &files->postProcessingVertexShaderFiles;

    if (shaderProps->atomicTextures > 0 && atomicImageTexture == NULL) {
        atomicImageTexture = new unsigned int[shaderProps->atomicTextures];
        for (int i = 0; i < shaderProps->atomicTextures; i++)
            atomicImageTexture[i] = 0;
    }

    while (fragmentShaderFilesIterator != NULL) {
        VertexShaderCompilationArgs *vertexArgs =
            new VertexShaderCompilationArgs(
                shaderProps->windowWidth, shaderProps->windowHeight,
                &currentStage->glProgram, &currentStage->uniformLocations);
        if (vertexShaderFilesIterator == NULL) {
            vertexShaderFilesIterator = new ShaderFiles;
            vertexShaderFilesIterator->fileContent = "";
        }

        currentStage->vertexShader = new VertexShader(
            vertexShaderFilesIterator->fileContent, vertexArgs);

        currentStage->vertexShaderFile = vertexShaderFilesIterator;
        currentStage->fragmentShaderFile = fragmentShaderFilesIterator;

        FragmentShaderCompilationArgs *args =
            currentStage != startStage
                ? new FragmentShaderCompilationArgs(
                      shaderProps->windowWidth, shaderProps->windowHeight,
                      &currentStage->glProgram, &currentStage->uniformLocations)
                : new FragmentShaderCompilationArgs(
                      shaderProps->windowWidth, shaderProps->windowHeight,
                      &currentStage->glProgram, &currentStage->uniformLocations,
                      shaderProps->atomicTextures, atomicImageTexture);

        currentStage->fragmentShader =
            new FragmentShader(fragmentShaderFilesIterator->fileContent, args);

        // Every stage renders to its own FBO; render() blits the final one
        // centered onto the (surface-sized) default framebuffer.
        currentStage->fragmentShader->bind2DTextureToFrameBuffer(
            shaderProps->className);

        fragmentShaderFilesIterator = fragmentShaderFilesIterator->next;

        if (vertexShaderFilesIterator != NULL)
            vertexShaderFilesIterator = vertexShaderFilesIterator->next;

        if (currentStage->next == NULL)
            currentStage->next = new ShaderStage;

        currentStage = currentStage->next;
    }

    if (!postProcessingFragmentShaderFilesIterator->fileContent.empty())

        while (postProcessingFragmentShaderFilesIterator != NULL) {
            VertexShaderCompilationArgs *vertexArgs =
                new VertexShaderCompilationArgs(
                    shaderProps->windowWidth, shaderProps->windowHeight,
                    &currentStage->glProgram, &currentStage->uniformLocations);

            if (postProcessingVertexShaderFilesIterator == NULL) {
                postProcessingVertexShaderFilesIterator = new ShaderFiles;
                postProcessingVertexShaderFilesIterator->fileContent = "";
            }

            currentStage->vertexShader = new VertexShader(
                postProcessingVertexShaderFilesIterator->fileContent,
                vertexArgs);

            currentStage->vertexShaderFile =
                postProcessingVertexShaderFilesIterator;
            currentStage->fragmentShaderFile =
                postProcessingFragmentShaderFilesIterator;

            FragmentShaderCompilationArgs *args =
                currentStage != startStage
                    ? new FragmentShaderCompilationArgs(
                          shaderProps->windowWidth, shaderProps->windowHeight,
                          &currentStage->glProgram,
                          &currentStage->uniformLocations)
                    : new FragmentShaderCompilationArgs(
                          shaderProps->windowWidth, shaderProps->windowHeight,
                          &currentStage->glProgram,
                          &currentStage->uniformLocations,
                          shaderProps->atomicTextures, atomicImageTexture);

            currentStage->fragmentShader = new FragmentShader(
                postProcessingFragmentShaderFilesIterator->fileContent, args);

            currentStage->fragmentShader->bind2DTextureToFrameBuffer(
                shaderProps->className);

            postProcessingFragmentShaderFilesIterator =
                postProcessingFragmentShaderFilesIterator->next;

            if (postProcessingVertexShaderFilesIterator != NULL)
                postProcessingVertexShaderFilesIterator =
                    postProcessingVertexShaderFilesIterator->next;

            if (currentStage->next == NULL)
                currentStage->next = new ShaderStage;

            currentStage = currentStage->next;
        }
}

#define TWOPI 6.28318530718
// #define PI 3.14159265359
#define window(t, sz)                                                          \
    (0.53836 - (0.46164 * cos(TWOPI * (double)t / (double)sz)))

// Portions adapted from GLava's original implementation of Radix-2 DIT FFT by
// jarcode-foss Licensed under GPL-3.0

void applyFFT(float *samples, int n_samples, float fftScale, float fftCutOff) {

    float *data = samples;
    unsigned long nn = (unsigned long)(n_samples) / 2;

    unsigned long n, mmax, m, j, istep, i;
    float wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;

    /* apply window */
    for (i = 0; i < (unsigned long)n_samples; ++i) {
        data[i] *= window(i, n_samples - 1);
    }

    /* reverse-binary reindexing */
    n = nn << 1;
    j = 1;
    for (i = 1; i < n; i += 2) {
        if (j > i) {
            std::swap(data[j - 1], data[i - 1]);
            std::swap(data[j], data[i]);
        }
        m = nn;
        while (m >= 2 && j > m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    };

    /* here begins the Danielson-Lanczos section */
    mmax = 2;
    while (n > mmax) {
        istep = mmax << 1;
        theta = -(2 * M_PI / mmax);
        wtemp = sin(0.5 * theta);
        wpr = -2.0 * wtemp * wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m = 1; m < mmax; m += 2) {
            for (i = m; i <= n; i += istep) {
                j = i + mmax;
                tempr = wr * data[j - 1] - wi * data[j];
                tempi = wr * data[j] + wi * data[j - 1];

                data[j - 1] = data[i - 1] - tempr;
                data[j] = data[i] - tempi;
                data[i - 1] += tempr;
                data[i] += tempi;
            }
            wtemp = wr;
            wr += wr * wpr - wi * wpi;
            wi += wi * wpr + wtemp * wpi;
        }
        mmax = istep;
    }

    /* abs and log scale */
    for (n = 0; n < (unsigned long)n_samples; n += 2) {

        if (data[n] < 0.0F)
            data[n] = -data[n];

        data[n] = sqrt(data[n] * data[n] + data[n + 1] * data[n + 1]);
        data[n] = log(data[n] + 1) / 3;
        data[n] *= std::max((((float)n / (float)n_samples) * fftScale) +
                                (1.0F - fftCutOff),
                            1.0F);

        data[n + 1] = data[n];
    }
}

void ShaderProgram::applyAudioTransformations() {

    pthread_mutex_lock(&pipeWireSetting->mutex);

    // Before the capture thread delivers its first buffer the sizes are 0. A
    // zero-width audio texture is an incomplete FBO attachment, which ES
    // (unlike desktop GL here) reports as an error and the app then exits. Skip
    // the transform until real data arrives; nothing is on screen yet anyway.
    if (pipeWireSetting->audioLSize == 0 || pipeWireSetting->audioRSize == 0) {
        pthread_mutex_unlock(&pipeWireSetting->mutex);
        return;
    }

    bool modified = pipeWireSetting->modified;
    if (modified)
        pipeWireSetting->modified = false;

    if (modified && pipeWireSetting->applyFFT) {

        applyFFT(pipeWireSetting->audioLData, pipeWireSetting->audioLSize,
                 pipeWireSetting->fftScale, pipeWireSetting->fftCutOff);
        applyFFT(pipeWireSetting->audioRData, pipeWireSetting->audioRSize,
                 pipeWireSetting->fftScale, pipeWireSetting->fftCutOff);
    }

    audioShaderStages->updateAudioTextures(
        0, pipeWireSetting->audioLSize, pipeWireSetting->audioLData, 0,
        pipeWireSetting->audioRSize, pipeWireSetting->audioRData);

    pthread_mutex_unlock(&pipeWireSetting->mutex);

    audioShaderStages->applyGravityPassShader(
        pipeWireSetting->audioRSize, 1, "audioR", shaderProps->shaderName);
    audioShaderStages->applyGravityPassShader(
        pipeWireSetting->audioLSize, 2, "audioR", shaderProps->shaderName);

    audioShaderStages->applySmoothPassShader(
        pipeWireSetting->audioRSize,
        shaderProps->audioOverrides->smoothSettings->adjacentSampleNums, 1,
        "audioR", shaderProps->shaderName);
    audioShaderStages->applySmoothPassShader(
        pipeWireSetting->audioLSize,
        shaderProps->audioOverrides->smoothSettings->adjacentSampleNums, 2,
        "audioR", shaderProps->shaderName);

    glViewport(0, 0, shaderProps->windowWidth, shaderProps->windowHeight);
}

void ShaderProgram::loadAudioShaders() {

    audioShaderStages->createAudioTextures();

    ShaderFiles *file = &files->passShaderFile;
    while (file != NULL) {
        audioShaderStages->passStage->compile(&files->passShaderFile);

        file = file->next;
    }

    file = &files->smoothShaderFile;
    while (file != NULL) {

        audioShaderStages->smoothStage->compile(&files->smoothShaderFile);

        glUseProgram(audioShaderStages->smoothStage->glProgram);

        int uniformLoc =
            audioShaderStages->smoothStage->uniformLocations["smooth_factor"];
        glUniform1f(uniformLoc,
                    shaderProps->audioOverrides->smoothSettings->smoothFactor);

        uniformLoc = audioShaderStages->smoothStage
                         ->uniformLocations["sample_hybrid_weight"];
        glUniform1f(
            uniformLoc,
            shaderProps->audioOverrides->smoothSettings->sampleHybridWeight);

        uniformLoc =
            audioShaderStages->smoothStage->uniformLocations["sample_scale"];
        glUniform1f(uniformLoc,
                    shaderProps->audioOverrides->smoothSettings->sampleScale);

        uniformLoc =
            audioShaderStages->smoothStage->uniformLocations["sample_range"];
        glUniform1f(uniformLoc,
                    shaderProps->audioOverrides->smoothSettings->sampleRange);

        uniformLoc =
            audioShaderStages->smoothStage->uniformLocations["sample_mode"];
        glUniform1i(uniformLoc,
                    shaderProps->audioOverrides->smoothSettings->sampleMode);

        glUseProgram(0);

        file = file->next;
    }

    file = &files->gravityShaderFile;
    while (file != NULL) {

        audioShaderStages->gravityStage->compile(&files->gravityShaderFile);

        glUseProgram(audioShaderStages->gravityStage->glProgram);

        int uniformLoc =
            audioShaderStages->gravityStage->uniformLocations["diff"];
        glUniform1f(uniformLoc, (float)((shaderProps->audioOverrides
                                             ->gravitySettings->gravityStep) /
                                        fps));
        glUseProgram(0);

        file = file->next;
    }

    file = &files->averageShaderFile;
    while (file != NULL) {

        audioShaderStages->averageStage->compile(&files->averageShaderFile);

        glUseProgram(audioShaderStages->averageStage->glProgram);
        int frameLoc =
            audioShaderStages->averageStage->uniformLocations["avgFrames"];
        glUniform1i(
            frameLoc,
            shaderProps->audioOverrides->gravitySettings->averageFrames);
        glUseProgram(0);
        file = file->next;
    }

    audioLoaded = true;
}

void ShaderProgram::render() {

    if (ticks == 0 && !audioLoaded)
        loadAudioShaders();

    int defaultID;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultID);

    applyAudioTransformations();

    if (ticks == 0)
        initializeShaders();

    ShaderStage *currentStage = startStage;
    ShaderStage *lastStage = startStage;

    while (currentStage->next != NULL) {

        glUseProgram(currentStage->glProgram);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                          currentStage->fragmentShader->frameBufferObject);
        glViewport(0, 0, shaderProps->windowWidth, shaderProps->windowHeight);

        currentStage->fragmentShader->updateUniforms(
            shaderProps->windowWidth, shaderProps->windowHeight, 0, 1, ticks,
            pipeWireSetting->audioLSize,
            audioShaderStages->smoothStage->outputLTexture,
            pipeWireSetting->audioRSize,
            audioShaderStages->smoothStage->fragmentShader->outputTexture,
            currentStage->uniformLocations);

        currentStage->vertexShader->draw(
            currentStage != startStage ? &prevStageTexture : NULL);

        prevStageTexture = currentStage->fragmentShader->outputTexture;

        lastStage = currentStage;
        currentStage = currentStage->next;
        glUseProgram(0);
    };

    // Composite the fixed square canvas centered onto the real surface. Bigger
    // surface -> transparent margin; smaller -> centered crop (glBlitFramebuffer
    // clips the destination), matching how the gtk-layer-shell window crops its
    // oversized sphere.
    const int canvas = shaderProps->windowWidth;
    const int offX = (shaderProps->surfaceWidth - canvas) / 2;
    const int offY = (shaderProps->surfaceHeight - canvas) / 2;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultID);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                      lastStage->fragmentShader->frameBufferObject);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultID);
    glBlitFramebuffer(0, 0, canvas, canvas, offX, offY, offX + canvas,
                      offY + canvas, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    ticks++;
}
