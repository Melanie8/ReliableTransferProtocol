CC=gcc
CFLAGS=-I.
# Les .h
DEPS =
OBJ_S = sender.o
OBJ_R = receiver.o

all: sender receiver

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(OBJ_S)
	$(CC) -o $@ $^ $(CFLAGS)

receiver: $(OBJ_R)
	$(CC) -o $@ $^ $(CFLAGS)
