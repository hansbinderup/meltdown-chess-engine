omp_dep = dependency('openmp', required: true)

# create a strong link to the provided files - this makes the build and scripts very flexible
training_data_path = meson.current_source_dir() / 'training-data/lichess-big3-resolved.book'
generated_file = meson.project_source_root() / 'src/evaluation/generated/tuned_terms.h'
add_project_arguments(
  '-DTRAINING_DATA_PATH="@0@"'.format(training_data_path),
  '-DGENERATED_FILE="@0@"'.format(generated_file),
  '-fno-fast-math',
  language: 'cpp'
)

tuner = executable('meltdown-tuner',
           'src/main.cpp',
           install : true,
           include_directories: 'src',
           dependencies: [meltdown_dep, omp_dep])

test('tuner', tuner)
