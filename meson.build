project(
    'cws',
    'c',
    version: '0.1.1',
    default_options: ['c_std=gnu23', 'warning_level=3'],
)

add_global_arguments('-Wno-pedantic', language: 'c')

cc = meson.get_compiler('c')

incdir = include_directories('include')
srcdir = include_directories('src')

include_dirs = [incdir, srcdir]

subdir('src')

libtomlc17 = dependency('libtomlc17', required: true)
libmyclib = cc.find_library('myclib', required: true)

deps = [libtomlc17, libmyclib]

add_global_arguments('-DUSE_COLORS', language: 'c')
add_global_arguments('-DEVELOPER', language: 'c')
add_global_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

exe = executable('cws', server, include_directories: include_dirs, dependencies: deps)

# Test
test_src = files('test/server.c')
test_curl_dep = dependency('libcurl', required: false)
test_deps = [test_curl_dep]
test_exec = executable('test_http', test_src, dependencies: test_deps)

test('index get', test_exec, args: [exe.full_path()])