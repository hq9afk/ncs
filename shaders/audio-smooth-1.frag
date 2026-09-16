// Adapted from GLava's Audio Smoothing Shader by jarcode-foss
// Licensed under GPL-3.0

#version 100
precision highp float;
precision highp int;

#include ":audio-smooth-params.glsl"

uniform sampler2D audioR;

uniform int audioRSize;
uniform int adjacentSampleNums;

void main()
{
    float u = gl_FragCoord.x / float(audioRSize);
    gl_FragColor.r = 0.0;
    float aRI = 1. / float(audioRSize);
#define adjacent(I) gl_FragColor.r += (1. - step(float(I), 1.)) * (smooth_audio(audioR, audioRSize, u + float(I - 1) * aRI) + smooth_audio(audioR, audioRSize, u - float(I - 1) * aRI));

#expand adjacent adjacentSampleNums

    gl_FragColor.r += (smooth_audio(audioR, audioRSize, u));

    gl_FragColor.r /= 2. * (float(adjacentSampleNums) - 1.) + 1.;
}
