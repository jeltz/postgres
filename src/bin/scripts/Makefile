#-------------------------------------------------------------------------
#
# Makefile for src/bin/scripts
#
# Portions Copyright (c) 1996-2024, PostgreSQL Global Development Group
# Portions Copyright (c) 1994, Regents of the University of California
#
# src/bin/scripts/Makefile
#
#-------------------------------------------------------------------------

PGFILEDESC = "PostgreSQL utility"
PGAPPICON=win32

subdir = src/bin/scripts
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

PROGRAMS = pg_createdb pg_createuser pg_dropdb pg_dropuser pg_clusterdb pg_vacuumdb pg_reindexdb pg_isready

override CPPFLAGS := -I$(libpq_srcdir) $(CPPFLAGS)
LDFLAGS_INTERNAL += -L$(top_builddir)/src/fe_utils -lpgfeutils $(libpq_pgport)

all: $(PROGRAMS)

pg_createdb: pg_createdb.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_createuser: pg_createuser.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_dropdb: pg_dropdb.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_dropuser: pg_dropuser.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_clusterdb: pg_clusterdb.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_vacuumdb: pg_vacuumdb.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_reindexdb: pg_reindexdb.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils
pg_isready: pg_isready.o common.o $(WIN32RES) | submake-libpq submake-libpgport submake-libpgfeutils

install: all installdirs
	$(INSTALL_PROGRAM) pg_createdb$(X)   '$(DESTDIR)$(bindir)'/pg_createdb$(X)
	$(INSTALL_PROGRAM) pg_dropdb$(X)     '$(DESTDIR)$(bindir)'/pg_dropdb$(X)
	$(INSTALL_PROGRAM) pg_createuser$(X) '$(DESTDIR)$(bindir)'/pg_createuser$(X)
	$(INSTALL_PROGRAM) pg_dropuser$(X)   '$(DESTDIR)$(bindir)'/pg_dropuser$(X)
	$(INSTALL_PROGRAM) pg_clusterdb$(X)  '$(DESTDIR)$(bindir)'/pg_clusterdb$(X)
	$(INSTALL_PROGRAM) pg_vacuumdb$(X)   '$(DESTDIR)$(bindir)'/pg_vacuumdb$(X)
	$(INSTALL_PROGRAM) pg_reindexdb$(X)  '$(DESTDIR)$(bindir)'/pg_reindexdb$(X)
	$(INSTALL_PROGRAM) pg_isready$(X)    '$(DESTDIR)$(bindir)'/pg_isready$(X)
	$(LN_S) pg_createdb$(X)   '$(DESTDIR)$(bindir)'/createdb$(X)
	$(LN_S) pg_dropdb$(X)     '$(DESTDIR)$(bindir)'/dropdb$(X)
	$(LN_S) pg_createuser$(X) '$(DESTDIR)$(bindir)'/createuser$(X)
	$(LN_S) pg_dropuser$(X)   '$(DESTDIR)$(bindir)'/dropuser$(X)
	$(LN_S) pg_clusterdb$(X)  '$(DESTDIR)$(bindir)'/clusterdb$(X)
	$(LN_S) pg_vacuumdb$(X)   '$(DESTDIR)$(bindir)'/vacuumdb$(X)
	$(LN_S) pg_reindexdb$(X)  '$(DESTDIR)$(bindir)'/reindexdb$(X)

installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'

uninstall:
	rm -f $(addprefix '$(DESTDIR)$(bindir)'/, $(addsuffix $(X), $(PROGRAMS)))

clean distclean:
	rm -f $(addsuffix $(X), $(PROGRAMS)) $(addsuffix .o, $(PROGRAMS))
	rm -f common.o $(WIN32RES)
	rm -rf tmp_check

export with_icu

check:
	$(prove_check)

installcheck:
	$(prove_installcheck)
