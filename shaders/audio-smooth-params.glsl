// Portions adapted from GLava's Audio Smoothing Shader by jarcode-foss
// Licensed under GPL-3.0

#include ":audio-utils.glsl"

#define ROUND_FORMULA sinusoidal

uniform int sample_mode;
uniform float sample_hybrid_weight;
uniform float sample_scale;
uniform float sample_range;
uniform float smooth_factor;

float scale_audio(float idx)
{
    return -log((-(sample_range)*idx) + 1.0) / (sample_scale);
}

float smooth_audio(in sampler2D tex, int tex_sz, highp float idx)
{

    float smin = scale_audio(clamp(idx - smooth_factor, 0.0, 1.0)) * float(tex_sz),
          smax = scale_audio(clamp(idx + smooth_factor, 0.0, 1.0)) * float(tex_sz);
    float m = ((smax - smin) / 2.0F), s, w;
    float rm = smin + m;

    // ES 2.0 has no texelFetch; NEAREST-filtered texture2D at the texel's
    // center UV is the exact equivalent since tex_sz is the same width the
    // Nx1 audio texture was allocated/drawn at.
#define AUDIO_TEXEL(t, idx) texture2D(t, vec2((float(idx) + 0.5) / float(tex_sz), 0.5))

    if (sample_mode == 0) {
        float avg = 0.0, weight = 0.0;
        for (s = smin; s <= smax; s += 1.0F) {
            w = ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0.0, 1.0));
            weight += w;
            avg += AUDIO_TEXEL(tex, int(round(s))).r * w;
        }
        avg /= weight;
        return avg;
    } else if (sample_mode == 2) {
        float vmax = 0.0, avg = 0.0, weight = 0.0, v;
        for (s = smin; s < smax; s += 1.0F) {
            w = ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0.0, 1.0));
            weight += w;
            v = AUDIO_TEXEL(tex, int(round(s))).r * w;
            avg += v;
            if (vmax < v)
                vmax = v;
        }
        return (vmax * (1.0 - sample_hybrid_weight)) + ((avg / weight) * sample_hybrid_weight);
    } else if (sample_mode == 1) {
        float vmax = 0.0, v;
        for (s = smin; s < smax; s += 1.0F) {
            w = AUDIO_TEXEL(tex, int(round(s))).r * ROUND_FORMULA(clamp((m - abs(rm - s)) / m, 0.0, 1.0));
            if (vmax < w)
                vmax = w;
        }
        return vmax;
    }

    return 0.0;
}

#ifdef TWOPI
#undef TWOPI
#endif
#ifdef PI
#undef PI
#endif
