# Makefile for the FC programming language interpreter
# Usage:
#   make            build the 'fc' binary
#   make run        build and start the interactive REPL
#   make clean      remove build artifacts

CC      ?= cc
CFLAGS  ?= -O2 -std=gnu11 -Wall -Wextra
LDLIBS  ?= -lm

SRCS = main.c lexer.c parser.c interpreter.c modules.c others.c stats_module.c moremods.c daily.c advmods.c
OBJS = $(SRCS:.c=.o)
HEADERS = lexer.h parser.h interpreter.h
BIN = fc

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all run clean
