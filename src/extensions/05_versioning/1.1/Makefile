EXTENSION = versioning
EXTVERSION = 1.1

MODULE_big = versioning
OBJS = versioning.o

DATA = $(wildcard versioning--*.sql)

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)
