EXTENSION = dsm_spinlock
EXTVERSION = 1.0

MODULE_big = dsm_spinlock
OBJS = dsm_spinlock.o

DATA = dsm_spinlock--1.0.sql

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)
