SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
CC = gcc
CFLAGS = -g -std=c99
LDFLAGS = -lm

default: simulator

simulator: $(SRCS) $(HEADERS)
	@echo "Building $@..."
	@echo "Sources: $(SRCS)"
	@echo "Headers: $(HEADERS)"
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

clean:
	-rm -f simulator
	-rm -f *dump.txt
	-rm -f pipe_trace.txt