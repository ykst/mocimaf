TARGET=$(shell basename $(CURDIR))
CC=gcc
CFLAGS_BASE= -Wall -std=gnu99 `sdl2-config --cflags`
SRCS=$(filter-out %.template.c,$(wildcard *.c))
OBJS=$(SRCS:%.c=%.o)
HEADERS=$(filter-out %.template.h,$(wildcard *.h))
GENS=$(wildcard *.src)
LDFLAGS= -lncurses `sdl2-config --libs`

.PHONY: all test clean debug release nestest
all: release

debug: CFLAGS = $(CFLAGS_BASE) -O0 -g -DDEBUG=1
debug: $(TARGET)

release: CFLAGS = $(CFLAGS_BASE) -O3
release: $(TARGET)

nestest: CFLAGS = $(CFLAGS_BASE) -O0 -g -DNESTEST -DDEBUG=1
nestest: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%o: %c $(HEADERS) $(GENS)
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET)

test: all
	test/regression.sh
