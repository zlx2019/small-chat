#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <errno.h>

/** ======================== 底层网络相关函数库  ================================
 *
 *
 * =========================================================================== */


/**
 * 接收TCP客户端连接函数
 * 接收成功返回其连接描述符，失败则返回-1
 *
 * @param server_socket TCP服务套接字
 */
int acceptClient(int server_socket){
    int client_fd;
    while (1){
        // 客户端信息载体
        struct sockaddr_in client;
        socklen_t  clen = sizeof(client);
        // 等待客户端连接
        client_fd = accept(server_socket, (struct sockaddr*)&client, &clen);
        // 判断是否连接成功
        if (client_fd == -1){
            if (errno == EINTR){
                // 如果是由于系统调用中断所导致连接失败，则继续尝试接收连接
                continue;
            } else{
                return -1;
            }
        }
        break;
    }
    return client_fd;
}



/**
 * 自定义内存分配函数,当内存不足时，则程序直接结束;
 * @param size 要分配的内存大小
 */
void* chatMalloc(size_t size){
    void* ptr;
    if ((ptr = malloc(size)) == NULL){
        perror("Out of memory");
        exit(1);
    }
    return ptr;
}

/**
 * 自定义内存重新分配函数，当内存不足于分配时，则程序直接结束;
 * @param ptr  重新分配的内存首地址
 * @param size 重新分配的内存大小
 */
void* chatRealloc(void* ptr, size_t size){
    if ((ptr = realloc(ptr,size)) == NULL){
        perror("Out of memory");
        exit(1);
    }
    return ptr;
}