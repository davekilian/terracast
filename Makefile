CFLAGS=-std=c2x -pedantic -Wall -Wextra -Werror -Wno-comment -g -DDEBUG
LFLAGS=
INCLUDES=-I.
LIBS=

PCH=pch.h.pch
OUT=bin

DB=$(OUT)/db
DBDIRS=db runtime math collide taskman store protect table index query sql

TILER=$(OUT)/tiler
TILERDIRS=tiler runtime math

ED=$(OUT)/ed
EDDIRS=ed runtime math collide

TEST=$(OUT)/test
TESTDIRS=test runtime math collide taskman store protect table index query sql

all: $(DB) $(TILER) $(ED) $(TEST)

sources-for=$(foreach dir,$(1),$(wildcard $(dir)/*.c))
objects-for=$(patsubst %.c,%.o,$(call sources-for,$(1)))
deps-for=$(patsubst %.c,%.d,$(call sources-for,$(1)))

$(DB): $(OUT) $(call objects-for,$(DBDIRS)) Makefile
	$(CC) $(LFLAGS) $(LIBS) $(call objects-for,$(DBDIRS)) -o $(DB)
-include $(call deps-for,$(DBDIRS))

$(TILER): $(OUT) $(call objects-for,$(TILERDIRS)) Makefile
	$(CC) $(LFLAGS) $(LIBS) $(call objects-for,$(TILERDIRS)) -o $(TILER)
-include $(call deps-for,$(TILERDIRS))

$(ED): $(OUT) $(call objects-for,$(EDDIRS)) Makefile
	$(CC) $(LFLAGS) $(LIBS) $(call objects-for,$(EDDIRS)) -o $(ED)
-include $(call deps-for,$(EDDIRS))

$(TEST): $(OUT) $(call objects-for,$(TESTDIRS)) Makefile
	$(CC) $(LFLAGS) $(LIBS) $(call objects-for,$(TESTDIRS)) -o $(TEST)
-include $(call deps-for,$(TESTDIRS))

dist: CFLAGS := $(filter-out -g, $(CFLAGS))
dist: CFLAGS := $(filter-out -DDEBUG, $(CFLAGS))
dist: CFLAGS += -O3 -flto
dist: LFLAGS += -O3 -flto
dist: all

%.o: %.c $(PCH) Makefile
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -include pch.h -c $< -o $@

$(PCH): pch.h Makefile
	$(CC) $(filter-out -flto, $(CFLAGS)) $(INCLUDES) -x c-header pch.h -o $(PCH)

test: $(TEST)
	$(TEST)

clean:
	$(RM) $(PCH) $(DB) $(TILER) $(TEST) $(call objects-for,*) $(call deps-for,*)

$(OUT):
	mkdir $(OUT)

.PHONY: all clean test dist