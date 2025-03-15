# TODO: this is very primitive. Add support for more build targets
if get_option('ci')
  # very generic setup - should fit almost all CPUs out there
  add_project_arguments('-march=x86-64-v2', '-Ofast', language: 'cpp')
else
  # local build - try to optimize locally
  add_project_arguments('-march=native', '-Ofast', language: 'cpp')
endif
