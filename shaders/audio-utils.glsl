#ifndef _COMMON_GLSL
#define _COMMON_GLSL

#ifndef TWOPI
#define TWOPI 6.28318530718
#endif

#ifndef PI
#define PI 3.14159265359
#endif

/* Window value t that resides in range [0, sz] */
#define window(t, sz) (0.53836 - (0.46164 * cos(TWOPI * t / sz)))
/* Take value x that scales linearly between [0, 1) and return its sinusoidal
 * curve */
#define sinusoidal(x) ((0.5 * sin((PI * (x)) - (PI / 2.0))) + 0.5)

#endif