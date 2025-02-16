EXTENSION = versioning
EXTVERSION = 1.0

MODULE_big = versioning
OBJS = versioning.o

DATA = versioning--1.0.sql

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)
