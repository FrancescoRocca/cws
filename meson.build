project('cws', 'c', version : '0.1.0')

subdir('src')

incdir = include_directories('include')

executable('server', server, include_directories : incdir)
executable('client', client, include_directories : incdir)