EXTENSION = custom_datatype
EXTVERSION = 1.0

MODULE_big = custom_datatype
OBJS = custom_datatype.o

DATA = custom_datatype--1.0.sql

TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test --load-extension=$(EXTENSION)

PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)

include $(PGXS)
