#version 100

precision highp float;

// Point-sprite fragment shader for the particle-accumulation pass (paired
// with ncs-1.vert; see that file for why this exists on ES 2.0 instead of the
// atomic-image scatter the ES 3.2 version used). Evaluates the same
// distance-based splat falloff the old -size..size double loop did, but
// against gl_PointCoord instead of a manual pixel loop. The C++ draw call
// uses additive blending (glBlendFunc(GL_ONE, GL_ONE)) into a plain RGBA8
// target, which is what actually replaces imageAtomicAdd's per-pixel sum.

varying float vSplatSize;
varying float vSplatFeather;
varying float vSplatOpacity;

void main()
{
    // gl_PointCoord is 0..1 across the point sprite; map to a -1..1 offset,
    // then to the same "pixel distance from center" units the original
    // -size..size loop used (edge of the point == vSplatSize).
    vec2 offset = (gl_PointCoord - vec2(0.5)) * 2.0;
    float d = length(offset) * vSplatSize;

    float coverage = mix(step(d, vSplatSize),
        1.0 - smoothstep(vSplatSize - vSplatFeather * vSplatSize, vSplatSize, d),
        vSplatFeather);

    // The ES 3.2 version scaled this by 100000 for its raw uint accumulator;
    // here it accumulates directly in the 0..1 blend target, so no such scale.
    gl_FragColor = vec4(coverage * vSplatOpacity, 0.0, 0.0, 1.0);
}
