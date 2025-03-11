catch2 = subproject('catch2')
catch2_dep = catch2.get_variable('catch2_with_main_dep')

# NOTE: must be named test_*** to be picked up by the runner
unit_test_names = [
  'test_perft',
  'test_zobrist_hashing',
  'test_tt_hash_table',
  'test_move_gen_hashing',
  'test_killer_moves',
  'test_history_moves',
  'test_pv_table',
  'test_see_swap',
  'test_scoring',
]

foreach test_name : unit_test_names
  test_src = 'src/' + test_name + '.cpp'  # Construct file path

  exe = executable(
    'meltdown_' + test_name,
    test_src,
    dependencies: [catch2_dep, meltdown_dep]
  )

  test('meltdown_' + test_name, exe)
endforeach

