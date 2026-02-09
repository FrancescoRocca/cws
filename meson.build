project(
  'cws',
  'c',
  version: '0.1.0',
  default_options: ['c_std=c11', 'warning_level=3'],
)

cc = meson.get_compiler('c')

subdir('src')

incdir = include_directories('include')

libtomlc17 = dependency('libtomlc17', required: true)
libmath = cc.find_library('m', required: true)
libmyclib = cc.find_library('myclib', required: true)

deps = [libtomlc17, libmath, libmyclib]

add_global_arguments('-DUSE_COLORS', language: 'c')
add_global_arguments('-DEVELOPER', language: 'c')
add_global_arguments('-D_POSIX_C_SOURCE=200112L', language: 'c')

executable('cws', server, include_directories: incdir, dependencies: deps)
