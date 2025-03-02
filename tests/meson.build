catch2 = subproject('catch2')
catch2_dep = catch2.get_variable('catch2_with_main_dep')

unit_test_sources = files(
  'src/test_perft.cpp',
  'src/test_zobrist_hashing.cpp',
  'src/test_tt_hash_table.cpp',
  'src/test_move_gen_hashing.cpp',
  'src/test_move_ordering.cpp',
  'src/test_killer_moves.cpp',
  'src/test_history_moves.cpp',
  'src/test_pv_table.cpp',
)

exe = executable(
  'unit-tests',
  unit_test_sources,
  dependencies: [catch2_dep, meltdown_dep],
)

test('unit-tests', exe)
