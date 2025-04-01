omp_dep = dependency('openmp', required: true)

tuner = executable('meltdown-tuner',
           'src/main.cpp',
           install : true,
           dependencies: [meltdown_dep, omp_dep])

test('tuner', tuner)
