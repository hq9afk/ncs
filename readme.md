# ncs

A single-visualiser cut of [WayVes](https://roonil.github.io): an OpenGL ES audio visualiser for Wayland. It captures audio from PipeWire, runs an FFT and a few GPU transform passes over it, and draws the `ncs` shader (a Perlin-noise blob that reacts to the sound) with a `glow` post-processing pass, in a click-through, borderless `xdg_toplevel` window it drives through the Wayland protocol directly (no toolkit).

## Build

Dependencies: `wayland-client`, `wayland-egl`, `wayland-protocols`, `egl`, `libpipewire-0.3`, `libepoxy`, `meson`, `ninja`, `python3`.

Runtime: an `OpenGL ES 3.2` context with the `GL_EXT_texture_norm16` extension.

```sh
./build.sh          # meson setup build (first run) + meson compile
./build/ncs         # run
./build/ncs -V      # print version
```

The shaders in [`shaders/`](shaders/) are embedded into the binary at build time by [`src/embed_resources.py`](src/embed_resources.py), so the installed executable is self-contained.

## Shaders

`shaders/<name>-<n>.vert` / `.frag` are the pipeline stages for a given `<name>` (`ncs` for the visualiser, `glow` for post-processing, `audio-*` for the transform passes). Two preprocessor directives run before GLSL compilation:

- `#include ":file.glsl"` - absolute include, resolved against the embedded shader set. `#include ":$CONFIGFILE"` pulls in the visualiser's config GLSL (`ncs.glsl` by default).
- `#expand fn N` - emits `fn(0); fn(1); ... fn(N-1);`, where `N` is an integer or a build-time variable. If the body contains `#` it is a template: `#` is replaced by the index and the line emitted verbatim (GLSL ES has no `##` paste operator).

## Audio transform passes

The pass / gravity / averaging / smoothing shaders under `shaders/audio-*` are adapted from [GLava](https://github.com/jarcode-foss/glava) by jarcode-foss and are licensed under GPL-3.0, as is the radix-2 FFT in [`src/shader_program.cpp`](src/shader_program.cpp).
