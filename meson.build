project(
    'cws',
    'c',
    version: '0.1.1',
    default_options: ['c_std=gnu23', 'warning_level=3'],
)

incdir = include_directories('include')
srcdir = include_directories('src')

include_dirs = [incdir, srcdir]

subdir('src')

libtomlc17 = dependency('libtomlc17', required: true)
libmyclib = dependency('myclib', required: true)

deps = [libtomlc17, libmyclib]

add_global_arguments('-DUSE_COLORS', language: 'c')
add_global_arguments('-DEVELOPER', language: 'c')
add_global_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

exe = executable('cws', server, include_directories: include_dirs, dependencies: deps)
subdir('test')

# Commands

clangformat = find_program('clang-format', required: false)
if clangformat.found()
    run_target(
        'format',
        command: [
            'ninja',
            '-C', join_paths(meson.current_build_dir()),
            'clang-format',
        ],
    )
endif