CC = gcc
CFLAGS = -g -Wall

a.out: server

server: quacker.c
	$(CC) $(CFLAGS) quacker.c -o server

clean:
	$(RM) server
