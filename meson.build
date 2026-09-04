project('wayves', ['c', 'cpp'], version: '1.2.0', meson_version: '>= 1.8.0')

cc = meson.get_compiler('cpp')

dependencies = [
  dependency('wayland-client'),
  dependency('wayland-egl'),
  dependency('egl'),
  dependency('libpipewire-0.3'),
  dependency('epoxy'),
  cc.find_library('m'),
]

wl_mod = import('wayland')
protocol_srcs = wl_mod.scan_xml(
  wl_mod.find_protocol('xdg-shell'),
  wl_mod.find_protocol('xdg-decoration', state: 'unstable', version: 1),
)

sources = files(
  'src/main.cpp',
  'src/audio_shader_stages.cpp',
  'src/files.cpp',
  'src/g_application_handler.cpp',
  'src/pipewire_handler.cpp',
  'src/shader_files.cpp',
  'src/shader_program.cpp',
  'src/shaders.cpp',
  'src/window_handler.cpp',
)

# ponytail: regenerates on every build (build_always_stale) instead of listing
# every shader file as an `input:` dependency; fine at this file count/size.
embedded_resources = custom_target(
  'embedded-resources',
  output: 'embedded_resources.cpp',
  command: [
    find_program('python3'),
    files('src/embed_resources.py'),
    meson.project_source_root(),
    '@OUTPUT@',
  ],
  build_always_stale: true,
)

executable(
  'ncs',
  sources,
  embedded_resources,
  protocol_srcs,
  include_directories: include_directories('include'),
  dependencies: dependencies,
  install: true,
)
