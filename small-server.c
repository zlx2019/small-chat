/**  
 * small-server.c 服务端
 */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/select.h>

#include "chatlib.h"
#include "log.h"

// 最大客户端连接数
#define MAX_CLIENTS 1000
// 服务端口 
#define SERVER_PORT 7711
// 客户端关闭指令
#define EXIT "exit\n"

// 服务端全局状态数据，启动时由`initChat`函数初始化
struct chatState *Chat;


/* 表示一个已连接的客户端 */
struct client{
    // client socket fd
    int fd;         
    // client name
    char *nick_name;
};

/* 全局状态体 */
struct chatState{
    //  server socket fd
    int server_sock;
    // 当前已建立连接的客户端数量
    int num_clients;
    // 当前最大客户端连接 fd
    int max_client;
    // client list
    struct client* clients[MAX_CLIENTS];
};

/**
 * 将新建立的连接(fd)，封装为一个客户端实例
 */
struct client* create_client(int client_fd){
    // 初始昵称: "user-fd"
    char nick[32];
    int nick_len = snprintf(nick, sizeof(nick),"user:%d", client_fd);
    // 初始化客户端
    struct client* client = chatMalloc(sizeof(*client));
    socketSetNonBlockNoDelay(client_fd);
    client->fd = client_fd;
    // 设置昵称
    client->nick_name = chatMalloc(nick_len + 1);
    memcpy(client->nick_name, nick, nick_len);
    // 将连接放入客户端列表
    assert(Chat->clients[client->fd] == NULL);
    Chat->clients[client->fd] = client;
    // 更新当前最大客户端fd
    if (client->fd > Chat->max_client)
        Chat->max_client = client->fd;
    Chat->num_clients++;
    return client;
}

/**
 * 将消息发送给所有客户端(发送者除外)
 */
void sendMessageToAllClientsBut(int sender, char* msg, size_t msg_len){
    for (int j = 0; j <= Chat->max_client; j++){
        if (Chat->clients[j] == NULL || Chat->clients[j]->fd == sender) continue;
        write(Chat->clients[j]->fd, msg, msg_len);
    }
}


/**
 * 关闭客户端,释放资源.
 */
void closeClient(struct client* client){
    Info("Disconnected client fd = %d, nick = %s", client->fd, client->nick_name);
    // 广播退出通知消息
    char notify_message[sizeof(client->nick_name) + 24];
    int notify_len = snprintf(notify_message, sizeof(notify_message), "Player [%s] Quit Chat!\n", client->nick_name);
    sendMessageToAllClientsBut(client->fd, notify_message, notify_len);

    free(client->nick_name);
    close(client->fd);
    Chat->clients[client->fd] = NULL;
    Chat->num_clients--;
    // 如果关闭的是最大客户端，则找出新的最大客户端并且更新
    if (Chat->max_client == client->fd){
        int j;
        for (j = Chat->max_client - 1; j >= 0; j--){
            if (Chat->clients[j] != NULL){
                Chat->max_client = j;
                break;
            }
        }
        if (j == -1)
        // 已经没有客户端
            Chat->max_client = -1;
    }
    free(client);
}



/**
 * 初始化服务端全局状态数据
 */
void initChat(void){
    // alloc memory 
    Chat = chatMalloc(sizeof(*Chat));
    memset(Chat, 0, sizeof(*Chat));
    Chat->max_client = -1;
    Chat->num_clients = 0;
    // Create server listening socket
    Chat->server_sock = createTCPServer(SERVER_PORT);
    if (Chat->server_sock == -1){
        perror("Creating listening socket");
        exit(1);
    }
}

/**
 * Server main function
 * 
 * 实现聊天室主要逻辑:
 *  - 接收新的客户端连接
 *  - 接收客户端发送的消息
 *  - 将消息转发给其他客户端
 */
int main(void){
    // 初始化服务端
    initChat();
    // event loop
    while (1){
        // 需要被 select 监听的描述符集合
        fd_set listen_fds;
        // 设置 select 监听超时时间为 1s
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        // 每次 select 的结果
        int retval;

        // 清除集合，并将 服务监听socket放入集合
        FD_ZERO(&listen_fds);
        FD_SET(Chat->server_sock, &listen_fds);
        // 将所有客户端连接 socket 放入集合
        for (int j = 0; j <= Chat->max_client; j++){
            if (Chat->clients[j])
                FD_SET(j, &listen_fds);
        }
        // 本次要监听最大fd
        int listen_max_fd = Chat->max_client;
        if (listen_max_fd < Chat->server_sock) listen_max_fd = Chat->server_sock;

        // select listen
        retval = select(listen_max_fd + 1, &listen_fds, NULL, NULL, &timeout);
        if (retval == -1){
            // 错误处理
            if (errno == EINTR) {
                // 可能由于信号而被打断
                Error("server listening interrupt dut to signal");
                continue;
            };
            perror("select () error");
            exit(1);
        }else if (retval){
            // 服务端 socket 就绪
            if (FD_ISSET(Chat->server_sock, &listen_fds)){
                int fd = acceptClient(Chat->server_sock);
                struct client* client = create_client(fd);

                // 回复欢迎消息
                char *welcome_message =
                    "Welcome to Small Chat! \n"
                    "Use /nike <nick> to set your nick. \n";
                write(client->fd, welcome_message, strlen(welcome_message));
                Info("Connected client fd = %d", fd);

                // 广播玩家进入通知消息
                char notify_message[sizeof(client->nick_name) + 24];
                int notify_len = snprintf(notify_message, sizeof(notify_message), "Player [%s] enter Chat!\n", client->nick_name);
                sendMessageToAllClientsBut(fd, notify_message, notify_len);
            }

            char buf[256];
            // 遍历检查是否有客户端发送数据
            for (int j = 0; j <= Chat->max_client; j++){
                if (Chat->clients[j] == NULL)
                    continue;
                // 处理事件就绪的客户端
                if (FD_ISSET(j, &listen_fds)){
                    struct client* client = Chat->clients[j];
                    int nread = read(j, buf, sizeof(buf) - 1);
                    if (nread <= 0){
                        // 客户端关闭
                        closeClient(client);
                    }else{
                        buf[nread] = 0;
                        if (buf[0] == '/'){
                            // 发送的是命令，处理命令，目前只支持修改昵称 '/nike'
                        }else{
                            if (strlen(buf) == strlen(EXIT) && strncmp(buf, EXIT, strlen(EXIT)) == 0) {                                                                // 客户端关闭
                                closeClient(client);
                                continue;
                            }
                            // 发送的是消息，广播给其他客户端
                            // 消息格式： 发送者> 消息内容
                            // 总消息大小：消息长度 + 昵称长度 + 1(换行符) 
                            char message[nread + sizeof(client->nick_name) + 1];
                            int message_len = snprintf(message, sizeof(message), "%s> %s", client->nick_name, buf);
                            // snprintf 返回值可能大于 sizeof(message)
                            if (message_len >= (int)sizeof(message)){
                                message_len = sizeof(message) - 1;
                            }
                            printf("%s", message);
                            sendMessageToAllClientsBut(j, message, message_len);
                        }
                    }
                }
            }
        }else{
            // select 监听超时
            // Debug("server listen timeout...");
        }
    }
    return 0;
}