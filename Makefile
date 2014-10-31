CC=gcc
CFLAGS=-I. -pthread -lz -g
# Les .h
DEPS = src/common.h src/error.h src/mailbox.h src/agent.h src/timer.h src/network.h src/sr.h src/acker.h
OBJ_S = src/sender.o src/common.o src/error.o src/mailbox.o src/agent.o src/timer.o src/network.o src/sr.o src/acker.o
OBJ_R = src/receiver.o src/common.o src/error.o

all: sender receiver

send: sender
	./sender ::1 2141 --file cin --delay 1000 --sber 600

sendbiglorem: sender
	./sender ::1 2141 --file biglorem --delay 10 --sber 500 --splr 500

sendlittlelorem: sender
	./sender ::1 2141 --file littlelorem --delay 100 --sber 200 --splr 200

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
