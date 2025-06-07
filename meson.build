project('meltdown-chess-engine', 'cpp',
  meson_version: '>=1.1',
  version: 'dev-1.0',
  default_options: ['warning_level=3', 'cpp_std=c++23'])

subdir('version')

if get_option('buildtype') != 'debug'
  add_project_arguments('-flto=auto', '-Ofast', '-funroll-loops', '-m64', '-finline-functions',  language : 'cpp')

  # allows developers to have a safer build - easier to debug with minimal runtime cost
  if get_option('developer-build') == false
    add_project_arguments('-DNDEBUG',  language : 'cpp')
  endif
endif

if get_option('spsa') == true
    add_project_arguments('-DSPSA',  language : 'cpp')
endif


# magic_enum
magic_enum = subproject('magic_enum', default_options: ['test=false'])
magic_enum_dep = magic_enum.get_variable('magic_enum_dep')

# fmt
fmt = subproject('fmt', default_options: ['default_library=static'])
fmt_dep = fmt.get_variable('fmt_dep')

fathom = subproject('fathom')
fathom_dep = fathom.get_variable('fathom_dep')

meltdown_dep = declare_dependency(
  include_directories: ['src', version_inc],
  dependencies: [magic_enum_dep, fmt_dep, fathom_dep]
)

# statically link the C++ standard library and libgcc to avoid runtime issues
# use full static linking (-static) only on Windows to avoid DLL dependencies
if host_machine.system() == 'windows'
  add_project_link_arguments('-static', '-static-libgcc', '-static-libstdc++', language: 'cpp')
else
  add_project_link_arguments('-static-libgcc', '-static-libstdc++', language: 'cpp')
endif

# build tuner instead of meltdown engine
if get_option('tuning') == true
  add_project_arguments('-DTUNING', language : 'cpp')
  subdir('tuner')
else
  meltdown = executable('meltdown-chess-engine', 'src/main.cpp',
    install : true,
    dependencies: [meltdown_dep])

  test('meltdown-chess-engine', meltdown)

  if get_option('unit-tests')
    subdir('tests')
  endif
endif


