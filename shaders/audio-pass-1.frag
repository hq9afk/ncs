#version 320 es
precision highp float;
precision highp int;
precision highp sampler2D;

uniform sampler2D audioR;

out vec4 fragment;

/* 1D texture mapping (Nx1 2D texture under ES) */
void main() {

    fragment.r = texelFetch(audioR, ivec2(int(gl_FragCoord.x), 0), 0).r;
}
