catch2 = subproject('catch2')
catch2_dep = catch2.get_variable('catch2_with_main_dep')

unit_test_sources = files(
  'src/perft.cpp',
  'src/zobrist_hashing.cpp',
  'src/tt_hash_table.cpp',
  'src/move_gen_hashing.cpp'
)

exe = executable(
  'unit-tests',
  unit_test_sources,
  dependencies: [catch2_dep, meltdown_dep],
)

test('unit-tests', exe)
