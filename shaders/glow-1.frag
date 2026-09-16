#version 100
precision highp float;
precision highp int;

uniform vec2 resolution;

uniform sampler2D tex;

#ifndef TWOPI
#define TWOPI 6.28318530718
#endif
#ifndef PI
#define PI 3.14159265359
#endif

#include ":$CONFIGFILE"

float glowLightVal(float glowValue, float glowLightStrengthValue, float glowLightDistanceValue)
{
    return glowLightDistanceValue + glowLightDistanceValue / pow(glowValue, glowLightStrengthValue);
}

vec4 addColors(float blendMode, vec4 above, vec4 below)
{
    return above + (1. - blendMode * above.w) * below;
}

#expand Glow glow#; postProcessingNumber

void defaultGlowValues(inout Glow glow)
{
    glow.blendMode = 1.0;
    glow.mixAlpha = 1.0;
    glow.offsetAngle = 0.0;
    glow.size = vec2(10);
    glow.intensity = .5;
    glow.directions = 8.0;
    glow.onTop = 0.0;
    glow.coords = gl_FragCoord.xy;
    glow.maxAngle = 360.0;

    glow.quality = 4.0;
    vec4 color = vec4(0.5, 0.5, 0.5, 1.0);
    glow.brightnessOffset = .0;
    glow.lightStrength = .5;
}

void main()
{

#expand defaultGlowValues(glow#); postProcessingNumber

#expand setGlow#(glow#); postProcessingNumber

    Glow glow;

#expand glow = glow#; postProcessingNumber

    vec2 uv = (glow.coords) / resolution.xy;
    vec4 prevColor = texture2D(tex, gl_FragCoord.xy / resolution.xy);

    vec2 glowRadius = (glow.size) / resolution.xy;
    vec4 Color = vec4(0);

    float glowOffsetValue = (float(glow.offsetAngle) / 360.) * TWOPI;

    for (float d = glowOffsetValue; d < (glow.maxAngle / 360. * TWOPI); d += TWOPI / (glow.directions))
    {
        for (float i = 1.0 / (glow.quality); i <= 1.0; i += 1.0 / (glow.quality))
        {
            vec2 coords = uv + glowRadius * i * vec2(cos(d), sin(d));

            if (coords.x > 0.0 && coords.x < 1.0 && coords.y > 0.0 && coords.y < 1.0)
                Color += texture2D(tex, coords);
        }
    }

    Color /= (glow.quality) * (glow.directions);

    gl_FragColor = (vec4(glow.color.xyz * glow.color.w, glow.color.w)) * glow.intensity * length(Color);

    gl_FragColor = addColors(glow.blendMode, mix(prevColor, gl_FragColor, glow.onTop), mix(gl_FragColor, prevColor, glow.onTop));

    gl_FragColor *= glowLightVal(length(gl_FragColor), glow.brightnessOffset, glow.lightStrength);

    gl_FragColor.w = mix(prevColor.w, gl_FragColor.w, glow.mixAlpha * 0.5);
}
