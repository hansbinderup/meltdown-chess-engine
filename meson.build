project('meltdown-chess-engine', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++26'])

# set these values as high as possible - required to compile the magic tables
add_project_arguments('-fconstexpr-loop-limit=2147483647', language: 'cpp')
add_project_arguments('-fconstexpr-ops-limit=100000000000', language: 'cpp')

# header only dependency
magic_enum_dep = declare_dependency(include_directories: 'externals/magic_enum/include/')

exe = executable('meltdown-chess-engine', 'src/main.cpp',
  install : true,
  dependencies: magic_enum_dep)

test('basic', exe)
