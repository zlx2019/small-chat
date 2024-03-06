# make server and client
all: smallchat-server smallchat-client
CFLAGS=-O2 -Wall -W -std=c99

# make server
smallchat-server: server.c chatlib.c
	$(CC) server.c chatlib.c -o smallchat-server $(CFLAGS)

# make client
smallchat-client: client.c chatlib.c
	$(CC) client.c chatlib.c -o smallchat-client $(CFLAGS)

# clean make
clean:
	rm -f smallchat-server
	rm -f smallchat-client