CC=gcc
CFLAGS=-I. -pthread -lz -g
# Les .h
DEPS = common.h error.h mailbox.h agent.h timer.h network.h sr.h acker.h
OBJ_S = sender.o common.o error.o mailbox.o agent.o timer.o network.o sr.c acker.c
OBJ_R = receiver.o common.o error.o

all: sender receiver

send: sender
	./sender ::1 2141 --file cin --delay 1000

sendbiglorem: sender
	./sender ::1 2141 --file biglorem --delay 1 --sber 500 --splr 500

sendlittlelorem: sender
	./sender ::1 2141 --file littlelorem --delay 1 --sber 10 --splr 10

receive: receiver
	./receiver :: 2141 --file out

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sender: $(OBJ_S)
	$(CC) -o $@ $^ $(CFLAGS)

receiver: $(OBJ_R)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	$(RM) sender receiver *.o
