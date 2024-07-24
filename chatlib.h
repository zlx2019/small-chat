/** chatlib.h 基础库头文件 **/

#ifndef CHATLIB_H
#define CHATLIB_H
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
    /* ===================== Networking ===================== */

    /**
     * 创建TCP服务，并且返回服务socket描述符，失败返回-1.
     * 
     * @param port: 服务使用的端口号
     */
    int createTCPServer(int port);

    /**
     * 与指定地址建立 TCP 连接, 并且返回连接 socket 描述符，失败返回-1.
     * 
     * @param addr: TCP服务端地址
     * @param port: TCP服务端口
     * @param nonblock: 非阻塞模式标志
     */
    int TCPConnect(char* addr, int port, int nonblock);

    /**
     * 阻塞从服务中接收一个TCP连接，并返回该连接的 socket 描述符，失败返回-1.
     * 
     * @param server_socket: socket 服务 
     */
    int acceptClient(int server_socket);

    /**
     * 将套接字描述符设置为非阻塞模式，且不带延迟标志，成功返回0，错误返回-1.
     */
    int socketSetNonBlockNoDelay(int fd);

    /* 读取 io数据 读取到缓冲区 失败返回-1 */
    ssize_t Read(int fd, void* buf, size_t len);
    /* 将 缓冲区数据 写入io 失败返回-1 */
    ssize_t Write(int fd, const void* buf, size_t len);



    /* ===================== Allocation ===================== */
    /* 自定义内存分配函数 */
    void* chatMalloc(size_t size);
    /* 自定义内存重新分配函数 */
    void* chatRealloc(void* ptr, size_t size);
#ifdef __cplusplus
}
#endif
#endif