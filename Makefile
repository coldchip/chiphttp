CC      := gcc
LD      := ld
BIN     := bin
SRCS    := $(wildcard *.c)
EXE     := $(BIN)/http
CFLAGS  := -Wall
LIBS    := -lpthread -Ofast -s
ifeq ($(OS),Windows_NT)
	LIBS := $(LIBS) -lws2_32
endif

.PHONY: clean

all:
	./build.sh compile
run:
	./build.sh run

clean:
	./build.sh clean