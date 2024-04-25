CC = gcc
ARGS = -Wall


all: wordquest_server wordquest_client 

wordquest_server: wordquest_server.c
	$(CC) $(ARGS) -o wordquest_server wordquest_server.c -pthread

wordquest_client: wordquest_client.c
	$(CC) $(ARGS) -o wordquest_client wordquest_client.c

clean:
	rm -f *.o wordquest_server *~
	rm -f *.o wordquest_client *~