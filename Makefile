# 项目编译(客户端 && 服务端)
all: smallchat-server smallchat-client
# 编译参数
CFLAGS=-O2 -Wall -W -std=c99

# 编译 && 运行服务端
run-server: smallchat-server
	bin/server
# 编译 && 运行客户端
run-client: smallchat-client
	bin/client

# make server
smallchat-server: small-server.c chatlib.c
	$(CC) small-server.c chatlib.c -o bin/server $(CFLAGS)

# make client
smallchat-client: small-client.c chatlib.c
	$(CC) small-client.c chatlib.c -o bin/client $(CFLAGS)

# clean make
clean:
	rm -f bin/server
	rm -f bin/client