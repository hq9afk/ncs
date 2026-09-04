#include "defaults.h"
#include "g_application_handler.h"
#include "window_handler.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

static ShaderWindowHandler *buildWindow() {
    PipeWireHandler *pipewire = new PipeWireHandler;
    pipewire->sampleRate = Defaults::sampleRate;
    pipewire->channels = Defaults::channels;
    pipewire->sampleSize = Defaults::sampleSize;
    pipewire->fragmentSize = Defaults::fragmentSize;
    pipewire->captureMic = Defaults::captureMic;
    pipewire->audioFormat = (char *)Defaults::audioFormat;
    pipewire->applyFFT = Defaults::applyFFT;
    pipewire->targetObject = Defaults::targetObject;
    pipewire->fftScale = Defaults::fftScale;
    pipewire->fftCutOff = Defaults::fftCutOff;
    pipewire->audioName = (char *)Defaults::audioName;

    GravitySettings *gravitySettings = new GravitySettings;
    gravitySettings->averageFrames = Defaults::gravityAverageFrames;
    gravitySettings->gravityStep = Defaults::gravityStep;

    SmoothSettings *smoothSettings = new SmoothSettings;
    smoothSettings->sampleHybridWeight = Defaults::sampleHybridWeight;
    smoothSettings->sampleMode = Defaults::sampleMode;
    smoothSettings->adjacentSampleNums = Defaults::adjacentSampleNums;
    smoothSettings->sampleRange = Defaults::sampleRange;
    smoothSettings->sampleScale = Defaults::sampleScale;
    smoothSettings->smoothFactor = Defaults::smoothFactor;

    AudioOverride *audioOverride = new AudioOverride;
    audioOverride->smoothSettings = smoothSettings;
    audioOverride->gravitySettings = gravitySettings;

    ShaderProps *shaderProps = new ShaderProps;
    shaderProps->shaderName = (char *)Defaults::shaderName;
    shaderProps->className = (char *)Defaults::className;
    shaderProps->configFileName = (char *)Defaults::configFileName;
    shaderProps->atomicTextures = Defaults::atomicTextures;
    shaderProps->fps = Defaults::fps;
    shaderProps->windowWidth = Defaults::sphereCanvas;
    shaderProps->windowHeight = Defaults::sphereCanvas;
    shaderProps->surfaceWidth = Defaults::windowWidth;
    shaderProps->surfaceHeight = Defaults::windowHeight;
    shaderProps->audioOverrides = audioOverride;

    AudioShaderStages *audioShaderStages = new AudioShaderStages(
        gravitySettings->averageFrames, pipewire->applyFFT);

    return new ShaderWindowHandler(shaderProps, pipewire, audioShaderStages);
}

int main(int argc, char *args[]) {
    if (argc > 1 && (!strcmp(args[1], "-V") || !strcmp(args[1], "--version"))) {
        std::cout << "WayVes Version 1.2.0\n";
        return 0;
    }

    GApplicationHandler gApplicationHandler(buildWindow());
    gApplicationHandler.runApp();
}
