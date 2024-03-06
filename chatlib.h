/** chatlib.h 基础库头文件 **/

#ifdef CHATLIB_H
#define CHATLIB_H


/* 创建TCP服务，返回监听文件描述符 */
int createTCPServer(int port);

/* 将套接字描述符设置为非阻塞模式，且不带延迟标志，成功返回0，错误返回-1 */
int socketSetNonBlockNoDelay(int fd);

/* 创建一个 TCP 套接字并将其连接到指定地址,成功时返回套接字描述符，否则返回-1 */
int TCPConnect(char* addr, int port, int nonblock);

/* 接收客户端连接，并且返回客户端连接的文件描述符 */
int acceptClient(int server_socket);

/* 自定义内存分配函数 */
void* chatMalloc(size_t size);

/* 自定义内存重新分配函数 */
void* chatRealloc(void* ptr, size_t size);


#endif