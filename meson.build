project('cws', 'c', version : '0.1.0')

cc = meson.get_compiler('c')

subdir('src')
subdir('include')

incdir = include_directories('include')

libssl = dependency('libssl')
libyaml = dependency('yaml-0.1')
libcyaml = dependency('libcyaml')
libmath = cc.find_library('m', required : true)
deps = [libssl, libyaml, libcyaml, libmath]

add_global_arguments('-DUSE_COLORS', language : 'c')
add_global_arguments('-DEVELOPER', language : 'c')

executable('cws', server + myclib, include_directories : incdir, dependencies : deps)
