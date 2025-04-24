project('cws', 'c', version : '0.1.0')

subdir('src')

incdir = include_directories('include')

libyaml = dependency('yaml-0.1')
libcyaml = dependency('libcyaml')

executable('cws', server, include_directories : incdir, dependencies : [libyaml, libcyaml])
