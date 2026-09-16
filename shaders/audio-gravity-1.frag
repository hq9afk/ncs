// Adapted from GLava's Gravity Shader by jarcode-foss
// Licensed under GPL-3.0

#version 100
precision highp float;
precision highp int;

uniform sampler2D audioR;
uniform float diff;
// Nx1 audio texture width for this draw (viewport width); ES 2.0 has no
// texelFetch, so texture2D needs a normalized UV instead of an integer texel.
uniform int audioSize;

void main()
{

    gl_FragColor.r = texture2D(audioR, vec2((gl_FragCoord.x) / float(audioSize), 0.5)).r - diff;
}
