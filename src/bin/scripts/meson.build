# Copyright (c) 2022-2024, PostgreSQL Global Development Group

scripts_common = static_library('libscripts_common',
  files('common.c'),
  dependencies: [frontend_code, libpq],
  kwargs: internal_lib_args,
)

binaries = [
  'pg_createdb',
  'pg_dropdb',
  'pg_createuser',
  'pg_dropuser',
  'pg_clusterdb',
  'pg_vacuumdb',
  'pg_reindexdb',
  'pg_isready',
]

foreach binary : binaries
  binary_sources = files('@0@.c'.format(binary))

  if host_system == 'windows'
    binary_sources += rc_bin_gen.process(win32ver_rc, extra_args: [
      '--NAME', binary,
      '--FILEDESC', '@0@ - PostgreSQL utility'.format(binary),])
  endif

  binary = executable(binary,
    binary_sources,
    link_with: [scripts_common],
    dependencies: [frontend_code, libpq],
    kwargs: default_bin_args,
  )
  bin_targets += binary
endforeach

tests += {
  'name': 'scripts',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'tap': {
    'env': {'with_icu': icu.found() ? 'yes' : 'no'},
    'tests': [
      't/010_pg_clusterdb.pl',
      't/011_pg_clusterdb_all.pl',
      't/020_pg_createdb.pl',
      't/040_pg_createuser.pl',
      't/050_pg_dropdb.pl',
      't/070_pg_dropuser.pl',
      't/080_pg_isready.pl',
      't/090_pg_reindexdb.pl',
      't/091_pg_reindexdb_all.pl',
      't/100_pg_vacuumdb.pl',
      't/101_pg_vacuumdb_all.pl',
      't/102_pg_vacuumdb_stages.pl',
      't/200_connstr.pl',
    ],
  },
}

subdir('po', if_found: libintl)
