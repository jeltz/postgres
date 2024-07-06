# src/bin/scripts/nls.mk
CATALOG_NAME     = pgscripts
GETTEXT_FILES    = $(FRONTEND_COMMON_GETTEXT_FILES) \
                   pg_createdb.c \
                   pg_createuser.c \
                   pg_dropdb.c \
                   pg_dropuser.c \
                   pg_clusterdb.c \
                   pg_vacuumdb.c \
                   pg_reindexdb.c \
                   pg_isready.c \
                   common.c \
                   ../../fe_utils/parallel_slot.c \
                   ../../fe_utils/cancel.c \
                   ../../fe_utils/print.c \
                   ../../fe_utils/connect_utils.c \
                   ../../fe_utils/option_utils.c \
                   ../../fe_utils/query_utils.c \
                   ../../fe_utils/string_utils.c \
                   ../../common/fe_memutils.c \
                   ../../common/file_utils.c \
                   ../../common/username.c
GETTEXT_TRIGGERS = $(FRONTEND_COMMON_GETTEXT_TRIGGERS) simple_prompt yesno_prompt
GETTEXT_FLAGS    = $(FRONTEND_COMMON_GETTEXT_FLAGS)
