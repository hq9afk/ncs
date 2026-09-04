// Adapted from GLava's Averaging Shader by jarcode-foss
// Licensed under GPL-3.0

#version 320 es
precision highp float;
precision highp int;
precision highp sampler2D;

#include ":audio-utils.glsl"

uniform int avgFrames;

#expand uniform sampler2D audioR#; avgFrames

out vec4 FragColor;

void main()
{
    float r = 0.0;

#expand r += window(float(#), float(avgFrames - 1)) * texelFetch(audioR#, ivec2(int(gl_FragCoord.x), 0), 0).r; avgFrames

    FragColor.r = (r / float(avgFrames));
}
