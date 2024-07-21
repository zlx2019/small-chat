#include "chatlib.h"
#include "log.h"

/* server.c -- SmallChat 服务端 */



#define MAX_CLIENTS 1000 // 最大客户端连接数
#define SERVER_PORT 7711 // 服务端口


/**
 * 表示一个已连接的客户端
 */
struct client{
    int fd;             // 客户端连接 fd
    char *nick_name;    // 客户端名称
};


/**
 * 全局状态信息
 */
struct chatState{
    int a;
};

int main(void){
    Log("hello %s", "你好");
    Info("hello %s", "你好");
    Debug("hello %s", "你好");
    Error("hello %s", "你好");
    return 0;
}