CC = mpicc
CFLAGS = -Wall
.PHONY: all clean run build
LIBS = -lm

all: build

build: $(wildcard *.c)
	$(CC) $(CFLAGS) $^ -o tema3 $(LIBS)
clean:
	rm -rf tema3
