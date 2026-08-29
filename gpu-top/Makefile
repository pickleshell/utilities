CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
ROCm_ROOT ?= /opt/rocm-6.4.2
ifeq ($(wildcard $(ROCm_ROOT)/lib/libamd_smi.so),)
  ROCm_ROOT := /opt/rocm
endif
NCURSES_CFLAGS := $(shell pkg-config --cflags ncursesw 2>/dev/null)
NCURSES_LIBS   := $(shell pkg-config --libs   ncursesw 2>/dev/null)
AMDSMI_CFLAGS  := -I$(ROCm_ROOT)/include
AMDSMI_LIBS    := -L$(ROCm_ROOT)/lib -lamd_smi -Wl,-rpath,$(ROCm_ROOT)/lib

all: gpu-top test_ollama test_bar

gpu-top: main.c amd.c ollama.c ui.c config.c amd.h ollama.h ui.h config.h
	$(CC) $(CFLAGS) $(NCURSES_CFLAGS) $(AMDSMI_CFLAGS) -pthread \
	    main.c amd.c ollama.c ui.c config.c -o gpu-top $(NCURSES_LIBS) $(AMDSMI_LIBS)

test_ollama: tests/test_ollama.c ollama.c ollama.h
	$(CC) $(CFLAGS) -I. tests/test_ollama.c ollama.c -o test_ollama

test_bar: tests/test_bar.c ui.c ui.h amd.h ollama.h
	$(CC) $(CFLAGS) -I. tests/test_bar.c ui.c -o test_bar $(NCURSES_LIBS)

test_config: tests/test_config.c config.c config.h
	$(CC) $(CFLAGS) -I. tests/test_config.c config.c -o test_config

test: test_ollama test_bar test_config
	./test_ollama && ./test_bar && ./test_config

clean:
	rm -f gpu-top test_ollama test_bar test_config *.o

.PHONY: all test clean
