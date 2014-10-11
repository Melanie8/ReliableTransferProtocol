CC=gcc
CFLAGS=-I.
# Les .h
DEPS = common.h
OBJ_S = sender.o common.o
OBJ_R = receiver.o common.o

all: sender receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(OBJ_S)
	$(CC) -o $@ $^ $(CFLAGS)

receiver: $(OBJ_R)
	$(CC) -o $@ $^ $(CFLAGS)
