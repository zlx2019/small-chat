/** server.c -- SmallChat Client
 * 
 * 使用到的一些标识符描述
 *  - BRKINT: 如果设置了这个标志，当接收到 BREAK 信号时，会产生一个 SIGINT 信号。清除此标志可以防止接收到 BREAK 信号时触发中断。
 *  - ICRNL: 如果设置了这个标志，回车（CR）字符会被转换成换行（NL）字符。清除此标志可以防止这种转换。
 *  - INPCK: 允许奇偶校验。清除标志表示关闭校验。
 *  - ISTRIP: 如果设置了这个标志，输入的字节将被剥离到7位。清除此标志可以确保输入的字节不被剥离。    
 *  - IXON: 如果设置了这个标志，启用软件流控制（XON/XOFF）。清除此标志可以禁用软件流控制。
 * 
 *  - ECHO: 如果设置了这个标志，输入的字符会回显到终端上。清除此标志可以关闭回显。
 *  - ICANON: 如果设置了这个标志，终端处于规范模式（canonical mode）清除此标志可以使终端进入非规范模式。
 *  - IEXTEN: 如果设置了这个标志，启用实现定义的输入处理（如某些控制字符的特殊处理）。清除此标志可以禁用这些扩展输入处理功能。
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



/* ============================================================================
 * Terminal raw mode
 * ========================================================================== */
void disableRawModelAtExit(void);
/**
 * 设置 raw 模式 or 恢复为默认模式
 */
int setRawMode(int fd, int enable){
    /* 一些全局状态 */
    // 备份终端默认模式，用于结束后恢复原模式
    static struct termios def_terminal;
    // 状态值，用于避免多次执行atexit()
    static int atexit_registered = 0;
    // 是否已经设置为原始模式
    static int rawmode_is_set = 0;

    struct termios raw;

    // 是恢复默认模式
    if (enable == 0){
        if (rawmode_is_set && tcsetattr(fd, TCSAFLUSH, &def_terminal) != -1)
            rawmode_is_set = 0;
        return 0;
    }

    // 检查是否可以开启
    if (!isatty(fd)) goto fatal;

    // 程序终止前，恢复默认模式
    if (!atexit_registered) {
        atexit(disableRawModelAtExit);
        atexit_registered = 1;
    }
    // 获取当前模式，保留备份
    if (tcgetattr(fd,&def_terminal) == -1) goto fatal;
    

    // 定义原始模式
    raw = def_terminal;
    // 清除部分输入模式标志
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // 设置每个字符数据位为8位
    raw.c_cflag |= (CS8);
    // 清除一些 本地模式标志符
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    // 设置非规范模式下，read函数返回之前至少读取的字符数(表示至少读取到一个字符)
    raw.c_cc[VMIN] = 1;
    // 设置非规范模式下，read函数的阻塞超时时长，0表示不会超时，一直阻塞，直到读取到数据
    raw.c_cc[VTIME] = 0;
    // 将终端设置为原始模式，并且刷新.
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) goto fatal;
    rawmode_is_set = 1;
    return 0;
/* 设置 raw 模式失败 */
fatal:
    errno = ENOTTY;
    return -1;
}

/**
 * 程序终止前执行的函数，用于恢复终端标准输入的默认模式
 */
void disableRawModelAtExit(void){
    setRawMode(STDIN_FILENO, 0);
}

/**
 * 终端清除游标当前所在整行内容.
 */
void terminalCleanCurrentRow(void){
    write(fileno(stdout), "\e[2K", 4);
}

/**
 * 将终端光标移动到当前行的起始位置.
 */
void terminalCursorAtRowStart(void){
    write(fileno(stdout), "\r", 1);
}


/* ============================================================================
 * Custom input buffer
 * ========================================================================== */

#define BUF_MAX 128 
/**
 * 终端标准输入缓冲区，用于缓存标准输入数据
 */
struct InputBuffer {
    char buf[BUF_MAX];  // 数据缓冲区
    int len;            // 当前数据长度
};


/* 一些关于缓冲区操作的状态 */
#define BUF_ERR  0       // 写入失败，缓冲区不足
#define BUF_OK   1       // 写入成功
#define BUF_GOTLINE 2    // 按下了回车键，将缓冲区数据发送给服务端


