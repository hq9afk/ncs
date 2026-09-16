#version 100
precision highp float;
precision highp int;

uniform sampler2D audioR;
// Nx1 audio texture width for this draw (viewport width); ES 2.0 has no
// texelFetch, so texture2D needs a normalized UV instead of an integer texel.
uniform int audioSize;

/* 1D texture mapping (Nx1 2D texture under ES) */
void main() {

    gl_FragColor.r = texture2D(audioR, vec2((gl_FragCoord.x) / float(audioSize), 0.5)).r;
}
