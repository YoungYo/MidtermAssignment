#include "common.h"

# define MAXLINE 4096

void error(int status, int err, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (err)
        fprintf(stderr, ": %s (%d)\n", strerror(err), err);
    if (status)
        exit(status);
}
int main(int argc, char **argv) {
    char* COMMANDS[] = {"pwd", "cd", "ls"};
    int COMMAND_COUNT = 3;
    if (argc != 2) {
        error(1, 0, "usage: graceclient <IPaddress>");
    }
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    socklen_t server_len = sizeof(server_addr);
    int connect_rt = connect(socket_fd, (struct sockaddr *) &server_addr, server_len);
    if (connect_rt < 0) {
        error(1, errno, "connect failed ");
    }

    char send_line[MAXLINE], recv_line[MAXLINE + 1];
    int n;

    fd_set readmask; //fd_set是一个结构体，是一个存放文件描述符的集合
    fd_set allreads;

    FD_ZERO(&allreads); //将allreads里面的文件描述符清空
    FD_SET(0, &allreads); //标准输入的文件描述符是0，此处是将标准输入的文件描述符加入allreads
    FD_SET(socket_fd, &allreads); //将之前创建的socket的文件描述符加入allreads
    for (;;) {
        readmask = allreads;
        int rc = select(socket_fd + 1, &readmask, NULL, NULL, NULL); //select函数的作用是检查指定的文件描述符集合中的文件描述符是否发生了变化（是否可以读或写）
        if (rc <= 0)
            error(1, errno, "select failed");
        if (FD_ISSET(socket_fd, &readmask)) { //FD_ISSET的作用是检查集合中指定的文件描述符是否可以读写
            n = read(socket_fd, recv_line, MAXLINE);
            if (n < 0) {
                error(1, errno, "read error");
            } else if (n == 0) {
                error(1, 0, "server terminated \n");
            }
            recv_line[n] = 0;
            fputs(recv_line, stdout);
            fputs("\n", stdout);
        }
        if (FD_ISSET(0, &readmask)) {
            if (fgets(send_line, MAXLINE, stdin) != NULL) {
                int i = 0;
                for (; i < COMMAND_COUNT; i++){
                    if (strncmp(send_line, COMMANDS[i], strlen(COMMANDS[i])) == 0) {
//                        printf("Command: %s\n", send_line);
                        size_t rt = write(socket_fd, send_line, strlen(send_line));
                        if (rt < 0) {
                            error(1, errno, "write failed ");
                        }
//                        printf("send bytes: %zu \n", rt);
                        break;
                    }
                }
                if (i == COMMAND_COUNT){
                    if (strncmp(send_line, "quit", 4) == 0){
                        FD_CLR(0, &allreads);
                        if (shutdown(socket_fd, 2)) {
                            error(1, errno, "shutdown failed");
                        }
                    } else{
                        printf("Invalid input: %s\n", send_line);
                        continue;
                    }
                }
/*
                if (strncmp(send_line, "shutdown", 8) == 0) {
                    FD_CLR(0, &allreads);
                    if (shutdown(socket_fd, 1)) {
                        error(1, errno, "shutdown failed");
                    }
                } else if (strncmp(send_line, "close", 5) == 0) {
                    FD_CLR(0, &allreads);
                    if (close(socket_fd)) {
                        error(1, errno, "close failed");
                    }
                    sleep(6);
                    exit(0);
                } else {
                    int i = strlen(send_line);
                    if (send_line[i - 1] == '\n') {
                        send_line[i - 1] = 0;
                    }

                    printf("now sending %s\n", send_line);
                    size_t rt = write(socket_fd, send_line, strlen(send_line));
                    if (rt < 0) {
                        error(1, errno, "write failed ");
                    }
                    printf("send bytes: %zu \n", rt);
                }
*/
            }
        }
    }
}
