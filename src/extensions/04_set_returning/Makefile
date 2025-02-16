EXTENSION = set_returning
EXTVERSION = 1.0

MODULE_big = set_returning
OBJS = set_returning.o

DATA = set_returning--1.0.sql

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test --load-extension=$(EXTENSION)

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)
