CC = gcc
CFLAGS = -I../lib -g -O2 -D_REENTRANT -Wall
LIBS = ../libunp.a -lpthread

client:	client.o
		${CC} ${CFLAGS} -o $@ client.o ${LIBS}

server:	server.o
		${CC} ${CFLAGS} -o $@ server.o ${LIBS}

test:	test.o
		${CC} ${CFLAGS} -o $@ test.o ${LIBS}
