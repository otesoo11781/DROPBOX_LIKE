all: server client

server:
	gcc -o server server.c
client:
	gcc -o client client.c
clean:
	rm -rf *.o server client
