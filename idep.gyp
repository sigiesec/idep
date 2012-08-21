{
  'targets': [
    {
      'target_name': 'idep',
      'type': 'static_library',
      'dependencies': [
      ],
      'sources': [
        'basictypes.h',
        'idep_alias_dep.cc',
        'idep_alias_dep.h',
        'idep_alias_table.cc',
        'idep_alias_table.h',
        'idep_alias_util.cc',
        'idep_alias_util.h',
        'idep_binary_relation.cc',
        'idep_binary_relation.h',
        'idep_compile_dep.cc',
        'idep_compile_dep.h',
        'idep_file_dep_iterator.cc',
        'idep_file_dep_iterator.h',
        'idep_link_dep.cc',
        'idep_link_dep.h',
        'idep_name_array.cc',
        'idep_name_array.h',
        'idep_name_index_map.cc',
        'idep_name_index_map.h',
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
    {
      'target_name': 'cdep',
      'type': 'executable',
      'dependencies': [
        'idep',
      ],
      'sources': [
        'cdep.cc',
      ],
    },
    {
      'target_name': 'ldep',
      'type': 'executable',
      'dependencies': [
        'idep',
      ],
      'sources': [
        'ldep.cc',
      ],
    },
  ],
}
