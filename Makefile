all: server

server: server.c
		gcc -c -std=c99 -o server.o server.c
		gcc server.o -o server


clean:
	rm -f *.o server *~``