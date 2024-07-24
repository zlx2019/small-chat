/** server.c -- SmallChat Client
 * 
 * 
 * 
 * 
 * 
 * 
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/select.h>
#include<termios.h>
#include<errno.h>
#include "chatlib.h"



/**
 * 将fd对应的io设备设置为 '原始模式'
 */
int setRawMode(int fd, int enable){

}


/**
 * 将终端标准输入 设置为原始模式
 */
void terminalInRawMode(){
    setRawMode(fileno(stdin), 1);
}

/**
 * 将终端标准输入恢复为默认模式
 */
void terminalInRecover(void){
    setRawMode(STDIN_FILENO, 0);
}




// 缓冲区上限
#define BUF_MAX 128 

/**
 * 自定义标准输入缓冲区
 */
struct InputBuffer {
    char buf[BUF_MAX];  // 缓冲数据
    int len;            // 当前长度
};

/**
 * Client main.
 */
int main(int argc, char **args){
    if (argc != 3){
        printf("Usage: %s <host> <port>\n", args[0]);
        exit(1);
    }
    // 与服务端建立TCP连接
    int server = TCPConnect(args[1], atoi(args[2]), 0);
    if (server == -1){
        perror("Connecting to server");
        exit(1);
    }

    /* 将终端标准输入，设置为原始模式.
     *  - 即无缓冲区，每次单击事件都能收到.
     *  - 也不转义任何特殊字符.
     */
    terminalInRawMode();
    return 0;
}