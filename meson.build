project('cws', 'c', version: '0.1.0', default_options: ['c_std=c23', 'warning_level=3'])

cc = meson.get_compiler('c')

subdir('src')

incdir = include_directories('include')

libssl = dependency('libssl')
libyaml = dependency('yaml-0.1')
libcyaml = dependency('libcyaml')
libmath = cc.find_library('m', required: true)
libmyclib = cc.find_library('myclib', required: true)

deps = [libssl, libyaml, libcyaml, libmath, libmyclib]

add_global_arguments('-DUSE_COLORS', language: 'c')
add_global_arguments('-DEVELOPER', language: 'c')

executable('cws', server, include_directories: incdir, dependencies: deps)
