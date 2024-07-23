#define _POSIX_C_SOURCE 200112L
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/** ======================== 底层网络相关函数库  ================================ */


/* 创建TCP服务，返回监听文件描述符 */
int createTCPServer(int port){
    int server;
    struct sockaddr_in serverAddr;
    int yes = 1;

    /* 创建 socket */
    if ((server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // 设置地址端口复用
    /* 初始化服务端信息 */
    memset(&serverAddr, 0x00, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* 绑定IP端口 */
    if (bind(server, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
        close(server);
        return -1;
    }
    /* 开始监听 */
    if (listen(server, 511) == -1){
        close(server);
        return -1;
    }
    return server;
}

/* 将套接字描述符设置为非阻塞模式，且不带延迟标志 */
int socketSetNonBlockNoDelay(int sockfd){
    int flags, noDelay = 1;

    /* 获取描述符的当前标志 */
    if ((flags = fcntl(sockfd, F_GETFL)) == -1){
        return -1;
    }
    /* 将标志置为非阻塞 */
    if(fcntl(sockfd,F_SETFL, flags | O_NONBLOCK) == -1){
        return -1;
    }
    /* 启用无延迟特性 */
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay)) == -1){
        return -1;
    }
    return 0;
}

/* 创建一个 TCP 套接字并将其连接到指定地址,成功时返回套接字描述符，否则返回-1 */
int TCPConnect(char* addr, int port, int nonblock){
    int server, retval = -1;
    struct addrinfo hints, *servinfo, *p;

    //  port to string
    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);

    // 设置要连接的服务端地址信息
    memset(&hints, 0x00, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // 表示未指定协议，用于获取查询可用地址
    hints.ai_socktype = SOCK_STREAM; 

    /* 将主机名 | 域名 | 字符串格式的IP地址 解析为addrinfo结构，保存到servinfo变量中 */
    if (getaddrinfo(addr,portStr, &hints, &servinfo) != 0)
        return -1;
    /* 一个主机可能存在多个IP，所以结构为链表结构，循环尝试连接其中任何一个IP地址，直到成功或全部失败; */
    for (p = servinfo; p != NULL; p = p->ai_next){
        if ((server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            return -1;
        
        /* 如果需要，就设置为非阻塞 */
        if(nonblock && socketSetNonBlockNoDelay(server) == -1){
            close(server);
            break;
        }
        /* 尝试连接服务端 */
        if(connect(server, p->ai_addr, p->ai_addrlen) == -1){
            if (errno == EINPROGRESS && nonblock)
                return server;
            close(server);
            break;
        }
        /* 连接至服务端成功 */
        retval = server;
        break;
    }

    // 释放服务端地址信息
    freeaddrinfo(servinfo);
    return retval; /* 如果没有连接成功，则返回 -1 */
}

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
            if (errno == EINTR)
                // 如果是由于信号(系统调用)中断所导致，则继续尝试接收连接
                continue;
            return -1;
        }
        break;
    }
    return client_fd;
}



/**
 * 从一个 io 设备中读取数据，放入指定的缓冲区
 */
ssize_t Read(int fd, void* buf, size_t len){
    ssize_t n;
reset:
    if ((n = read(fd, buf, len)) == -1){
        switch (errno){
        case EINTR:
            // 被信号打断，重试
            goto reset;
        case EAGAIN:
            // io为非阻塞式,没有数据可读
            return n;
        }
        return -1;
    }
    return n;
}

/**
 * 将缓冲区的数据，写入到指定的 io 设备中
 */
ssize_t Write(int fd, const void* buf, size_t len){
    ssize_t n;
reset:
    if ((n = write(fd, buf, len)) == -1){
        switch (errno){
        case EINTR:
            goto reset;
        case EAGAIN:
            return n;
        }
        return -1;
    }
    return n;
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