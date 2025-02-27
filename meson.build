project('meltdown-chess-engine', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++26'])

# set these values as high as possible - required to compile the magic tables
add_project_arguments('-fconstexpr-loop-limit=2147483647', language: 'cpp')
add_project_arguments('-fconstexpr-ops-limit=100000000000', language: 'cpp')

# default compile with native cpu instructions
# TODO: make an option to allow for better cross compilation
add_project_arguments('-march=native', language: 'cpp')

if get_option('buildtype') != 'debug'
  add_project_arguments('-Ofast', '-funroll-loops', '-m64', '-finline-functions',  language : 'cpp')
endif


# header only dependency - TODO: add as wrap dependency instead
magic_enum_dep = declare_dependency(include_directories: 'externals/magic_enum/include/')
meltdown_dep = declare_dependency(include_directories: '..', dependencies: magic_enum_dep)

meltdown = executable('meltdown-chess-engine', 'src/main.cpp',
  install : true,
  dependencies: [meltdown_dep])

test('meltdown-chess-engine', meltdown)

if get_option('unit-tests')
  subdir('tests')
endif
