project('cws', 'c', version : '0.1.0')

subdir('src')

incdir = include_directories('include')

libssl = dependency('libssl')
libyaml = dependency('yaml-0.1')
libcyaml = dependency('libcyaml')
deps = [libssl, libyaml, libcyaml]

add_global_arguments('-DUSE_COLORS', language : 'c')
add_global_arguments('-DEVELOPER', language : 'c')

executable('cws', server, include_directories : incdir, dependencies : deps)

executable('testbuild', test, include_directories : incdir)
