meltdown_version = get_option('meltdown-version')
meltdown_hash = run_command('git', 'rev-parse',  '--short', 'HEAD').stdout().strip()
meltdown_build_type = get_option('buildtype')
meltdown_authors = run_command('git', 'shortlog',  '-sn', 'HEAD').stdout().replace('\n', '\\n"\n"')

conf_data = configuration_data({
    'MELTDOWN_VERSION' : meltdown_version,
    'MELTDOWN_HASH' : meltdown_hash,
    'MELTDOWN_BUILD_TYPE' : meltdown_build_type,
    'MELTDOWN_AUTHORS': meltdown_authors,
})

configure_file(
  input : 'version.h.in',
  output : 'version.h',
  configuration : conf_data,
)

version_inc = include_directories('.')

