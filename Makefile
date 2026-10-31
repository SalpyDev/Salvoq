
VERSION = 1.0.0
CC ?= gcc
ifeq ($(origin CC),default)
CC = gcc
endif
CSTD = -std=c17
WARN = -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wstrict-prototypes -Werror
LIBS = -lole32 -luuid -lwinmm -lm
SRC = src/main.c src/platform_win32.c src/fft.c src/window.c src/capture_wasapi.c \
      src/capture_test.c src/capture_process.c src/procfind.c src/analysis.c \
      src/term.c src/canvas.c src/render.c src/selftest.c
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))
LTO = $(shell $(CC) -flto -x c -c /dev/null -o /dev/null 2>/dev/null && echo -flto || true)

all: build/salvoq.exe

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) $(CSTD) $(WARN) -O3 -march=native -DNDEBUG $(LTO) -DSALVOQ_VERSION=\"$(VERSION)\" -Isrc -c $< -o $@

build/salvoq.exe: $(OBJ) | build
	$(CC) $(CSTD) $(WARN) -O3 -march=native -DNDEBUG $(LTO) -DSALVOQ_VERSION=\"$(VERSION)\" -Isrc $^ $(LIBS) -o $@

debug: build/salvoq-debug.exe

build/salvoq-debug.exe: $(SRC) | build
	$(CC) $(CSTD) $(WARN) -O0 -g -fsanitize=undefined -DSALVOQ_VERSION=\"$(VERSION)\" -Isrc $(SRC) $(LIBS) -o $@

test: build/salvoq.exe
	./build/salvoq.exe --selftest

dist-win: clean
	$(CC) $(CSTD) $(WARN) -O3 -mtune=generic -DNDEBUG $(LTO) -DSALVOQ_VERSION=\"$(VERSION)\" -Isrc $(SRC) -static -static-libgcc $(LIBS) -o dist/salvoq.exe

clean:
	rm -rf build dist

.PHONY: all debug test dist-win clean
