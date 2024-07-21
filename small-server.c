#include "chatlib.h"
#include "log.h"

/* server.c -- SmallChat 服务端 */


int main(void){
    Log("hello %s", "你好");
    Info("hello %s", "你好");
    Debug("hello %s", "你好");
    Error("hello %s", "你好");
    return 0;
}