/**
 * 将输入的字符，添加到缓冲区
 */
int inputBufferAppend(struct InputBuffer *buffer, int c){
    if (buffer->len >= BUF_MAX)
        // 已满
        return BUF_ERR;
    buffer->buf[buffer->len] = c;
    buffer->len++;
    return BUF_OK;
}

/**
 * 将终端当前行内容清除，并跳转到行起始位置.
 */
void inputBufferHide(struct InputBuffer *buffer){
    (void)buffer;
    terminalCleanCurrentRow();
    terminalCursorAtRowStart();
}

/**
 * 将缓冲区中的数据，输出到终端当前行
 */
void inputBufferShow(struct InputBuffer *buffer){
    write(fileno(stdout), buffer->buf, buffer->len);
}

/**
 * 重置缓冲区
 */
void inputBufferClear(struct InputBuffer *buffer){
    buffer->len = 0;
    inputBufferHide(buffer);
}


/**
 * 处理来自键盘的每一个按键事件
 * 将字符写入缓冲区，然后输出到终端.
 */
int inputBufferFeedChar(struct InputBuffer *buffer, int c){
    switch (c){
    case '\n':
        break; // 忽略换行符,不做处理
    case '\r':
        // 返回 BUF_GOTLINE 表示用户按下了回车键.
        return BUF_GOTLINE;
    case 127:
        // 退格键，删除缓冲区最后一个字符
        if (buffer->len > 0){
            buffer->len--;
            inputBufferHide(buffer);
            inputBufferShow(buffer);
        }
        break;
    default:
        if (inputBufferAppend(buffer, c) == BUF_OK){
            // 写入缓冲区成功，输出到stdout
            write(fileno(stdout), buffer->buf + buffer->len - 1, 1);
        }
        break;
    }
    return BUF_OK;
}

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
     *  - 也不转义任何特殊字符 */
    setRawMode(fileno(stdin), 1);

    // 定义标准输入缓冲区
    struct InputBuffer buffer;
    inputBufferClear(&buffer);
    
    // 通过select监听 服务端消息 && 标准输入
    fd_set listen_fds;
    int stdin_fd = fileno(stdin);
    while (1){
        FD_ZERO(&listen_fds);
        FD_SET(server, &listen_fds);
        FD_SET(stdin_fd, &listen_fds);
        int max_fd = server > stdin_fd ? server : stdin_fd;
        int num_evnets = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (num_evnets == -1){
            perror("client select error");
            exit(1);
        }else if (num_evnets){
            // 有io事件就绪
            char lines[128];

            // 服务端事件就绪
            if (FD_ISSET(server, &listen_fds)){
                ssize_t n = read(server, lines, sizeof(lines));
                if (n <= 0){
                    printf("Connection exit.\n");
                    exit(1);
                }
                // 清除当前行内容，输出服务端数据
                // 然后将缓冲区数据，输出到下一行
                inputBufferHide(&buffer);
                write(fileno(stdout), lines, n);
                inputBufferShow(&buffer);
            }else if (FD_ISSET(stdin_fd, &listen_fds)){
                // 终端标准输入事件就绪
                ssize_t n = read(stdin_fd, lines, sizeof(lines));
                // 处理从终端读取到的所有字符
                for (int j = 0; j < n; j++){
                    int res = inputBufferFeedChar(&buffer, lines[j]);
                    switch (res){
                    case BUF_GOTLINE:
                        // 用户按下了 Enter 键
                        if (buffer.len <= 0){
                            break;   
                        }
                        // 将缓冲区数据 输出到终端 && 发送给服务端
                        inputBufferAppend(&buffer, '\n');
                        inputBufferHide(&buffer);
                        write(fileno(stdout), "you> ", 5);
                        write(fileno(stdout), buffer.buf, buffer.len);
                        write(server, buffer.buf, buffer.len);
                        inputBufferClear(&buffer);
                        break;
                    case BUF_OK:
                        break;
                    }
                }
            }
        }
    }
    close(server);
    return 0;
}