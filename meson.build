project('meltdown-chess-engine', 'cpp',
  meson_version: '>=1.1',
  version: 'dev-1.0',
  default_options: ['warning_level=3', 'cpp_std=c++26'])

subdir('version')

# set these values as high as possible - required to compile the magic tables
add_project_arguments('-fconstexpr-loop-limit=2147483647', language: 'cpp')
add_project_arguments('-fconstexpr-ops-limit=100000000000', language: 'cpp')

# default compile with native cpu instructions
# TODO: make an option to allow for better cross compilation
add_project_arguments('-march=native', language: 'cpp')

if get_option('buildtype') != 'debug'
  add_project_arguments('-funroll-loops', '-m64', '-finline-functions',  language : 'cpp')
endif

# magic_enum
magic_enum = subproject('magic_enum', default_options: ['test=false'])
magic_enum_dep = magic_enum.get_variable('magic_enum_dep')

# fmt
fmt = subproject('fmt')
fmt_dep = fmt.get_variable('fmt_dep')

meltdown_dep = declare_dependency(include_directories: 'src', dependencies: [magic_enum_dep, fmt_dep])

meltdown = executable('meltdown-chess-engine', 'src/main.cpp',
  install : true,
  dependencies: [meltdown_dep])

test('meltdown-chess-engine', meltdown)

if get_option('unit-tests')
  subdir('tests')
endif
