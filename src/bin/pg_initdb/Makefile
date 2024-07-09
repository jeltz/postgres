#-------------------------------------------------------------------------
#
# Makefile for src/bin/pg_initdb
#
# Portions Copyright (c) 1996-2024, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/bin/pg_initdb/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "pg_initdb - initialize a new database cluster"
PGAPPICON=win32

subdir = src/bin/pg_initdb
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

# Note: it's important that we link to encnames.o from libpgcommon, not
# from libpq, else we have risks of version skew if we run with a libpq
# shared library from a different PG version.  Define
# USE_PRIVATE_ENCODING_FUNCS to ensure that that happens.
override CPPFLAGS := -DUSE_PRIVATE_ENCODING_FUNCS -I$(libpq_srcdir) -I$(top_srcdir)/src/timezone $(ICU_CFLAGS) $(CPPFLAGS)

# We need libpq only because fe_utils does.
LDFLAGS_INTERNAL += -L$(top_builddir)/src/fe_utils -lpgfeutils $(libpq_pgport) $(ICU_LIBS)

# use system timezone data?
ifneq (,$(with_system_tzdata))
override CPPFLAGS += '-DSYSTEMTZDIR="$(with_system_tzdata)"'
endif

OBJS = \
	$(WIN32RES) \
	findtimezone.o \
	pg_initdb.o \
	localtime.o

all: pg_initdb

pg_initdb: $(OBJS) | submake-libpq submake-libpgport submake-libpgfeutils
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@$(X)

# We must pull in localtime.c from src/timezones
localtime.c: % : $(top_srcdir)/src/timezone/%
	rm -f $@ && $(LN_S) $< .

install: all installdirs
	$(INSTALL_PROGRAM) pg_initdb$(X) '$(DESTDIR)$(bindir)/pg_initdb$(X)'
	$(LN_S) pg_initdb$(X) '$(DESTDIR)$(bindir)/initdb$(X)'

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f '$(DESTDIR)$(bindir)/pg_initdb$(X)'

clean distclean:
	rm -f pg_initdb$(X) $(OBJS) localtime.c
	rm -rf tmp_check

# ensure that changes in datadir propagate into object file
pg_initdb.o: pg_initdb.c $(top_builddir)/src/Makefile.global

export with_icu

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)
