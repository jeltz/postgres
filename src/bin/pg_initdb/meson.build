# Copyright (c) 2022-2024, PostgreSQL Global Development Group

pg_initdb_sources = files(
  'findtimezone.c',
  'pg_initdb.c'
)

pg_initdb_sources += timezone_localtime_source

if host_system == 'windows'
  pg_initdb_sources += rc_bin_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'pg_initdb',
    '--FILEDESC', 'pg_initdb - initialize a new database cluster',])
endif

pg_initdb = executable('pg_initdb',
  pg_initdb_sources,
  include_directories: [timezone_inc],
  # Note: it's important that we link to encnames.o from libpgcommon, not
  # from libpq, else we have risks of version skew if we run with a libpq
  # shared library from a different PG version.  Define
  # USE_PRIVATE_ENCODING_FUNCS to ensure that that happens.
  c_args: ['-DUSE_PRIVATE_ENCODING_FUNCS'],
  dependencies: [frontend_code, libpq, icu, icu_i18n],
  kwargs: default_bin_args,
)
bin_targets += pg_initdb

tests += {
  'name': 'pg_initdb',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'tap': {
    'env': {'with_icu': icu.found() ? 'yes' : 'no'},
    'tests': [
      't/001_pg_initdb.pl',
    ],
  },
}

subdir('po', if_found: libintl)
