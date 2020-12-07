comp : server

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -o client.o -c client.c

server: server.o
	gcc -o server server.o

server.o: server.c
	gcc -o server.o -c server.c