# Makefile for FC with the SDL2 dependency
#
# Requires SDL2 to be installed:
#   Termux:        pkg install sdl2
#   Debian/Ubuntu: sudo apt install libsdl2-dev
#   Fedora:        sudo dnf install SDL2-devel
#   macOS:         brew install sdl2
#   Windows:       install SDL2 (MSYS2: pacman -S mingw-w64-x86_64-SDL2)
#
# Build:   make
# Run:     ./fcg win_demo.fc
# Install: make install   (copies to $PREFIX/bin/fcg on Termux)

CC ?= cc

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2 2>/dev/null || echo -lSDL2)

CFLAGS ?= -O2 -std=gnu11 -Wall -Wextra
CFLAGS += -DFC_USE_SDL $(SDL_CFLAGS)
LDLIBS  = -lm $(SDL_LIBS)

SRCS = main.c lexer.c parser.c interpreter.c modules.c others.c stats_module.c moremods.c daily.c advmods.c win_module.c
OBJS = $(SRCS:.c=.o)
HEADERS = lexer.h parser.h interpreter.h
BIN = fcg

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BIN)
	cp $(BIN) $(PREFIX)/bin/$(BIN)

run: $(BIN)
	./$(BIN) win_demo.fc

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all install run clean
