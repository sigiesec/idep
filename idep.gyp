{
  'targets': [
    {
      'target_name': 'idep',
      'type': 'static_library',
      'dependencies': [
      ],
      'sources': [
        'idep_alias_dep.cc',
        'idep_alias_dep.h',
        'idep_alias_table.cc',
        'idep_alias_table.h',
        'idep_alias_util.cc',
        'idep_alias_util.h',
        'idep_binrel.cc',
        'idep_binrel.h',
        'idep_name_array.cc',
        'idep_name_array.h',
        'idep_string.cc',
        'idep_string.h',
        'idep_token_iterator.cc',
        'idep_token_iterator.h',
      ],
    },
    {
      'target_name': 'adep',
      'type': 'executable',
      'dependencies': [
        'idep',
      ],
      'sources': [
        'adep.cc',
      ],
    },
  ],
}
