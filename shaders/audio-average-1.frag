// Adapted from GLava's Averaging Shader by jarcode-foss
// Licensed under GPL-3.0

#version 100
precision highp float;
precision highp int;

#include ":audio-utils.glsl"

uniform int avgFrames;
// Nx1 audio texture width for this draw (viewport width); ES 2.0 has no
// texelFetch, so texture2D needs a normalized UV instead of an integer texel.
uniform int audioSize;

#expand uniform sampler2D audioR#; avgFrames

void main()
{
    float r = 0.0;

#expand r += window(float(#), float(avgFrames - 1)) * texture2D(audioR#, vec2((gl_FragCoord.x) / float(audioSize), 0.5)).r; avgFrames

    gl_FragColor.r = (r / float(avgFrames));
}
