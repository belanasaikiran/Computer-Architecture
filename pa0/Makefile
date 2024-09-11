SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
CC = gcc
CFLAGS = -g -std=c99
LDFLAGS = -lm

default: simulator

simulator: $(SRCS) $(HEADERS)
	@echo "Building $@..."
	@#gcc simulator.o -o simulator -ggdb -std=c99
	@echo "Sources: $(SRCS)"
	@echo "Headers: $(HEADERS)"
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	-rm -rf simulator simulator.dSYM
	-rm -f pipe_trace.txt *.out mdump.txt
